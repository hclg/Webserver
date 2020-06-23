#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
int port = 4040;

int main(int argc, char *argv[]) {

    if (argc == 2) port = atoi(argv[1]);
    int sock;
    int connfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = PF_INET;
    server_addr.sin_port = htons(port);

    int ret = bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr));
    assert(ret != -1);
    ret = listen(sock, 1);
    assert(ret != -1);
    while (1) {
        connfd = accept(sock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (connfd < 0) printf("errno\n");
        else {
            char request[1024];
            recv(connfd, request, 1024, 0);
            request[strlen(request)+1] = '\0';
            printf("%s\n", request);
            printf("successeful!\n");
            char buf[520] = "HTTP/1.1 200 OK\r\nconnection: close\r\n\r\n";//HTTP响应
            int s = send(connfd, buf, strlen(buf), 0);
            int fd = open("hello.html", O_RDONLY);//消息体只读
            sendfile(connfd,fd,NULL, 2500);
            close(fd);
            close(connfd);
        }
    }
    close(sock);
    return 0;
}