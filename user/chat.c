#include "menu.h"
#include "../lvgl/lvgl.h"
#include "../lvgl/demos/lv_demos.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

// 外部标识
extern lv_obj_t* page_menu;
extern lv_font_t Ch_make;
extern int page_flag;

void chat_network_stop(void);
#define CHAT_LOG_BUF_SIZE 1024
static char chat_log_buf[CHAT_LOG_BUF_SIZE] = {0};
static int chat_send_msg(const char *msg);
static int chat_send_get(const char *filename);
static int chat_send_put(const char *filename);

// 聊天室枚举
 enum MSGTYPE
{
    MSGDATA,   //普通消息
    MSGQUIT,    //退出消息
    MSGGET,     //下载
    MSGPUT,
    FILENAME,
    FILEDATA
};

int type=-1;
char chat_name[100]={0};
int chat_sockfd=-1;
time_t chat_now_time;
int chat_fd=-1;
int len=-1;

//线程互斥锁
pthread_mutex_t chat_net_mutex;
#define CHAT_UI_BUF_MAX 512
char chat_ui_buf[CHAT_UI_BUF_MAX]={0};
int chat_ui_msg_flag = 0;

//聊天室UI控件（新增全局）
lv_obj_t* page_chat=NULL;
static int build=0;
lv_obj_t *label_chat = NULL;
lv_obj_t *chat_ta_input = NULL;
lv_obj_t *panel_chat = NULL;
lv_obj_t* back_container=NULL;  //按钮容器


//my_read函数
int my_read(int fd, char* buf, int len)
{
    int readed_bytes = 0;
    int ret = -1;
    while (1)
    {
        ret = read(fd, buf+readed_bytes, len-readed_bytes);
        if(ret == -1)
            return -1;
        if(ret == 0)
            return readed_bytes;
        readed_bytes += ret;
        if(readed_bytes == len)
            return readed_bytes;
    }
}

//提取文件名，例如 /mywork/song.mp3 → song.mp3
static const char* get_file_name(const char *path)
{
    const char *p = strrchr(path, '/');
    if(p != NULL)
        return p + 1;
    return path;
}

static void chat_btn_send_cb(lv_event_t *e);
static void chat_btn_get_cb(lv_event_t *e);
static void chat_btn_put_cb(lv_event_t *e);
static void chat_back_click(lv_event_t *e);



static void chat_btn_send_cb(lv_event_t *e)
{
    (void)e;
    const char *text = lv_textarea_get_text(chat_ta_input);
        if(strncmp(text,"get ",4)==0)
        {
            chat_send_get(text+4);
            lv_textarea_set_text(chat_ta_input, "");
        }
        else if(strncmp(text,"put ",4)==0)
        {
            chat_send_put(text+4);
            lv_textarea_set_text(chat_ta_input, "");
        }
        else if(strcmp(text,"quit")==0)
        {
                        // 1. 停止网络，等待接收线程结束
            chat_network_stop();

            // 2. 删除整个聊天容器（内部所有控件自动删除）
            if(page_chat != NULL)
            {
                lv_obj_del(page_chat);
                page_chat = NULL;
            }

            // 3. 将所有聊天相关的全局控件指针置空，防止主循环访问野指针
            label_chat = NULL;
            panel_chat= NULL;
            chat_ta_input = NULL;
            panel_chat = NULL;  // 已经是野指针，置空即可

            // 4. 如果退出按钮容器是独立的（不在 page_chat 内），也删除
            if(back_container != NULL)
            {
                lv_obj_del(back_container);
                back_container = NULL;
            }

            // 5. 返回主界面
            main_1();
            return;  // 提前返回，避免后续重复清空输入框
    
        }
        else
        {
        chat_send_msg(text);          // 使用已有的安全函数
        lv_textarea_set_text(chat_ta_input, "");
        }
    lv_textarea_set_text(chat_ta_input, "");
}

static void chat_push_msg_ui(const char *str)
{
    pthread_mutex_lock(&chat_net_mutex);
    strncpy(chat_ui_buf, str, CHAT_UI_BUF_MAX-1);
    chat_ui_buf[CHAT_UI_BUF_MAX-1] = '\0';
    chat_ui_msg_flag = 1;
    pthread_mutex_unlock(&chat_net_mutex);
}

