/*
 * @Author: peng.cao
 * @Date: 2021-05-07 21:02:48
 * @LastEditTime: 2021-05-07 22:58:43
 * @LastEditors: Please set LastEditors
 * @Description: include http
 * @FilePath: /tinyhttp/http.h
 */

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <queue>
#include <string>
#include <map>

using namespace std;

#define SERVER_STRING "Server: http++/1.0.0\r\n"
#define ERROR_DIE(msg) do 
    {
        perror("[ERROR_DIE]" #msg);   //这里我没有完全看懂
    } while(0)

#define ERROR(msg) do
    {
        perror("[ERROR]" #msg);
    } while(0)

#ifdef DEBUG
#define LOG(fmt, ...) fprint(stdout, "[%s][%u]" fmt "\n" , __FILE__, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#endif

#define MAX_BUF_SIZE 1024

class Http;

class HttpSocket
{
 public:
    HttpSocket() : m_client_fd_(0), m_buffer_xi_(0),
        m_query_(NULL), m_buffer_len_(0), m_httpd_(NULL) {}
    HttpSocket(int fd, struct sockaddr_in &s) : m_client_fd_(fd),
        m_client_name_(s), m_buffer_xi_(0), m_query_(NULL),
        m_buffer_len_(0), m_http_(NULL) {}

    virtual ~HttpSocket()
    {
        reset();
    }

    inline void setCientFd(int fd)
    {
        m_client_fd_ = fd;
    }

 private:
    int m_client_fd_;
    struct sockaddr_in m_client_name_;
    char *m_query_;
    size_t m_buffer_xi_, m_buffer_len_;
    char m_buffer_[MAX_BUF_SIZE], m_method_[255], m_url_[255];

    map<string, string> m_header_; // 这里m_header_这个map存的是什么？？？不应该是用unordered_map吗？
    Http *m_http_;
}
