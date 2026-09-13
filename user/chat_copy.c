#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
//#include "lv_mygec_frnt.h"
#include <stdio.h>
#include "lv_mygec_font.h"
#include "./mysrc/us100.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

static int i=0;

static lv_obj_t * image ;

#define MINE_ROW 8
#define MINE_COL 8
#define MINE_COUNT 10



#define FOOD_NUM (sizeof(food_list) / sizeof(fooditem))     //菜单数量定义
#define USER_COUNT (sizeof(user_list) / sizeof(userinfo))   //账户数量定义
#define IMG_MAX_NUM_12 (sizeof(path_12)/sizeof(path_12[0])) //照片数量定义

lv_obj_t *user;             //账户
lv_obj_t *password;         //密码
lv_obj_t* btn_12_photo=NULL;//相册按钮设置
lv_obj_t* btn_12_re=NULL;   //点餐按钮设置
lv_obj_t * label_music_name = NULL; //歌名设置
lv_obj_t* num_label;                //数量标签
lv_obj_t* total_label;              //总价格标签
lv_obj_t* photo_container=NULL;   //相册容器
lv_obj_t* re_container=NULL;     //点餐容器
lv_obj_t* video_container=NULL; //音乐容器
lv_obj_t* exe_container=NULL;   //菜单价格容器
lv_obj_t* back_container=NULL;  //按钮容器
lv_obj_t* main_obj=NULL;        //主界面容器
lv_obj_t* btn_back=NULL;        //按钮设置
lv_obj_t* total_price;          //总价格设置
lv_obj_t *btn_12_video;         //音乐按钮设置
lv_timer_t *g_song_timer = NULL;    //切歌定时器
lv_timer_t *g_ui_timer = NULL;      //歌名切换定时器
static int mplayer_running = 0;     //音乐播放标志位
static uint8_t is_pause_state = 0; //0播放状态，1暂停状态
static lv_obj_t *label_pause_icon;  //暂停标签
static uint8_t need_switch_song = 0;//歌名更新标志位
static lv_obj_t * image_12 ;        //图片设置

int mine_board[MINE_ROW][MINE_COL]; //棋盘数组 -1为地雷   0-8为周围地雷数
int mine_state[MINE_ROW][MINE_COL]; //记录格子状态 0-未开 1-已开   -1-标记地雷
lv_obj_t *mine_btn[MINE_ROW][MINE_COL];//保证棋盘上按钮
int mine_game_status;                   //游戏状态变量  1-进行 2-失败  3-成功

void mine_init_board(void);
int mine_get_mine_num(int r, int c);
void mine_create_ui(lv_event_t *e);
static void mine_cell_cb(lv_event_t *e);
void mine_open_cell(int r, int c);
void mine_check_win(void);
static void mine_restart_cb(lv_event_t *e);




void chat_network_stop(void);
#define CHAT_LOG_BUF_SIZE 1024
static char chat_log_buf[CHAT_LOG_BUF_SIZE] = {0};
static int chat_send_msg(const char *msg);
static int chat_send_get(const char *filename);
static int chat_send_put(const char *filename);
// 聊天室枚举、全局变量（和你之前客户端完全一致，不改名）
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
lv_obj_t *chat_container = NULL;
lv_obj_t *chat_label_show = NULL;
lv_obj_t *chat_ta_input = NULL;
lv_obj_t *chat_colum_cnotainer = NULL;


int order_num=0;        //菜品数量
int price=0;            //价格
lv_obj_t* sum=0;        //总价格
int j=0;                //音乐下标
char* mp3[] = {"huluwa.mp3", "lanjingling.mp3","04.flv"};
int num=sizeof(mp3)/sizeof(mp3[0]);     //歌曲数量

//账户初始化
typedef struct{
    char username[32];
    char password[32];
}userinfo;
//账户信息
userinfo user_list[]={
    {"zhangsan","123456789"},
    {"lisi","666666"},
    {"wangwu","8888888"},
    {"1","1"},
    {"q","q"}
};
//菜单初始化
typedef struct{
    const char* food_name;
    int unit_price;
    int count;
    lv_obj_t* label_count;
}fooditem;
//菜单信息
fooditem food_list[]={
    {"牛肉面",18,0,NULL},
    {"薯条",15,0,NULL},
    {"可乐",3,0,NULL},
    {"汤",1,0,NULL},
    {"白菜",6,0,NULL},
    {"腊肉",20,0,NULL},
};
//图片名单
char* path_12[]= {
    "A:/mywork/1.bmp",
    "A:/mywork/2.bmp",
    "A:/mywork/3.bmp",
    "A:/mywork/4.bmp",
    "A:/mywork/5.bmp",
    };

#define MAX_PHOTO_FILES 100
static char photo_paths[MAX_PHOTO_FILES][256];  // 存储图片路径
static int photo_count = 0;                     // 实际图片数量


//主界面函数
void main_1();

//my_read函数原样复制
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

