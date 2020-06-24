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
const char path_400[] =  "bad_respond.html";
const char path_403[] =  "forbidden_respond.html";
const char path_404[] =  "not_found_request.html";

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
    char respond_head_buf[1000]; /*响应头填充*/
    char read_buf[READ_BUF];
    char post_buf[1000];
    char *argv;
    char body[2000];
    // char path_400[17];
    // char path_403[23];
    // char path_404[40];
    char url[100];
    char *method;
    char *version;
    char filename[200];
    bool m_flag; /*是否是动态链接*/
    int file_size; /*文件大小*/
    int check_index;
    int read_buf_len;
    CHECK_STATUS status;


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
    void succeessful_respond();
    void bed_respond();
    void forbidden_respond();
    void not_found_request();
    void dynamic(char *argv);
    void post_respond();
    int judge_line(int &check_index, int &read_buf_len);

    HTTP_CODE requestion_analyse(char *temp);
    HTTP_CODE head_analyse(char *temp);
    HTTP_CODE analyse(char *temp);
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
    bzero(&read_buf, sizeof(read_buf));
    while (true) {
        int ret = recv(client_sock, read_buf+read_pos, READ_BUF-read_pos, 0);
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
    strcpy(post_buf, read_buf);
    return 1;
}

/*响应状态填充，这里返回可以不为bool类型*/
void http_coon::succeessful_respond() { //200
    m_flag = false;
    bzero(respond_head_buf, sizeof(respond_head_buf));
    sprintf(respond_head_buf, "HTTP/1.1 200 ok\r\nConnection: close\r\ncontent-length:%d\r\n\r\n", file_size);
}

void http_coon::bed_respond() {//400
    bzero(url, sizeof(url));
    strcpy(url, path_400);
    bzero(filename, sizeof(filename));
    sprintf(filename, "../html/%s", url);
    struct stat my_file;
    if (stat(filename, &my_file) < 0) {
        std::cout << "bed_respond文件不存在" << std::endl;
    }
    file_size = my_file.st_size;
    bzero(respond_head_buf, sizeof(respond_head_buf));
    sprintf(respond_head_buf, "HTTP/1.1 400 BAD_REQUSETION\r\nConnection: close\r\ncontent-length:%d\r\n\r\n", file_size);
}

void http_coon::forbidden_respond() {//403 //资源权限限制请求响应的填充
    bzero(url, sizeof(url));
    strcpy(url, path_403);
    bzero(filename, sizeof(filename));
    sprintf(filename, "../html/%s", url);
    struct stat my_file;
    if (stat(filename, &my_file) < 0) {
        std::cout << "forbidden_respond文件不存在" << std::endl;
    }
    file_size = my_file.st_size;
    bzero(respond_head_buf, sizeof(respond_head_buf));
    sprintf(respond_head_buf, "HTTP/1.1 403 FORBIDDEN\r\nConnection: close\r\ncontent-length:%d\r\n\r\n", file_size);
}

void http_coon::not_found_request() {//404
    bzero(url, sizeof(url));
    strcpy(url, path_404);
    bzero(filename, sizeof(filename));
    sprintf(filename, "../html/%s", url);
    struct stat my_file;
    if (stat(filename, &my_file) < 0) {
        std::cout << "not_found_request文件不存在" << std::endl;
    }
    bzero(respond_head_buf, sizeof(respond_head_buf));
    sprintf(respond_head_buf, "HTTP/1.1 404 NOT_FOUND\r\nConnection: close\r\ncontent_length:%d\r\n\r\n", file_size);
}

void http_coon::dynamic(char *argv) { //动态请求处理
    int sum = 0;
    int numbers[2];
    // bzero(respond_head_buf, sizeof(respond_head_buf));
    sscanf(argv, "a=%d&b=%d\n", &numbers[0], &numbers[1]);
    if (strcmp(filename, "/add") == 0) {
        sum = numbers[0] + numbers[1];
        sprintf(body, "<html><body>\r\n<p>%d + %d = %d</p><hr>\r\n</body></html>\r\n", numbers[0], numbers[1], sum);
        sprintf(respond_head_buf, "HTTP/1.1 200 ok\r\nConnection: close\r\ncontent-length: %d\r\n\r\n", file_size);
    }
    else if (strcmp(filename, "/mutiplication") == 0) {
        sum = numbers[0] * numbers[1];
        sprintf(body, "<html><body>\r\n<p>%d * %d = %d</p><hr>\r\n</body></html>\r\n", numbers[0], numbers[1], sum);
        sprintf(respond_head_buf, "HTTP/1.1 200 ok\r\nConnection: close\r\ncontent-length: %d\r\n\r\n", file_size);
    }
}

void http_coon::post_respond() { //post请求处理响应
    if (fork() == 0) {
        dup2(client_sock, STDOUT_FILENO);
        execl(filename, argv, NULL);
    }
    wait(NULL);
}

int http_coon::judge_line(int &check_index, int &read_buf_len) { //判断一行是否已经读取完整
    // std::cout << read_buf << std::endl;
    char ch;
    for (; check_index < read_buf_len; ++check_index) {
        ch = read_buf[check_index];
        if (ch == '\r' && check_index+1 == read_buf_len) {
            return 0;
        }
        if (ch == '\r' && read_buf[check_index+1] == '\n') {
            read_buf[check_index++] = '\0';
            read_buf[check_index++] = '\0';
            return 1;
        }
        if (ch == '\n') {
            return 0;
        }
    }
    return 0;
}   

http_coon::HTTP_CODE http_coon::requestion_analyse(char *temp) { //请求行解析
    char *p = temp;
    // std::cout << p << std::endl;
    method = p;
    while (*p != ' ' && *p != '\r') {
        ++p;
    }
    *p = '\0';
    ++p;
    int pos = 0;
    while (*p != ' ' && *p != '\r') {
        url[pos++] = *p;
        ++p;
    }
    url[pos++] = *p = '\0';
    ++p;
    version = p;
    while (*p != ' ' && *p != '\0' && *p != '\r') {
        ++p;
    }
    // std::cout << version << std::endl;
    if (strcasecmp(method, "GET") != 0 && strcasecmp(method, "POST") != 0) {
        return BAD_REQUESTION;
    }
    if (!url || url[0] != '/') {
        return BAD_REQUESTION;
    }
    if (strcasecmp(version, "HTTP/1.1") != 0) {
        return BAD_REQUESTION;
    }
    status = HEAD;
    return NO_REQUESTION;
}


#endif