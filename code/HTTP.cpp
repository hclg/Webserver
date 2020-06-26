#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/epoll.h>
#include "threadpool.h"

int post = 8000;
const int MAX_FD = 1000;
int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int edfd, int fd, bool flag) {//flag == 1 client flag == 0 server
    epoll_event ev;//保存发生事件文件描述符的结构体
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;//EPOLLREDHUP 是监听断开连接半关闭， ET 是边缘触发
    if (flag) {
        ev.events |= EPOLLONESHOT; //只触发一次
    }
    epoll_ctl(edfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);//非阻塞IO的设置
}

int main(int argc, char *argv[]) {
    if (argc == 2) post = atoi(argv[1]);
    threadpool<http_coon> *pool = new threadpool<http_coon>;
    http_coon *users = new http_coon[MAX_FD];
    assert(users);
    int ser_sock, cli_sock;
    struct sockaddr_in ser_addr, cli_addr;
    socklen_t cli_addr_len = sizeof(cli_addr);
    bzero(&ser_addr, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(post);
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    ser_sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(ser_sock != -1);

    int ret = bind(ser_sock, (struct sockaddr *) &ser_addr, sizeof(ser_addr));
    assert(ret != -1);
    
    ret = listen(ser_sock, 5);
    assert(ret != -1);

    epoll_event *events = new epoll_event[MAX_FD];
    int epfd = epoll_create(5);
    addfd(epfd, ser_sock, false);
    while (true) {
        int numbers = epoll_wait(epfd, events, MAX_FD, -1); // timeout == -1 表示一直到事件发生
        if (numbers < 0) {
            printf("my epoll is failure! %d\n", numbers);
            break;
        }
        for (int i = 0; i < numbers; ++i) {
            int sock = events[i].data.fd;

            if (sock == ser_sock) {
                cli_sock = accept(ser_sock, (struct sockaddr *) &cli_addr, &cli_addr_len);
                if (cli_sock < 0)  {
                    printf("errno is %d\n", errno);//errno 保存最近发生的错误
                    continue;
                }
                /*
                if(http_coon::m_user_count > MAX_FD)
                {
                    show_error(client_fd, "Internal sever busy");
                    continue;
                }
                */
                std::cout << epfd << " cli_sock:" << cli_sock <<  " ------" << std::endl;
                addfd(epfd, cli_sock, true);
                users[cli_sock].init(epfd, cli_sock);
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                users[sock].close_coon();//异常关闭客户端连接
            }
            else if (events[i].events & EPOLLIN) {
                if (users[sock].myread()) {
                    pool->addjob(users+sock);
                }
                else {
                    users[sock].close_coon();
                }
            }
            else if (events[i].events & EPOLLOUT) {
                if (!users[sock].mywrite()) {
                    users[sock].close_coon();
                }
            }
        }
    }
    close(epfd);
    close(ser_sock);
    delete [] users;
    delete pool;
    return 0;
}


