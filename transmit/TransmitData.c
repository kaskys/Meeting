//
// Created by abc on 21-2-23.
//

#include "TransmitData.h"

/**
 * 打开socket
 * @return  socket描述符
 */
int open_socket(){
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    return (fd <= 0) ? 0 : fd;
}

/**
 * 关闭socket
 * @param fd  socket描述符
 */
void close_socket(int fd){
    if(fd > 0) {
        close(fd);
    }
}

/**
 * 绑定socket地址
 * @param addr  地址
 * @param fd    socket描述符
 * @return
 */
int bind_socket(struct sockaddr_in *addr, int fd){
    int opt_len = sizeof(struct sockaddr_in);
    //判断是否打开socket
    if(fd > 0){
        //设置参数
        addr->sin_family = AF_INET;
        if(((addr->sin_port > 0) && (addr->sin_port < IPPORT_RESERVED))
                                    || (addr->sin_addr.s_addr == INADDR_ANY) || (addr->sin_addr.s_addr == INADDR_NONE)){
            return 1;
        }
restart:
        //绑定地址或已经绑定
        if(bind(fd, addr, (socklen_t)opt_len)){
            if(errno == EADDRINUSE){
                addr->sin_port = 0;
                goto restart;
            }
            //绑定成功
            return 1;
        }
        //是否已经绑定地址？
        return getsockname(fd, addr, (socklen_t*)&opt_len);
    }else{
        return 1;
    }
}

/**
 * 链接socket地址
 * @param addr  地址
 * @param fd    socket描述符
 */
void connect_socket(struct sockaddr_in *addr, int fd){
    if(fd > 0){
        connect(fd, addr, 0);
    }
}

/**
 * 读取输入数据
 * @param read_info         读取数据结构
 * @param default_read_size 输入长度
 */
void read_data(TransmitReadInfo *read_info, int default_read_size){
    int addr_len = 0, read_result = 0, relen = 0;;

    /*
     * 判断参数
     * 1.没有读取数据结构或没有打开socket
     * 2.没有读取函数
     * 3.没有获取输入数据长度
     * 4.获取输入错误
     */
    if(!read_info || (read_info->fd <= 0)){
        return;
    }

    if(!read_info->init_func || !read_info->create_func || !read_info->read_func || !read_info->transmit_func){
        return;
    }

    if(ioctl(read_info->fd, FIONREAD, &read_info->read_len) < 0){
        read_info->read_len = default_read_size;
    }

    if(read_info->read_len <= 0){
        read_info->read_flag = MSG_DONTWAIT;
        if(recvfrom(read_info->fd, NULL, 0, read_info->read_flag, (struct sockaddr*)read_info->addr, ((socklen_t*)(&addr_len))) < 0){
            read_info->read_error = errno;
        }
        return;
    }

    //初始化数据输入
    if(!(*read_info->init_func)(read_info)){
        return;
    }

    //复位输入标志
    read_info->read_flag = 0;
    for(int alread_len = 0; alread_len < read_info->read_len; alread_len += read_result){
        //根据输入数据长度申请内存（中心化或去中心化:动态、非中心化：固定）
        if(!(*read_info->create_func)(read_info, read_info->read_len - alread_len)){
            return;
        }

        //复位剩余数据长度及输入地址
        relen = 0;
        memset(read_info->addr, 0, (addr_len = sizeof(struct sockaddr_in)));

        //数据输入函数
        read_result = read_info->read_func(read_info, alread_len, addr_len);

        if(read_result < 0){
            if((read_info->read_error = errno) == EINVAL){
                break;
            }
            continue;
        }else if(read_result > 0){
            (*read_info->transmit_func)(read_info, read_result);
        }else{
            if((ioctl(read_info->fd, FIONREAD, &relen) < 0) || (relen <= 0)){
                break;
            }
        }
    }

    read_info->uinit_func(read_info);
}

/**
 * 数据输入实现函数
 * @param fd        socket描述符
 * @param len       内存长度
 * @param flag      标志
 * @param buffer    内存
 * @param addr      地址
 * @param addr_len  地址长度
 * @return
 */
int read_data0(int fd, size_t len, int flag, void *buffer, struct sockaddr_in *addr, socklen_t *addr_len){
    return recvfrom(fd, buffer, len, flag, addr, &addr_len);
}

/**
 * 发送输出数据
 * @param write_info    输出数据结构
 */
void write_data(TransmitWriteInfo *write_info){
    int write_flag = 0;

    /*
     * 判断参数
     * 1.没有输出数据结构或没有打开socket或内有输出内存
     * 2.地址参数不正确
     */
    if(!write_info || (write_info->fd <= 0) || !write_info->write_buf){
        return;
    }

    if(!write_info->addr || (write_info->addr->sin_port < IPPORT_RESERVED)
       || (write_info->addr->sin_addr.s_addr == INADDR_ANY) || (write_info->addr->sin_addr.s_addr == INADDR_NONE)){
        return;
    }

    //发送数据
    write_info->alsend_len = (int)sendto(write_info->fd, write_info->write_buf, (size_t)write_info->write_len, write_flag,
                                         (const struct sockaddr_in*)write_info->addr, sizeof(struct sockaddr_in));
    //判断是否发送成功？
    if(write_info->alsend_len < 0){
        write_info->alsend_len = 0;
        write_info->write_error = errno;
    }
}

/**
 * 设置socket接收缓存长度
 * @param info
 * @return
 */
int set_recv_size(TransmitReadInfo *info){
    int recv_size = 0, default_size = 0, opt_size = sizeof(int);

    if(!info || (info->fd <= 0) || (recv_size <= 0)){
        return 0;
    }

    recv_size = info->read_len;
    if(getsockopt(info->fd, SOL_SOCKET, SO_RCVBUF, (socklen_t)default_size, (socklen_t*)&opt_size) < 0){
        return 0;
    }

    if(default_size < recv_size){
        if(setsockopt(info->fd, SOL_SOCKET, SO_RCVBUF, (socklen_t)recv_size, opt_size) < 0){
            info->read_len = default_size;
        }else {
            info->read_len = recv_size;
        }
    }
    return 1;
}

/**
 * 设置socket发送缓存长度
 * @param info
 * @return
 */
int set_send_size(TransmitWriteInfo *info){
    int send_size = 0, default_size = 0, opt_size = sizeof(int);

    if(!info || (info->fd <= 0) || (send_size <= 0)){
        return 0;
    }

    send_size = info->write_len;
    if(getsockopt(info->fd, SOL_SOCKET, SO_SNDBUF, (socklen_t)default_size, (socklen_t*)&opt_size) < 0){
        return 0;
    }

    if(default_size < send_size){
        if(setsockopt(info->fd, SOL_SOCKET, SO_SNDBUF, (socklen_t)send_size, opt_size) < 0){
            info->write_len = default_size;
        }else{
            info->write_len = send_size;
        }
    }
    return 1;
}