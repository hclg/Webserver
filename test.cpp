 
#ifndef _MYHTTP_COON_H
#define _MYHTTP_COON_H
#include<iostream>
#include<stdio.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include<assert.h>
#include<sys/sendfile.h>
#include<sys/epoll.h>
#include<sys/fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
using namespace std;
#define READ_BUF 2000

int main() {
    int fd = open("data.txt", O_RDWR);
    if (fd == -1) {
        return -1;
    }
    int fd2 = dup2(fd, 1);
    printf("sdsd");
    
    return 0;
}

#endif