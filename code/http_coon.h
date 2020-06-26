#ifndef _HTTP_COON_H
#define _HTTP_COON_H
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
    char *m_host;
    char filename[200];
    bool m_flag; /*是否是动态链接*/
    int file_size; /*文件大小*/
    int check_index;
    int read_buf_len;
    CHECK_STATUS status;
    bool m_linger;
    int m_http_count;

public:
    int epfd;
    int client_sock;
    int read_pos;
    http_coon();
    ~http_coon();
    int init(int e_fd, int c_fd);
    int myread();
    bool mywrite();
    void do_it();
    void close_coon();
private:
    void modfd(int ev);
    void succeessful_respond();
    void bad_respond();
    void forbidden_respond();
    void not_found_request();
    void dynamic(char *argv);
    void post_respond();
    bool my_write();
    int judge_line(int &check_index, int &read_buf_len);

    HTTP_CODE requestion_analyse(char *temp);
    HTTP_CODE head_analyse(char *temp);
    HTTP_CODE analyse();
    HTTP_CODE do_get();
    HTTP_CODE do_post();
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

void http_coon::modfd(int ev) {
    epoll_event event;
    event.data.fd = client_sock;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epfd, EPOLL_CTL_MOD, client_sock, &event);
}

void http_coon::close_coon()
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, client_sock, 0);
    close(client_sock);
    client_sock = -1;
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

void http_coon::bad_respond() {//400
    bzero(url, sizeof(url));
    strcpy(url, path_400);
    bzero(filename, sizeof(filename));
    sprintf(filename, "../html/%s", url);
    struct stat my_file;
    if (stat(filename, &my_file) < 0) {
        std::cout << "bad_respond文件不存在" << std::endl;
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
    file_size = my_file.st_size;
    bzero(respond_head_buf, sizeof(respond_head_buf));
    sprintf(respond_head_buf, "HTTP/1.1 404 NOT_FOUND\r\nConnection: close\r\ncontent_length:%d\r\n\r\n", file_size);
}

void http_coon::dynamic(char *argv) { //动态请求处理
    int sum = 0;
    int numbers[2];
    m_flag = true;
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
    std::cout << read_buf << std::endl;
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
    std::cout << p << std::endl;
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
    p[0] = '\0';
    p++;
    std::cout << version << std::endl;
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

http_coon::HTTP_CODE http_coon::head_analyse(char *temp) {//解析头部
    if (temp[0] == '\0') {
        return GET_REQUESTION;
    }
    else if (strncasecmp(temp, "Connection:", 11) == 0) {
        temp = temp+11;
        while (*temp == ' ') {
            ++temp;
        }
        if (strcasecmp(temp, "keep-alive") == 0) {
            m_linger = true;
        }
    }
    else if (strncasecmp(temp, "Content-Length:", 15) == 0){
        temp = temp+15;
        while (*temp == ' ') {
            ++temp;
        }
        m_http_count = atoi(temp);
    }
    else if (strncasecmp(temp, "Host:", 5) == 0) {
        temp = temp+5;
        while (*temp == ' ') {
            ++temp;
        }
        m_host = temp;
    }
    else {
        std::cout << "can't handle it's hand\n";
    }
    return NO_REQUESTION;
}

http_coon::HTTP_CODE http_coon::do_get() {//GET 请求方式，对其解析
    char *ch;
    if (ch = strchr(url, '?')) {
        argv = ch+1;
        *ch = '\0';
        strcpy(filename, url);
        return DYNAMIC_FILE;
    }
    else {
        sprintf(filename, "../html%s", url);
        struct stat my_file;
        if (stat(filename, &my_file) < 0) {
            return NOT_FOUND; //找不到
        }
        if (!(my_file.st_mode & S_IROTH)) {//没有读的权限
            return FORBIDDEN_REQUESTION; //403
        }
        if (S_ISDIR(my_file.st_mode)) {
            return BAD_REQUESTION; //400
        }
        file_size = my_file.st_size;
        return FILE_REQUESTION;
    }
}


http_coon::HTTP_CODE http_coon::do_post() {//POST请求 ，分解存入参数
    int start = read_buf_len-m_http_count;
    sprintf(filename, "../html%s", url);
    argv = post_buf+start;//消息体的开头
    std::cout << argv << std::endl;
    argv[strlen(argv)+1] = '\0';
    if (filename != NULL && argv != NULL) {
        return POST_FILE;
    }
    return BAD_REQUESTION;
}

http_coon::HTTP_CODE http_coon::analyse() { // HTTP请求解析
    status = REQUESTION;
    check_index = 0;
    int start = 0;
    char *temp;
    read_buf_len = strlen(read_buf);
    int len = read_buf_len;
    while (judge_line(check_index, len)) {
        temp = read_buf+start;
        start = check_index;
        switch(status) {
            case REQUESTION: {
                std::cout << "requestion\n";
                int ret = requestion_analyse(temp);
                if (ret == BAD_REQUESTION) {
                    return BAD_REQUESTION;
                }
                break;
            }
            case HEAD: {
                int ret = head_analyse(temp);
                if (ret != GET_REQUESTION) { 
                    break;
                }
                if (strcasecmp(method, "GET") == 0) {
                    return do_get();
                }
                else if (strcasecmp(method, "POST") == 0) {
                    return do_post();
                }
                break;
            }
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUESTION;
}

void http_coon::do_it() {//线程取出的工作任务的接口函数
    http_coon::HTTP_CODE choice = analyse();//根据解析结果处理
    switch (choice)
    {
    case NO_REQUESTION: {
            std::cout << "NO_REQUESTION" << std::endl;
            modfd(EPOLLIN);
            break;
        }
    case FILE_REQUESTION: {//200
        std::cout << "文件 request" << std::endl;
        succeessful_respond();
        modfd(EPOLLOUT);
        break;
    }
    case BAD_REQUESTION: { //400
        std::cout << "BAD_REQUESTION" << std::endl;
        bad_respond();
        modfd(EPOLLOUT);
        break;
    }
    case FORBIDDEN_REQUESTION: { //403
        std::cout << "FORBIDDEN_REQUESTION" << std::endl;
        forbidden_respond();
        modfd(EPOLLOUT);
        break;
    }
    case NOT_FOUND: { //404
        std::cout << "not_found_request" << std::endl;
        not_found_request();
        modfd(EPOLLOUT);
        std::cout << "EPOLLOUT " << std::endl;
        break;
    }
    case DYNAMIC_FILE: {// 动态请求
        std::cout << "DYNAMIC_FILE" << std::endl;
        dynamic(argv);
        modfd(EPOLLOUT);
        break;
    }
    case POST_FILE: { //post 方法处理
        std::cout << "POST_FILE" << std::endl;
        post_respond();
        break;
    }
    default:
        close_coon();
        break;
    }
}

bool http_coon::mywrite() {
    if (m_flag) {
        int ret = send(client_sock, respond_head_buf, strlen(respond_head_buf), 0);
        int r = send(client_sock, body, strlen(body), 0);
        if (ret > 0 && r > 0) return true;
    }
    else {
        int fd = open(filename, O_RDONLY);
        assert(fd != -1);
        std::cout << "fd found" << filename <<  std::endl;
        int ret = write(client_sock, respond_head_buf, strlen(respond_head_buf));
        std::cout << "write->" << respond_head_buf <<  std::endl;
        if (ret < 0) {
            close(fd);
            return false;
        }
        ret = sendfile(client_sock, fd, NULL, file_size);
        read(fd, read_buf, file_size);
        if (ret < 0) {
            close(fd);
            return false;
        }
        close(fd);
        return true;
    }
    return false;
}


#endif