//接收
void* chat_recv_pthread(void* arg)
{
    (void)arg;
    pthread_detach(pthread_self()); //分离线程：线程退出自动回收资源，不用pthread_join
    int confd=chat_sockfd;
    int len = -1, type = -1,file=-1;
    char head_buf[8] = {0}; //存放头部8字节：4长度+4类型
    char buf[1024] = {0};
    int ret;
    while(1)
    {
        memset(head_buf, 0, sizeof(head_buf));
        //第一步读取8字节包头
        ret=my_read(confd,head_buf,8);
        if(ret <= 0)
        {
            chat_push_msg_ui("服务端断开连接");
            close(confd);
            pthread_exit(NULL); //线程结束
        }
        //解析包头，取出数据长度、消息类型
        sscanf(head_buf,"%4d%4d",&len,&type);

        //分支处理不同消息类型
        if(type == MSGDATA)
        {
            ret = my_read(confd, buf, len);
                if(ret > 0)
            {
                buf[ret] = '\0';   // 保证字符串结束
                chat_push_msg_ui(buf);
            }
        }
        else if(type==MSGQUIT)
        {
            //退出消息
        }
        else if(type == FILEDATA)
        {
            if(chat_fd < 0)   // 文件根本没打开，忽略后续数据
            {
                if(len > 0)
                    my_read(chat_sockfd, buf, len);   // 把数据读走，避免粘包
                continue;
            }

            if(len == 0)
            {
                close(chat_fd);
                chat_fd = -1;
                chat_push_msg_ui("文件下载完成");
            }
            else
            {
                ret = my_read(chat_sockfd, buf, len);
                if(chat_fd >= 0)      // 双重保险
                    write(chat_fd, buf, ret);
            }
        }
    }
}

static int chat_client_init(void)
{
    chat_sockfd = socket(AF_INET, SOCK_STREAM, 0); //创建tcp套接字
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(10086); //服务端端口10086
    addr.sin_addr.s_addr = inet_addr("192.168.176.185");//服务端IP
    int ret = connect(chat_sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret == -1)
    {
        close(chat_sockfd);
        chat_sockfd=-1;
        return -1;
    }
    return 0;
}

int chat_network_start(const char *user_name)
{
    pthread_mutex_init(&chat_net_mutex,NULL); //初始化互斥锁
    strncpy(chat_name, user_name, sizeof(chat_name)-1);
    int ret = chat_client_init();
    if(ret !=0) return -1;
    int type=-1,len=-1;
    pthread_t pid;
    int pd=pthread_create(&pid,NULL,chat_recv_pthread,NULL); //创建接收子线程

    //发送上线消息，告诉聊天室有人进来
    char tempbuf[500]={0};
    char send_buf[1000]={0};
    type=MSGDATA;
    time(&chat_now_time);
    sprintf(tempbuf,"%s %s 加入聊天室",ctime(&chat_now_time),chat_name);
    len=strlen(tempbuf);
    sprintf(send_buf,"%4d%4d%s",len,type,tempbuf);
    write(chat_sockfd,send_buf,len+8);
    return 0;
}

void chat_network_stop(void)
{
    pthread_mutex_lock(&chat_net_mutex);
    if(chat_sockfd>0){
        close(chat_sockfd); //关闭socket，子线程my_read会返回0，线程自动退出
        chat_sockfd = -1;
    }
    pthread_mutex_unlock(&chat_net_mutex);
    pthread_mutex_destroy(&chat_net_mutex); //销毁锁
}

//聊天文本框
static void chat_event(lv_event_t* e)
{
    //获取事件编码
    lv_event_code_t code=lv_event_get_code(e);
    //哪个文本框触发的事件
    lv_obj_t* ta=lv_event_get_target(e);
    //用户转递过来的键盘
    lv_obj_t* kb=lv_event_get_user_data(e);

    //鼠标点击了文本框
    if(code==LV_EVENT_FOCUSED)
    {
        //键盘绑定文本框
        lv_keyboard_set_textarea(kb,ta);
        //解除键盘隐藏属性
        lv_obj_remove_flag(kb,LV_OBJ_FLAG_HIDDEN);
    }
    if(code==LV_EVENT_DEFOCUSED)
    {
        //键盘绑解除文本框
        lv_keyboard_set_textarea(kb,NULL);
        //键盘隐藏
        lv_obj_add_flag(kb,LV_OBJ_FLAG_HIDDEN);
    }
    if(code == LV_EVENT_READY) //按下了确认键
    {
        char send_buf[100]={0};
        //键盘隐藏
        lv_obj_add_flag(kb,LV_OBJ_FLAG_HIDDEN);
        const char* chat_text=lv_textarea_get_text(chat_ta_input);
        if(strncmp(chat_text,"get ",4)==0)
        {
            chat_send_get(chat_text+4);
        }
        else
        {
        chat_send_msg(chat_text);          // 使用已有的安全函数
        lv_textarea_set_text(chat_ta_input, "");
        }
    }
}


//发送普通聊天文本
static int chat_send_msg(const char *msg)
{
    if(chat_sockfd <= 0)
    {
        printf("未连接服务端\n");
        return -1;
    }
    int len = strlen(msg);
    enum MSGTYPE type = MSGDATA;
    char send_buf[1024] = {0};
    sprintf(send_buf,"%04d%04d%s", len, type,msg);
    write(chat_sockfd, send_buf, len + 8);
    return 0;
}

//发送get下载文件指令
static int chat_send_get(const char *filename)
{
    if(chat_sockfd <=0) return -1;
    int len = strlen(filename);
    type = MSGGET;
    char send_buf[256]={0};
    sprintf(send_buf,"%04d%04d%s", len, type, filename);
    write(chat_sockfd, send_buf, len + 8);

    char full_path[300];
    snprintf(full_path, sizeof(full_path), "/mywork/%s", filename);
    //本地打开文件，准备接收服务端下发的FILEDATA数据包
    chat_fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0664);
    if(chat_fd < 0)
    {
        perror("open download file");
        return -1;
    }
    return 0;
}