//棋盘初始化
/*void mine_init_board(void)
{
 //1.全部清零
    for(int i = 0; i < MINE_ROW; i++)
    {
        for(int j = 0; j < MINE_COL; j++)
        {
            mine_board[i][j] = 0;
            mine_state[i][j] = 0;
        }
    }
    mine_game_status = 1;

    //2.随机放地雷
    srand(time(NULL));
    int cnt = 0;
    while(cnt < MINE_COUNT)
    {
        int r = rand() % MINE_ROW;
        int c = rand() % MINE_COL;
        if(mine_board[r][c] != -1)
        {
            mine_board[r][c] = -1;
            cnt++;
        }
    }
    //3.遍历每个格子，如果不是地雷，计算周围地雷数
    for(int i = 0; i < MINE_ROW; i++)
    {
        for(int j = 0; j < MINE_COL; j++)
        {
            if(mine_board[i][j] == -1)
                continue;
            mine_board[i][j] = mine_get_mine_num(i,j);
        }
    }

}
//周围地雷计算
int mine_get_mine_num(int r, int c)
{
    int num = 0;
    //dr偏移行，dc偏移列，-1、0、1代表上下左右斜八个方向
    for(int dr = -1; dr <= 1; dr++)
    {
        for(int dc = -1; dc <= 1; dc++)
        {
            //跳过自己本身
            if(dr == 0 && dc == 0)
                continue;

            //计算相邻格子坐标
            int nr = r + dr;
            int nc = c + dc;
            //判断坐标不能越界
            if(nr >=0 && nr < MINE_ROW && nc >=0 && nc < MINE_COL)
            {
                if(mine_board[nr][nc] == -1)
                {
                    num++;
                }
            }
        }
    }
    return num;
}
//扫雷界面
void mine_create_ui(lv_event_t *e)
{
    (void)e;

    //1.全屏容器
    lv_obj_t *container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(container, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    //2.重新开始按钮
    lv_obj_t *btn_restart = lv_button_create(container);
    lv_obj_set_size(btn_restart,120,50);
    lv_obj_align(btn_restart,LV_ALIGN_TOP_MID,0,10);
    lv_obj_t *lab_restart = lv_label_create(btn_restart);
    lv_label_set_text(lab_restart,"重新开始");
    lv_obj_center(lab_restart);
    lv_obj_add_event_cb(btn_restart, mine_restart_cb, LV_EVENT_CLICKED, NULL);

    //3.棋盘容器
    lv_obj_t *board_cont = lv_obj_create(container);
    lv_obj_set_size(board_cont, 320,320);
    lv_obj_align(board_cont, LV_ALIGN_TOP_MID, 0,80);
    lv_obj_set_flex_flow(board_cont,LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(board_cont,LV_FLEX_ALIGN_SPACE_EVENLY,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(board_cont,LV_OBJ_FLAG_SCROLLABLE);

    //初始化棋盘数据
    mine_init_board();



}*/


