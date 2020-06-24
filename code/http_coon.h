#ifndef _MYHTTP_COON_H
#define _MYHTTP_COON_H
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#define READ_BUF 2000

class http_coon {
public:
    /*
        NO_REQUESTION是代表请求不完整，需要客户继续输入； 
        GET_REQUESTION代表获得并且解析了一个正确的HTTP请求；
        BAD_REQUESTION是HTTP请求语法不正确； 404
        FORBIDDEN_REQUESTION是代表访问资源的权限有问题；forbidden
        FILE_REQUESTION代表GET方法资源请求；
        INTERNAL_ERROR代表服务器自身问题；
        NOT_FOUND代表请求的资源文件不存在；
        DYNAMIC_FILE表示是一个动态请求；
        POST_FILE表示获得一个以POST方式请求的HTTP请求
    */
    enum HTTP_CODE {//响应状态
        NO_REQUESTION,
        GET_REQUESTION,
        BAD_REQUESTION,
        FORBIDDEN_REQUESTION,
        FILE_REQUESTION,
        INTERNAL_ERROR,
        NOT_FOUND,
        DYNAMIC_FILE,
        POST_FILE
    };
    enum CHECK_STATUS {
        HEAD,
        REQUESTION
    };
private:
    char request_head_buf[1000];
    char read_bug[READ_BUF];
    char post_buf[2000];
    bool m_flag; /*是否是动态链接*/
public:
    int epfd;
    int client_sock;
    int read_pos;
    http_coon();
    ~http_coon();
    int init(int e_fd, int c_fd);
    int myread();
    bool mywrite();
    void doit();
    void close_coon();
    void modfd(int ev);

};

http_coon::http_coon() {
}

http_coon::~http_coon() {
}

int http_coon::init(int e_fd, int c_fd) {
    epfd = e_fd;
    client_sock = c_fd;
    read_pos = 0;
    m_flag = 0;
}

int http_coon::myread() {
    bzero(&read_bug, sizeof(read_bug));
    while (true) {
        int ret = recv(client_sock, read_bug+read_pos, READ_BUF-read_pos, 0);
        if (ret == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {/*对非阻塞socket而言，EAGAIN不是一种真正意义错误 
            一直再重复读 但是已经没数据了 所以是读完了*/ 
                break;
            }
            return 0;
        }
        else if (ret == 0) {/*网络断开连接了*/
            return 0;
        }
        else {
            read_pos += ret;
        }
    }
    strcpy(post_buf, read_bug);
    return 1;
}

#endif