//发送put上传文件指令
static int chat_send_put(const char *filename)
{
    if(chat_sockfd <=0) return -1;
    int fd = open(filename,O_RDONLY);
    if(fd <0)
    {
        perror("open upload file");
        return -1;
    }
    //第一步发送PUT指令+文件名
    int name_len = strlen(filename);
    type = MSGPUT;
    char send_buf[1024]={0};
    sprintf(send_buf,"%04d%04d%s", name_len, type, filename);
    write(chat_sockfd, send_buf, name_len+8);

    //循环读取本地文件，发送FILEDATA数据包
    char file_buf[992]={0};
    int ret;
    while( (ret = read(fd, file_buf, sizeof(file_buf))) >0 )
    {
        char pkg[1024]={0};
        sprintf(pkg,"%04d%04d", ret, FILEDATA);
        memcpy(pkg+8, file_buf, ret);
        write(chat_sockfd, pkg, ret+8);
    }
    //发送长度0包，代表文件传输结束
    char end_pkg[8]={0};
    sprintf(end_pkg,"%04d%04d",0,FILEDATA);
    write(chat_sockfd, end_pkg,8);

    close(fd);
    return 0;
}

/*============构建主界面=============*/
void build_page(void)
{
    // 总界面容器
    page_chat = lv_obj_create(lv_screen_active());
    lv_obj_set_size(page_chat, 1024, 600);
    lv_obj_set_align(page_chat, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(page_chat, lv_color_hex(0x222222), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(page_chat, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(page_chat, 0, LV_STATE_DEFAULT);
    lv_obj_add_flag(page_chat, LV_OBJ_FLAG_HIDDEN);

    // 聊天区域
    panel_chat=lv_obj_create(page_chat);
    lv_obj_set_size(panel_chat, LV_HOR_RES-40, 350);
    lv_obj_align(panel_chat,LV_ALIGN_TOP_MID,0,20);

    //聊天显示标签
    label_chat = lv_label_create(panel_chat);
    lv_label_set_long_mode(label_chat,LV_LABEL_LONG_WRAP);
    //lv_obj_set_size(label_chat, LV_HOR_RES-40, 350);
    //lv_obj_align(label_chat,LV_ALIGN_TOP_MID,0,20);
    lv_label_set_text(label_chat,"聊天:\n");
    set_cn_font_red(label_chat);

    lv_obj_set_flex_flow(panel_chat, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel_chat, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(panel_chat, LV_DIR_VER); // 垂直方向滚动
    lv_obj_add_flag(panel_chat, LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_ELASTIC);

    //输入框
    //创建一个键盘
    lv_obj_t * kb = lv_keyboard_create(lv_screen_active());
    chat_ta_input = lv_textarea_create(page_chat);
    lv_obj_set_size(chat_ta_input, 450,60);
    lv_obj_align(chat_ta_input,LV_ALIGN_CENTER,0,-20);
    set_cn_font_red(chat_ta_input);
    lv_textarea_set_placeholder_text(chat_ta_input,"send");
    //文本框添加事件   传递的是键盘控件
    lv_obj_add_event_cb(chat_ta_input, chat_event, LV_EVENT_ALL, kb);

    //发送按钮
    lv_obj_t *btn_send = lv_button_create(page_chat);
    lv_obj_set_size(btn_send,120,60);
    lv_obj_align_to(btn_send,chat_ta_input,LV_ALIGN_OUT_RIGHT_MID,10,0);
    lv_obj_t *lab_send = lv_label_create(btn_send);
    lv_label_set_text(lab_send,"发出");
    set_cn_font_red(lab_send);
    lv_obj_center(lab_send);
    lv_obj_add_event_cb(btn_send,chat_btn_send_cb,LV_EVENT_CLICKED,NULL);
    back_btn();

    //启动网络，用户名复用登录账号，这里写死，你也可以对接登录的账号
    chat_network_start("lvgl_user");

    // 退回
    page_flag=-1;
    back_prev(page_chat);
}

/*============进入=============*/
static void entry_cb(lv_event_t* e)
{
    (void)e;
    build=0;
    if(!build)
    {
        build_page();
        build=1;
    }
    lv_obj_add_flag(page_menu,LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(page_chat,LV_OBJ_FLAG_HIDDEN);
}

void chat_icon(void)
{
    lv_obj_t* btn=lv_button_create(page_menu);
    lv_obj_set_size(btn,120,120);
    lv_obj_align(btn,LV_ALIGN_LEFT_MID,510,0);

    lv_obj_t* label=lv_label_create(btn);
    lv_label_set_text(label,"社交");
    lv_obj_set_parent(label,lv_obj_get_parent(btn));
    lv_obj_align_to(label,btn,LV_ALIGN_OUT_BOTTOM_MID,-20,0);
    set_cn_font(label);
    
    lv_obj_add_event_cb(btn,entry_cb,LV_EVENT_CLICKED,NULL);
}