//界面退出
void back(lv_event_t * e)
{
    //销毁相册
    if(exe_container!=NULL)
    {
    lv_obj_clean(exe_container);
    exe_container=NULL;
    }
    if(photo_container!=NULL)
    {
        lv_obj_clean(photo_container);
        photo_container=NULL;
        i=0;
        image_12=NULL;
    }

    if(back_container!=NULL)
    {
    lv_obj_clean(back_container);
    back_container=NULL;
    }

     if(main_obj!=NULL)
     {
     lv_obj_add_flag(main_obj,LV_OBJ_FLAG_HIDDEN);
     main_obj=NULL;
     }

    if(g_song_timer != NULL)
    {
        lv_timer_del(g_song_timer);
        g_song_timer = NULL;
    }
    
      //重置暂停状态和图标
    is_pause_state = 0;
    if(label_pause_icon != NULL)
    {
        lv_label_set_text(label_pause_icon, "\xEF\x80\x8C");
    }
    if(chat_container!= NULL)
    {
        lv_obj_clean(chat_container);
       chat_container = NULL;
    }
    if(chat_colum_cnotainer!= NULL)
    {
         lv_obj_clean(chat_colum_cnotainer);
       chat_colum_cnotainer = NULL;
    }
    system("killall -9 mplayer64 >/dev/null 2>&1");
    usleep(120000);
     main_1();

}
//退出按钮设置
void back_btn()
{
      static lv_style_t style;
    lv_style_init(&style);
    //设置标签内容的字体
    lv_style_set_text_font(&style, &lv_mygec_font);
    //设置标签内容的颜色
    lv_style_set_text_color(&style, lv_color_hex(0xFF0000));

    back_container=lv_obj_create(lv_screen_active());
    lv_obj_set_size(back_container, 120, 50);
    lv_obj_align(back_container, LV_ALIGN_TOP_LEFT,0,0);
    //退出
     btn_back = lv_button_create(back_container);
    //设置按钮的大小
    lv_obj_set_size(btn_back, 120, 50);                // 按钮尺寸缩小
    //设置按钮控件的位置
    lv_obj_align(btn_back, LV_ALIGN_CENTER,0,0);  // 左上角，留一点边距
    //在按钮上创建一个标签控件
    lv_obj_t* label3 = lv_label_create(btn_back);
      
    //设置标签控件的位置
    lv_obj_set_align(label3, LV_ALIGN_CENTER);
     lv_label_set_text(label3,"退出");
    //将样式添加到标签
    lv_obj_add_style(label3, &style, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn_back,back, LV_EVENT_CLICKED, NULL);
}
//提取文件名，例如 /mywork/song.mp3 → song.mp3
static const char* get_file_name(const char *path)
{
    const char *p = strrchr(path, '/');
    if(p != NULL)
        return p + 1;
    return path;
}
//更新歌名
static void refresh_music_name(void)
{
    if(label_music_name == NULL)
        return;
    const char *name = get_file_name(mp3[j]);
    lv_label_set_text(label_music_name, name);
}
//音乐播放
static void play_music()
{
  system("killall -9 mplayer64 >/dev/null 2>&1");
   usleep(150000);   //给一点时间进程结束
   system("dd if=/dev/zero of=/dev/fb0 >/dev/null 2>&1");
    lv_obj_invalidate(lv_screen_active());
      mplayer_running=0;

    if(mplayer_running == 1)
    {
        return; //已经启动直接返回，禁止重复启动mplayer
    }
    char cmd[1000] = {0};
    snprintf(cmd, sizeof(cmd), "mplayer64 -slave -idle -ao alsa -vo fbdev2 -input file=/mywork/2.fifo %s &", mp3[j]);
    system(cmd);
    mplayer_running=1;
    refresh_music_name(); //更新歌名
}
//歌名更新
static void song_switch_timer_cb(lv_timer_t * timer)
{
    (void)timer;
    if(need_switch_song == 1)
    {
        need_switch_song = 0;
        play_music();
        refresh_music_name(); //更新歌名
    }
}
//音乐控制
void video_change(lv_event_t *e)
{

    uintptr_t temp = (uintptr_t)lv_event_get_user_data(e);
    int btn_id = (int)temp;

    if(mplayer_running != 1)
    {
        return; //mplayer没启动，直接返回，不要发指令
    }

    char cmd[1000] = {0};
    char* cmd1;
    char* cmd2;
     int fd = open("/mywork/2.fifo", O_RDWR);
    if(fd == -1)
    {
        perror("open error");
        return ;
    }

    switch(btn_id)
    {
        case 1:
            if(j>0)
                j--;
            else
                j=num-1;

            //调用系统命令清空fb，擦掉残留视频画面，不杀mplayer进程
            /*
           system("dd if=/dev/zero of=/dev/fb0 >/dev/null 2>&1");
            lv_obj_invalidate(lv_screen_active());
            snprintf(cmd, sizeof(cmd), "loadfile %s\n", mp3[j]);
            write(fd, cmd, strlen(cmd)); 
            */
        
            need_switch_song = 1;
            refresh_music_name(); //更新歌名
            is_pause_state = 1;
                lv_label_set_text(label_pause_icon,">"); //▶播放
            break;
    
        case 2:
            if(j<num-1)
                j++;
            else
                j=0;

            //调用系统命令清空fb，擦掉残留视频画面，不杀mplayer进程
          /*
            system("dd if=/dev/zero of=/dev/fb0 >/dev/null 2>&1");
            lv_obj_invalidate(lv_screen_active());
            snprintf(cmd, sizeof(cmd), "loadfile %s\n", mp3[j]);
            write(fd, cmd, strlen(cmd)); 
            */
            need_switch_song = 1;
            refresh_music_name(); //更新歌名
            is_pause_state = 1;
            lv_label_set_text(label_pause_icon,">"); //▶播放
            break;
        
        case 3:
              cmd1 = "pause\n";
            write(fd, cmd1, strlen(cmd1)); 
             //切换图标
            if(is_pause_state == 0)
            {
                is_pause_state = 1;
                lv_label_set_text(label_pause_icon,">"); //▶播放
            }
            else
            {
                is_pause_state = 0;
                lv_label_set_text(label_pause_icon, "||"); //⏸暂停
            }
            break;
    
        case 4:
            cmd2 = "quit\n";
            write(fd, cmd2, strlen(cmd2));
            mplayer_running=0;
            break;
        default:
            break;
    }
  close(fd);
}
//音乐界面
void video(lv_event_t *e)
{
    static lv_style_t style;
    lv_style_init(&style);
    //设置标签内容的字体
    lv_style_set_text_font(&style, &lv_mygec_font);
    //设置标签内容的颜色
    lv_style_set_text_color(&style, lv_color_hex(0xFF0000));

    // 创建音乐容器，承载所有音乐按钮、图片
    video_container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(video_container, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(video_container, LV_OBJ_FLAG_SCROLLABLE); // 关闭滚动条


    lv_obj_t* btn1 = lv_button_create(video_container);
    //设置按钮的大小
    lv_obj_set_size(btn1, 200, 150);
     //设置按钮控件的位置
    lv_obj_set_align(btn1, LV_ALIGN_BOTTOM_LEFT);
    //在按钮上创建一个标签控件
    lv_obj_t* label1 = lv_label_create(btn1);
    //设置标签控件的位置
    lv_obj_set_align(label1, LV_ALIGN_CENTER);
    lv_label_set_text(label1,"<<<");
    //将样式添加到标签
    lv_obj_add_style(label1, &style, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn1,video_change, LV_EVENT_CLICKED, (void*)(uintptr_t)1);


     lv_obj_t* btn2 = lv_button_create(video_container);
   
    //设置按钮的大小
    lv_obj_set_size(btn2, 200, 150);
    //在按钮上创建一个标签控件
    lv_obj_t* label2 = lv_label_create(btn2);
      //设置按钮控件的位置
    lv_obj_set_align(btn2, LV_ALIGN_BOTTOM_RIGHT);
    //设置标签控件的位置
    lv_obj_set_align(label2, LV_ALIGN_CENTER);
     lv_label_set_text(label2,">>>");
    //将样式添加到标签
    lv_obj_add_style(label2, &style, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn2,video_change, LV_EVENT_CLICKED, (void*)(uintptr_t)2);


     lv_obj_t* btn3 = lv_button_create(video_container);
     //设置按钮的大小
    lv_obj_set_size(btn3, 100, 75);
      //设置按钮控件的位置
    lv_obj_align(btn3, LV_ALIGN_BOTTOM_MID,-75,0);

    lv_obj_t* label_pause = lv_label_create(btn3);
    label_pause_icon = label_pause;
    
     lv_label_set_text(label_pause_icon,"||");
     lv_obj_center(label_pause_icon);
    //将样式添加到标签
    lv_obj_add_style(label_pause_icon, &style, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn3,video_change, LV_EVENT_CLICKED, (void*)(uintptr_t)3);

    lv_obj_t* btn4 = lv_button_create(video_container);
    //设置按钮的大小
    lv_obj_set_size(btn4, 100, 75);
    //在按钮上创建一个标签控件
    lv_obj_t* label4 = lv_label_create(btn4);
      //设置按钮控件的位置
    lv_obj_align(btn4, LV_ALIGN_BOTTOM_MID,75,0);
    //设置标签控件的位置
    lv_obj_set_align(label4, LV_ALIGN_CENTER);
     lv_label_set_text(label4,"stop");
    //将样式添加到标签
    lv_obj_add_style(label4, &style, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn4,video_change, LV_EVENT_CLICKED, (void*)(uintptr_t)4);

    //歌曲名称显示标签
    label_music_name = lv_label_create(video_container);
    lv_label_set_long_mode(label_music_name, LV_LABEL_LONG_SCROLL_CIRCULAR); //文字太长自动滚动
    lv_obj_set_width(label_music_name, 320);
    lv_obj_align(label_music_name, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_style(label_music_name, &style, LV_STATE_DEFAULT);
   // lv_obj_set_style_text_font(label_music_name, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_label_set_text(label_music_name, "等待播放");

    back_btn();

    //创建切歌定时器
    if(g_song_timer == NULL)
    {
        g_song_timer = lv_timer_create(song_switch_timer_cb, 300, NULL);
        lv_timer_set_repeat_count(g_song_timer, -1);
    }

    int fd = open("/mywork/2.fifo", O_RDWR);
    play_music();
    usleep(200000);  //等待mplayer启动完成
    //发送pause，立刻暂停音乐
    write(fd, "pause\n", 6);
        close(fd);
}


static void scan_photo_files(void)
{
    photo_count = 0;
    DIR *dir = opendir("/mywork");       // 改成你实际保存下载图片的目录
    if (dir == NULL) {
        perror("opendir");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // 只取 .bmp 结尾的文件
        if (strstr(entry->d_name, ".bmp") != NULL)
         {
            if (photo_count < MAX_PHOTO_FILES) 
            {
                snprintf(photo_paths[photo_count], sizeof(photo_paths[0]),"A:/mywork/%s", entry->d_name);   // 加上 LVGL 需要的盘符
                photo_count++;
            }
        }
    }
    closedir(dir);
}
//图片切换
static void change(lv_event_t* e)
{
    lv_obj_t* label=lv_event_get_user_data(e);
    char* str=lv_label_get_text(label);
    if (photo_count == 0) 
    return;
    if(strcmp(str, "Prev") == 0)
    {
        if(i>0)
        {
            i--;
        }
        else
        i=photo_count -1;
    }
    else if(strcmp(str, "Next") == 0)
    {
        if(i<photo_count -1)
        {
            i++;
        }
        else
        i=0;
    }
    lv_image_set_src(image_12,  photo_paths[i]);

}
//相册
void photo(lv_event_t * e)
{
       

     if(main_obj!=NULL)
    {
        lv_obj_add_flag(main_obj,LV_OBJ_FLAG_HIDDEN);
        main_obj=NULL;
    }
    scan_photo_files();
    if(photo_count == 0)
    {
        // 简单提示
    lv_obj_t *msg = lv_label_create(lv_screen_active());
    lv_label_set_text(msg, "暂无图片");
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, 0);
    }
    i=0;
    image_12=NULL;
      static lv_style_t style;
    lv_style_init(&style);
    //设置标签内容的字体
    lv_style_set_text_font(&style, &lv_mygec_font);
    //设置标签内容的颜色
    lv_style_set_text_color(&style, lv_color_hex(0xFF0000));

 // 创建相册容器，承载所有相册按钮、图片
    photo_container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(photo_container, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(photo_container, LV_OBJ_FLAG_SCROLLABLE); // 关闭滚动条

   

     lv_obj_t* btn1 = lv_button_create(photo_container);
    //设置按钮的大小
    lv_obj_set_size(btn1, 200, 150);
     //设置按钮控件的位置
    lv_obj_set_align(btn1, LV_ALIGN_BOTTOM_LEFT);
    //在按钮上创建一个标签控件
    lv_obj_t* label1 = lv_label_create(btn1);
    //设置标签控件的位置
    lv_obj_set_align(label1, LV_ALIGN_CENTER);
    lv_label_set_text(label1,"Prev");
    //将样式添加到标签
    lv_obj_add_style(label1, &style, LV_STATE_DEFAULT);
     lv_obj_set_user_data(btn1, (void*)1);
    lv_obj_add_event_cb(btn1,change, LV_EVENT_CLICKED, label1);
   

     lv_obj_t* btn2 = lv_button_create(photo_container);
    //设置按钮的大小
    lv_obj_set_size(btn2, 200, 150);
    //在按钮上创建一个标签控件
    lv_obj_t* label2 = lv_label_create(btn2);
      //设置按钮控件的位置
    lv_obj_set_align(btn2, LV_ALIGN_BOTTOM_RIGHT);
    //设置标签控件的位置
    lv_obj_set_align(label2, LV_ALIGN_CENTER);
     lv_label_set_text(label2,"Next");
      lv_obj_set_user_data(btn2, (void*)0);
    //将样式添加到标签
    lv_obj_add_style(label2, &style, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn2,change, LV_EVENT_CLICKED, label2);

        printf("i = %d\n", i);
        printf("path_12指针 %p\n", path_12);

       //在活动屏幕上创建一个图片控件 --- 静态控件
	image_12 = lv_image_create(photo_container);
    //在图片控件中导入图片数据，在开发板获取图片路径: /home/CS2614F/1.bmp
    //在代码中需要加上盘符: A:/home/CS2614F/1.bmp
	lv_image_set_src(image_12, photo_paths[i]);
    //设置图像的位置
    lv_obj_set_align(image_12, LV_ALIGN_CENTER);
  

    
    back_btn();
           

}
//价格更新
static void refresh(void)
{
    int all_total=0;
    for(int i = 0; i < FOOD_NUM; i++)
    {
        all_total += food_list[i].unit_price * food_list[i].count;
    }
    char buf[64];
    sprintf(buf, "%d", all_total);
    lv_label_set_text(total_label, buf);
}
//份数更新
static void num_change(lv_event_t * e)
{
    uintptr_t temp = (uintptr_t)lv_event_get_user_data(e);
    int idx = (int)temp;

    lv_event_code_t code=lv_event_get_code(e);
    lv_obj_t* btn=lv_event_get_target(e);

    if(lv_obj_get_user_data(btn) == 0)
    {
        if(food_list[idx].count> 0) // 最少1份，不能减到0
            food_list[idx].count--;
    }
    // 加按钮
    else
    {
        food_list[idx].count++;
    }

    // 更新数字显示
    char buf[8];
    sprintf(buf, "%d", food_list[idx].count);
    lv_label_set_text(food_list[idx].label_count, buf);
    refresh();
}
//菜单
void item()
{
     if(main_obj!=NULL)
    {
    lv_obj_add_flag(main_obj,LV_OBJ_FLAG_HIDDEN);
    }
    if(btn_12_video != NULL)
        lv_obj_add_flag(btn_12_video,LV_OBJ_FLAG_HIDDEN);

  static lv_style_t style;
    lv_style_init(&style);
    //设置标签内容的字体
    lv_style_set_text_font(&style, &lv_mygec_font);
    //设置标签内容的颜色
    lv_style_set_text_color(&style, lv_color_hex(0xFF0000));

  

    // 下方re容器，中间偏下，支持水平滚动
    re_container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(re_container, 1024,600);
    lv_obj_align(re_container, LV_ALIGN_CENTER, 0, 0);
    // 先清除可能存在的默认内边距
    //lv_obj_set_style_pad_all(re_container, 0, 0);
    lv_obj_set_style_pad_top(re_container, 180, 0);
    //lv_obj_set_style_pad_bottom(re_container, 0, 100);

   

     // 上方exe容器，顶部区域
    exe_container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(exe_container,200,150);
    lv_obj_align(exe_container, LV_ALIGN_TOP_MID, 0, 10);

    // 菜品多了开启横向滚动
    //lv_obj_add_flag(re_container, LV_OBJ_FLAG_SCROLL_ELASTIC);
    // 横向均分、垂直居中
    //lv_obj_set_flex_align(re_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY);


    lv_obj_set_flex_flow(re_container, LV_FLEX_FLOW_ROW);
    // 修改对齐 START从左向右排布
    lv_obj_set_flex_align(re_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    // 设置菜品之间空隙
    //lv_obj_set_style_flex_grow(re_container,50,LV_PART_MAIN);
    //开启水平滚动
    lv_obj_add_flag(re_container, LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_ELASTIC);
    //lv_obj_clear_flag(re_container, LV_OBJ_FLAG_SCROLL_ELASTIC);

    for(i=0;i<FOOD_NUM;i++)
    {
    //设置窗口
        lv_obj_t* order1=lv_obj_create(re_container);
    //设置窗口的大小
        lv_obj_set_size(order1, 220, 270);
     //设置窗口控件的位置
    lv_obj_set_flex_flow(order1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(order1, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, 0);


    //在窗口上创建一个标签控件
    lv_obj_t* label_name = lv_label_create(order1);
    //数组缓冲区
    char buf_name[32];
    //文字拼接
    sprintf(buf_name,"%s\n%d元",food_list[i].food_name,food_list[i].unit_price);
    //把文字显示在标签上
    lv_label_set_text(label_name,buf_name);
    //将样式添加到标签
    lv_obj_add_style(label_name, &style, LV_STATE_DEFAULT);


    //中间窗口w
    lv_obj_t* num_box=lv_obj_create(order1);
     //设置窗口的大小
    lv_obj_set_size(num_box, 150,90 );

    lv_obj_set_flex_flow(num_box, LV_FLEX_FLOW_ROW);
    //内部子控件水平排列
    lv_obj_set_flex_align(num_box, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, 0);

    //减按钮
    lv_obj_t* btn_sub=lv_button_create(num_box);
    lv_obj_set_size(btn_sub,30,30);
    lv_obj_align(btn_sub,LV_ALIGN_LEFT_MID,0,90);
    lv_obj_t* label_sub=lv_label_create(btn_sub);
    lv_label_set_text(label_sub,"-");
    lv_obj_set_size(label_sub,20,20);
    lv_obj_set_align(label_sub,LV_ALIGN_CENTER);
    lv_obj_add_style(label_sub, &style, LV_STATE_DEFAULT);
    lv_obj_set_user_data(btn_sub, (void*)0);
    lv_obj_add_event_cb(btn_sub, num_change, LV_EVENT_CLICKED, (void*)(uintptr_t)i);

     // 中间数字显示标签
    food_list[i].label_count = lv_label_create(num_box);
    char buf1[16];
    sprintf(buf1, "%d", food_list[i].count);
    lv_label_set_text( food_list[i].label_count, buf1);
    lv_obj_add_style( food_list[i].label_count, &style, LV_STATE_DEFAULT);

    //加按钮
    lv_obj_t* btn_add=lv_button_create(num_box);
    lv_obj_set_size(btn_add,30,30);
    lv_obj_t* label_add=lv_label_create(btn_add);
    lv_label_set_text(label_add,"+");
    lv_obj_set_size(label_add,20,20);
    lv_obj_set_align(label_add,LV_ALIGN_CENTER);
    lv_obj_add_style(label_add, &style, LV_STATE_DEFAULT);
    lv_obj_set_user_data(btn_add, (void*)1);
    lv_obj_add_event_cb(btn_add, num_change, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
    }

    total_label=lv_label_create(exe_container);
    lv_obj_align(total_label, LV_ALIGN_CENTER, 0, 0);
    char buf2[8];
    sprintf(buf2, "%d",price);
    lv_label_set_text(total_label, buf2);
    refresh(); // 初始化总价文字
    lv_obj_add_style(total_label, &style, LV_STATE_DEFAULT);

   
    back_btn();


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
            if(chat_container != NULL)
            {
                lv_obj_del(chat_container);
                chat_container = NULL;
            }

            // 3. 将所有聊天相关的全局控件指针置空，防止主循环访问野指针
            chat_label_show = NULL;
            chat_colum_cnotainer= NULL;
            chat_ta_input = NULL;
            chat_colum_cnotainer = NULL;  // 已经是野指针，置空即可

            // 4. 如果退出按钮容器是独立的（不在 chat_container 内），也删除
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


//聊天室界面
void chat_gui_create(lv_event_t *e)
{
    (void)e;
    if(main_obj!=NULL)
    {
        lv_obj_add_flag(main_obj,LV_OBJ_FLAG_HIDDEN);
    }

    
    chat_container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(chat_container, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(chat_container, LV_OBJ_FLAG_SCROLLABLE);


     static lv_style_t style;
    lv_style_init(&style);
    //设置标签内容的字体
    lv_style_set_text_font(&style, &lv_mygec_font);
    //设置标签内容的颜色
    lv_style_set_text_color(&style, lv_color_hex(0xFF0000));


    chat_colum_cnotainer=lv_obj_create(chat_container);
    lv_obj_set_size(chat_colum_cnotainer, LV_HOR_RES-40, 350);
    lv_obj_align(chat_colum_cnotainer,LV_ALIGN_TOP_MID,0,20);
    //聊天显示标签
    chat_label_show = lv_label_create(chat_colum_cnotainer);
    lv_label_set_long_mode(chat_label_show,LV_LABEL_LONG_WRAP);
    //lv_obj_set_size(chat_label_show, LV_HOR_RES-40, 350);
    //lv_obj_align(chat_label_show,LV_ALIGN_TOP_MID,0,20);
    lv_obj_add_style(chat_label_show, &style, LV_STATE_DEFAULT);
    lv_label_set_text(chat_label_show,"聊天:\n");

    lv_obj_set_flex_flow(chat_colum_cnotainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(chat_colum_cnotainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(chat_colum_cnotainer, LV_DIR_VER); // 垂直方向滚动
    lv_obj_add_flag(chat_colum_cnotainer, LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_ELASTIC);
  

    //输入框
     //创建一个键盘
    lv_obj_t * kb = lv_keyboard_create(lv_screen_active());
    chat_ta_input = lv_textarea_create(chat_container);
    lv_obj_set_size(chat_ta_input, 450,60);
    lv_obj_align(chat_ta_input,LV_ALIGN_CENTER,0,-20);
    lv_obj_add_style(chat_ta_input, &style, LV_STATE_DEFAULT);
    lv_textarea_set_placeholder_text(chat_ta_input,"send");
    //文本框添加事件   传递的是键盘控件
    lv_obj_add_event_cb(chat_ta_input, chat_event, LV_EVENT_ALL, kb);



    //发送按钮
    lv_obj_t *btn_send = lv_button_create(chat_container);
    lv_obj_set_size(btn_send,120,60);
    lv_obj_align_to(btn_send,chat_ta_input,LV_ALIGN_OUT_RIGHT_MID,10,0);
    lv_obj_t *lab_send = lv_label_create(btn_send);
    lv_label_set_text(lab_send,"发出");
    lv_obj_add_style(lab_send, &style, LV_STATE_DEFAULT);
    lv_obj_center(lab_send);
    lv_obj_add_event_cb(btn_send,chat_btn_send_cb,LV_EVENT_CLICKED,NULL);
    back_btn();

    //启动网络，用户名复用登录账号，这里写死，你也可以对接登录的账号
    chat_network_start("lvgl_user");
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


//主界面
void main_1()
{
    printf("进入主界面\n");
    //隐藏文本框
    lv_obj_add_flag(user,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(password,LV_OBJ_FLAG_HIDDEN);

    if(main_obj!=NULL)
    {
    lv_obj_remove_flag(main_obj,LV_OBJ_FLAG_HIDDEN);

    }
     static lv_style_t style;
    lv_style_init(&style);
    //设置标签内容的字体
    lv_style_set_text_font(&style, &lv_mygec_font);
    //设置标签内容的颜色
    lv_style_set_text_color(&style, lv_color_hex(0xFF0000));

    main_obj=lv_obj_create(lv_screen_active());
    lv_obj_set_size(main_obj, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(main_obj, LV_OBJ_FLAG_SCROLLABLE);
    //⭐新增，关闭对象默认内边距
    lv_obj_set_style_pad_all(main_obj,0,LV_PART_MAIN);

    //⭐【第一步】创建背景图片，父对象 login_container，放在最底层
    lv_obj_t *login_bg = lv_image_create(main_obj);
    //图片路径格式 A:/xxx/xxx.bmp
    lv_image_set_src(login_bg, "A:/mywork/8.bmp");
    //背景铺满整个容器
    lv_obj_set_size(login_bg, LV_HOR_RES, LV_VER_RES);
    //把背景送到最底层，所有其他控件画在背景上面
    lv_obj_move_background(login_bg);

    //相册设置按钮
     btn_12_photo=lv_button_create(main_obj);
    //设置按钮大小
    lv_obj_set_size(btn_12_photo,200,100);
    //设置按钮位置
    lv_obj_align(btn_12_photo,LV_ALIGN_TOP_LEFT,20,20);
    //给按钮设置标签
    lv_obj_t* label_12_ph=lv_label_create(btn_12_photo);
    //给标签设置位置
    lv_obj_set_align(label_12_ph,LV_ALIGN_CENTER);
    //给标签设置文字
    lv_label_set_text(label_12_ph,"相片");
    //把款式和标签联系在一起
     lv_obj_add_style(label_12_ph,&style,LV_STATE_DEFAULT);
     //进入相册界面
    lv_obj_add_event_cb(btn_12_photo,photo, LV_EVENT_CLICKED, label_12_ph);

     //点餐设置按钮
     btn_12_re=lv_button_create(main_obj);
    //设置按钮大小
    lv_obj_set_size(btn_12_re,200,100);
    //设置按钮位置
    lv_obj_align(btn_12_re,LV_ALIGN_TOP_MID,20,20);
    //给按钮设置标签
    lv_obj_t* label_12_re=lv_label_create(btn_12_re);
    //给标签设置位置
    lv_obj_set_align(label_12_re,LV_ALIGN_CENTER);
    //给标签设置文字
    lv_label_set_text(label_12_re,"点餐");
    //把款式和标签联系在一起
     lv_obj_add_style(label_12_re,&style,LV_STATE_DEFAULT);
      //进入点餐界面
    lv_obj_add_event_cb(btn_12_re,item, LV_EVENT_CLICKED, label_12_re);


    //音乐设置按钮
     btn_12_video=lv_button_create(main_obj);
    //设置音乐大小
    lv_obj_set_size(btn_12_video,200,100);
    //设置按钮位置
    lv_obj_align(btn_12_video,LV_ALIGN_TOP_RIGHT,-20,20);
    //给按钮设置标签
    lv_obj_t* label_12_video=lv_label_create(btn_12_video);
    //给标签设置位置
    lv_obj_set_align(label_12_video,LV_ALIGN_CENTER);
    //给标签设置文字
    lv_label_set_text(label_12_video,"music");
    //把款式和标签联系在一起
     lv_obj_add_style(label_12_video,&style,LV_STATE_DEFAULT);
      //进入音乐界面
    lv_obj_add_event_cb(btn_12_video,video, LV_EVENT_CLICKED, label_12_video);


     lv_obj_t* btn_chat=lv_button_create(main_obj);
    lv_obj_set_size(btn_chat,200,100);
    lv_obj_set_align(btn_chat,LV_ALIGN_CENTER);
    lv_obj_t* label_chat=lv_label_create(btn_chat);
    lv_obj_set_align(label_chat,LV_ALIGN_CENTER);
    lv_label_set_text(label_chat,"聊天室");
    lv_obj_add_style(label_chat,&style,LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn_chat,chat_gui_create, LV_EVENT_CLICKED, label_chat);

}
//账户密码检测
int word_test(const char* user,const char* password)
{
    for (int m=0;m<USER_COUNT;m++)
    {
        if((strcmp( user,user_list[m].username)==0 &&strcmp( password,user_list[m].password)==0))
        {
            return 1;
        }
        
    }
    return 0;
}
//账户密码输入
static void ta_event(lv_event_t* e)
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
        //键盘隐藏
        lv_obj_add_flag(kb,LV_OBJ_FLAG_HIDDEN);
        const char* user_text=lv_textarea_get_text(user);
        const char* password_text=lv_textarea_get_text(password);
            if(word_test(user_text,password_text))
            {
                printf("密码正确\n");
                main_1();
            }
            else
            {
                printf("密码错误\n");
            }

    }
}
//登录界面
void mytest12()
{
    //创建一个键盘
    lv_obj_t * kb = lv_keyboard_create(lv_screen_active());


  
    //登录文本框
    user=lv_textarea_create(lv_screen_active());
    //设置文本框位置
    lv_obj_align(user, LV_ALIGN_CENTER, 10, -200);
    //文本框添加事件   传递的是键盘控件
    lv_obj_add_event_cb(user, ta_event, LV_EVENT_ALL, kb);
    //设置文本框大小
    lv_obj_set_size(user,500,50);
    //账号悬浮
   lv_textarea_set_placeholder_text(user, "username");

    //密码文本框
    password=lv_textarea_create(lv_screen_active());
    //设置文本框位置
    lv_obj_align(password, LV_ALIGN_CENTER, 10, -50);
    //文本框添加事件   传递的是键盘控件
    lv_obj_add_event_cb(password, ta_event, LV_EVENT_ALL, kb);
     lv_obj_set_size(password,500,50);
     //密码悬浮
    lv_textarea_set_placeholder_text(password, "password");
    //密码遮掩
    lv_textarea_set_password_mode(password, true);
}




int main(void)
{
    lv_init();

    /*Linux frame buffer device init*/
    lv_display_t * disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, "/dev/fb0");

    lv_indev_t * indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event6");


    //mest();
    //my_Test2();
    mytest12();
    /*Create a Demo*/
    //lv_demo_widgets();
    //lv_demo_widgets_start_slideshow();

    /*Handle LVGL tasks*/
    while(1) {
        lv_timer_handler();

       pthread_mutex_lock(&chat_net_mutex);
if(chat_ui_msg_flag == 1)
{
    chat_ui_msg_flag = 0;
    char tmp_buf[CHAT_UI_BUF_MAX];
    strcpy(tmp_buf, chat_ui_buf);
    lv_obj_t * local_label = chat_label_show;
    pthread_mutex_unlock(&chat_net_mutex);

    if(local_label != NULL)
    {
        if( (strlen(chat_log_buf) + strlen(tmp_buf) + 2) < sizeof(chat_log_buf) )
        {
            strcat(chat_log_buf, tmp_buf);
            strcat(chat_log_buf, "\n");
            lv_label_set_text(local_label, chat_log_buf);
        }
    }
}
else
{
    pthread_mutex_unlock(&chat_net_mutex);
}
        usleep(5000);
    }

    return 0;
}
