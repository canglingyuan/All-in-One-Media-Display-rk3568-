#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include<errno.h>
#include<pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
int sockfd = -1; 
int confd=-1;
time_t now_time; //保存时间数据
int fd_txt=-1;
int client_fds[10] = {0}; //用来保存客户端的连接套接字描述符
int client_size = 3; //用来控制进入群聊的人数上限为3人
int fd=-1;
enum MSGTYPE //消息类型
{
    MSGDATA,   //普通消息
    MSGQUIT,    //退出消息
    MSGGET,     //下载
    MSGPUT,
    FILENAME,
    FILEDATA      //上传
};

// 封装read
int my_read(int fd, char* buf, int len)
{
    int readed_bytes = 0; //实际读取的字节数

    int ret = -1;
    while (1)
    {
        //读取指定长度的数据
        ret = read(fd, buf+readed_bytes, len-readed_bytes);

        //读错了
        if(ret == -1)
            return -1;

        //连接已经断开
        if(ret == 0)
            return readed_bytes;

        //记录成功读取到的字节数
        readed_bytes += ret;

        //已经读取到指定的字节数
        if(readed_bytes == len)
            return readed_bytes;
    }
    
}
void server_init(int port)
{
    // 1、创建通信结点(通信设备)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        perror("socket error");
        exit(1);
    }

    //设置套接字选项，允许地址重用
    int optval = 1;
    int r = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if(r == -1)
    {
        perror("setsockopt error");
        exit(1);
    }

    // 2、绑定(准备通信地址)
    struct sockaddr_in addr;
    addr.sin_family = AF_INET; //协议族
    addr.sin_port = htons(port); //端口号
   // addr.sin_addr.s_addr = inet_addr("192.168.176.185"); //本地的一个有效地址
   addr.sin_addr.s_addr = INADDR_ANY;//使用本地任意有效地址
    int ret = bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret == -1)
    {
        perror("bind error");
        exit(1);
    }
    
    // 3、监听
    ret = listen(sockfd, 10);
    if(ret == -1)
    {
        perror("listen error");
        exit(1);
    }
}




