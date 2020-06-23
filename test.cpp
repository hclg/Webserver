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
// #include "threadpool.h".


int main() {
    int epfd = epoll_create(5);
    epoll_event events;
    std::cout << events.events << " " << EPOLLRDHUP   ;
    std::cout << "ss" << std::endl;
    return 0;
}