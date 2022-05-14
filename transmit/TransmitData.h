//
// Created by abc on 21-2-23.
//

#ifndef TEXTGDB_TRANSMITDATA_H
#define TEXTGDB_TRANSMITDATA_H

#include <sys/errno.h>
#include <zconf.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

struct TransmitReadInfo{
    int fd;                     //socket描述符
    int read_len;               //输入数据长度或设置接收数据长度
    int read_flag;              //输入标志
    int read_error;             //输入错误
    struct sockaddr_in *addr;   //输入地址
    char *input_data;           //输入数据结构
    char *memory_util;          //内存工具（动态或固定）
    char *transmit_util;        //传输工具
    int (*init_func)(struct TransmitReadInfo*);             //初始化输入函数（创建内存工具、设置释放函数）
    int (*create_func)(struct TransmitReadInfo*, int);      //申请内存函数
    int (*read_func)(struct TransmitReadInfo*, int, int);   //读取输入函数
    void (*transmit_func)(struct TransmitReadInfo*, int);   //传输函数（控制层）
    void (*uinit_func)(struct TransmitReadInfo*);           //释放函数
};

struct TransmitWriteInfo{
    int fd;                     //socket描述符
    int write_len;              //输出数据长度
    int alsend_len;             //已经输出长度
    int write_error;            //输出错误
    struct sockaddr_in *addr;   //输出错误
    char *write_buf;            //输出内存
};

typedef struct TransmitReadInfo TransmitReadInfo;
typedef struct TransmitWriteInfo TransmitWriteInfo;

int open_socket();
void close_socket(int);
int bind_socket(struct sockaddr_in*, int);
void connect_socket(struct sockaddr_in*, int);

void read_data(TransmitReadInfo*, int);
int read_data0(int, size_t, int, void*, struct sockaddr_in*, socklen_t*);
void write_data(TransmitWriteInfo*);

int set_recv_size(TransmitReadInfo*);
int set_send_size(TransmitWriteInfo*);

#endif //TEXTGDB_TRANSMITDATA_H