void* handle_connect(void* arg)
{
     //分离线程  结束后，系统自动回收该线程所有资源
    pthread_detach(pthread_self());
    //获取连接套接字描述符
    int confd = (long)arg;
    //接收客户端发送的数据
    //接收客户端发送的数据
    //消息结构： 消息头 + 消息体
    //消息头: 消息体长度(4字节) + 消息类型(4字节)
    int len = -1, type = -1;
    char head_buf[8] = {0}; //保存消息头的内容
    char buf[992] = {0}; //保存消息体的内容
    char send_buf[1000] = {0}; //保存需要发送的消息
    char send_txt[1000]={0};
    int ret;
    while(1)
    {
        len = -1;
        type = -1;
        memset(head_buf, 0, sizeof(head_buf)); //清空head_buf
        memset(buf, 0, sizeof(buf)); //清空buf
        memset(send_buf, 0, sizeof(send_buf)); //清空send_buf

        ret=my_read(confd,head_buf,8);
        if(ret<=0)
        {
            for(int i=0;i<client_size;i++)
            {
                if(confd==client_fds[i])
                {
                    client_fds[i]=0;
                    close(confd);
                    pthread_exit(NULL);//结束线程
                }
            }
        }
        sscanf(head_buf,"%04d%04d",&len,&type);
        printf("-------%d----------%d-----\n", len, type);

        if(type==MSGDATA)
        {
            ret=my_read(confd,buf,len);
            if(ret<=0)
            {
                for(int i=0;i<client_size;i++)
            {
                if(confd==client_fds[i])
                {
                    client_fds[i]=0;
                    close(confd);
                    pthread_exit(NULL);//结束线程
                }
            }
            }
             //进行消息的转发
            //消息头 + 消息体
            //拼装成真正需要发送的消息结构
            sprintf(send_buf, "%04d%04d%s", len, type, buf);

            char* txt_buf=send_buf+8;
            time(&now_time);
            sprintf(send_txt,"%s%s\n",ctime(&now_time),txt_buf);
            write(fd_txt,send_txt,strlen(send_txt));

            for(int i=0;i<3;i++)
            {
                printf("发送给 %d 的消息: %s\n", client_fds[i], send_buf);
                write(client_fds[i], send_buf, strlen(send_buf));
            }
        }
        else if(type==MSGQUIT)
        {
             ret=my_read(confd,buf,len);

             char* txt_buf=buf+8;
            time(&now_time);
            sprintf(send_txt,"%s%s\n",ctime(&now_time),txt_buf);
            write(fd_txt,send_txt,strlen(send_txt));

             sprintf(send_buf, "%04d%04d%s", len, type, buf);
             for(int i=0;i<client_size;i++)
            {
                printf("发送给 %d 的消息: %s\n", client_fds[i], send_buf);
                write(client_fds[i], send_buf, strlen(send_buf));
            }
             for(int i=0;i<client_size;i++)
            {
                if(confd==client_fds[i])
                {
                    client_fds[i]=0;
                    close(confd);
                    pthread_exit(NULL);//结束线程
                }
            }
        }
        else if(type==MSGGET)
        {
            char filename[100]={0};
            int file_len=strlen(filename);
            int file=FILENAME;
             ret=my_read(confd,buf,len);
             buf[ret] = '\0'; 
             printf("发送文件 %s\n",buf);
              fd=open(buf,O_RDONLY);
            if(fd < 0)
            {
                perror("open file fail");
                close(confd);
                break;
            }
            //不断地读取文件内容并发送到客户端
            char read_buf[992]={0};
            int ret2=-1;
            while((ret2 = read(fd, read_buf, sizeof(read_buf))) > 0)
            {
                memset(buf,0,sizeof(buf));
                //memset(read_buf,0,sizeof(read_buf));
                sprintf(buf, "%04d%04d", ret2, FILEDATA);
                memcpy(buf+8, read_buf, ret2);
                write(confd, buf, ret2+8);
            }
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "%04d%04d", 0, FILEDATA);
            write(confd, buf, 8);
            close(fd);
        }
        else if(type==MSGPUT)
        {
            char filename[100]={0};
            int file_len=strlen(filename);
            int file=FILENAME;
             ret=my_read(confd,buf,len);
             printf("接收文件 %s\n",buf);
            fd = open(buf, O_RDWR | O_CREAT | O_TRUNC, 0777);
            if(fd < 0)
            {
                perror("本地创建文件失败");
                continue;
            }
            while (1) 
            {
                // 读取下一个包头
                ret = my_read(confd, head_buf, 8);
                if (ret <= 0) 
                break;
                sscanf(head_buf, "%4d%4d", &len, &type);
                if (len == 0) 
                {
                    // 文件传输结束
                    printf("文件上传完成\n");
                    close(fd);
                    break;
                }
                ret = my_read(confd, buf, len);
                if (ret <= 0) break;
                write(fd, buf, ret);
                printf("写入 %d 字节\n", ret);
            }
        }
    }
}

int main(int argc, char const *argv[])
{
    
    //服务端初始化
    server_init(10086);
    int i;
    //接收连接
    struct sockaddr_in client_addr; //用来保存连接过来的客户端的地址信息
    socklen_t len = sizeof(client_addr);


     fd_txt = open("note.txt", O_RDWR | O_CREAT | O_TRUNC, 0777);
   if(fd_txt==-1)
   {
    perror("open txt error");
    exit(1);
   }

    while(1)
    {
        memset(&client_addr, 0, sizeof(client_addr));

        //不断的接收连接
        confd = accept(sockfd, (struct sockaddr*)&client_addr, &len);
        if(confd == -1)
        {
            perror("accept error");
            exit(1);
        }
        printf("new client: %s\n", inet_ntoa(client_addr.sin_addr));

        for( i=0;i<client_size;i++)
        {
            if(client_fds[i]==0)
            {
                client_fds[i]=confd;
            pthread_t pid;
            pthread_create(&pid, NULL, handle_connect, (void*)(long)confd);
           // pthread_detach(pid);
            break;
            }
        }
        if(i==client_size)
        {
             int type = MSGQUIT; //退出消息
            char* str = "聊天室人数已经达到上限";
            int len = strlen(str);
            
            //封装成需要发送的消息
            char buf[200] = {0};
            //消息头 + 消息体
            sprintf(buf, "%4d%4d%s", len, type, str);
            //发送信息到客户端
            write(confd, buf, len+8);
            sleep(2);
            close(confd);
        }
    }
    sleep(1);
    // 6、断开连接
    close(sockfd);
    close(confd);
    return 0;
}