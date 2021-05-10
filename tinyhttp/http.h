/*
 * @Author: peng.cao
 * @Date: 2021-05-07 21:02:48
 * @LastEditTime: 2021-05-10 18:32:36
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
// 这两句话是在干什么？我是没搞懂的
#define ERROR_DIE(msg)  do              \
    {                                   \
        perror("[ERROR_DIE]"#msg);      \
        exit(1);                        \
    } while(0)

#define ERROR(msg)      do              \
    {                                   \
        perror("[ERROR]"#msg);          \
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
        m_query_(NULL), m_buffer_len_(0), m_http_(NULL) {}
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

    inline void setClientName(struct sockaddr_in &client)
    {
        m_client_name_ = client;
    }

    inline void setHttp(Http *h)
    {
        m_http_ = h;
    }

    inline Http* getHttpd()
    {
        return m_http_;
    }

    inline void close()
    {
        if(m_client_fd_ > 0)
        {
            ::close(m_client_fd_);
        }
    }

    inline void reset()
    {
        m_client_fd_ = 0;
        m_buffer_xi_ = 0;
        m_query_ = NULL;
        m_buffer_len_ = 0;
        m_http_ = NULL;
    }

    // 解析方法
    inline void parseMethod();

    // 解析query
    inline void parseUrl();

    // 解析header
    inline void parseHeader();

    // 获取content_length
    inline int getContentLength()
    {
        map<string, string>::iterator iter = m_header_.find("content-length");
        if(iter != m_header_.end())
        {
            LOG("content-length:%s", (iter->second).c_str());
            return atoi((iter->second).c_str());   //这里c_str()函数返回一个指向正规C字符串的指针常量，内容与本string串相同。
        }

        return 0;
    }

    inline bool isPOST()
    {
        if(strcasecmp(m_method_, "POST") == 0)  // strcasecmp(s1,s2)用忽略大小写比较字符串，此函数只在Linux中提供，
        {
            return true;
        }

        return false;
    }

    inline bool isGET()
    {
        if(strcasecmp(m_method_, "GET") == 0)
        {
            return true;
        }

        return false;
    }

    inline char* getUrl()
    {
        return m_url_;   //目前还不懂这个m_url_存放的是什么
    }

    virtual bool cgi()
    {
        if(strstr(m_url_, ".cgi") == NULL) // strstr()函数用来检索子串在字符串中首次出现的位置,返回字符串str中第一次出现子串substr的地址；如果没有检索到子串，则返回NULL。
        {
            return false;
        }

        return true;
    }

    virtual void error501();
    virtual void error500();
    virtual void error404();
    virtual void error400();
    virtual void serveFile(const char *path);
    virtual void executeCGI(const char *path);

    //这个discardBody()函数是干什么用的？？？
    int discardBody()
    {
        int len = 0, read_len = 0;
        while((len > 0) && strcmp("\n", m_buffer_))
        {
            len = getLine();
            read_len += len;
        }
        m_buffer_xi_ = 0;

        return read_len;
    }
    //这个的str传进来的是什么？是key和value混合的string吗？
    int split(const char* str, string &key, string &value)
    {
        int xi = 0;
        while (str[xi] != '\0' && isspace(str[xi]))
        {
            xi++;
        }
        // 先处理key
        while(str[xi] != '\0' && str[xi] != ':' && str[xi] != '\n')
        {
            key = key + (char)tolower(str[xi]);
            xi++;
        }

        if(str[xi] == ':' || str[xi] == '\n')
        {
            xi++;
        }

        while(str[xi] != '\0' && isspace(str[xi]))
            xi++;
        
        while(str[xi] != '\0' && str[xi] != '\n')
        {
            value = value + (char)tolower(str[xi]);
            xi++;
        }

        return xi + 1;
    }

    // 每次获取一行数据
    // 这里的数据 '\0' '\n' '\r'分别转义为了啥意思？
    int getLine()
    {
        int i = 0, n = 0;
        char c = '\0';

        while((i < MAX_BUF_SIZE - 1) && (c != '\n'))
        {
            n = ::recv(m_client_fd_, &c, 1, 0); //这里这个recv()函数的原型是什么？
            if(n > 0)
            {
                if(c == '\r')
                {
                    n = ::recv(m_client_fd_, &c, 1, MSG_PEEK); // ???
                    if((n > 0) && (c == '\n'))
                    {
                        ::recv(m_client_fd_, &c, 1, 0);
                    } else {
                        c = '\n';
                    }
                }
                m_buffer_[i] = c;
                i++;
            } else {
                c = '\n';
            }
        }
        m_buffer_[i] = '\0';
        //记录当前buf的大小
        m_buffer_len_ = i;
        LOG("read line : %s", m_buffer_);

        return i;
    }

private:
    int m_client_fd_;
    struct sockaddr_in m_client_name_;
    char *m_query_;
    size_t m_buffer_xi_, m_buffer_len_;
    char m_buffer_[MAX_BUF_SIZE], m_method_[255], m_url_[255]; // 这里的m_method存放的是访问的方式 如GET、POST

    map<string, string> m_header_; // 这里m_header_这个map存的是什么？？？不应该是用unordered_map吗？
    Http *m_http_;
};

typedef HttpSocket* HttpSocketptr;

class Http
{
 public:
    Http() : m_socket_fd_(0) {}
    ~Http()
    {
        // 清空queue ,这里不应该是 !m_queue_.empty()吗？？？
        while(m_queue_.empty())
        {
            HttpSocketptr o = m_queue_.front();
            m_queue_.pop();
            delete o;
        }
    }

    void startup(u_short port);

    void loop();

    HttpSocketptr newObject()
    {
        if(m_queue_.empty())
        {
            m_queue_.push(new HttpSocket());
        }

        HttpSocketptr o = m_queue_.front();
        m_queue_.pop();

        LOG("object:%p", o);

        return o;
    }

    void freeObject(HttpSocketptr o)
    {
        LOG("object :%p", o);

        if(o != NULL)
        {
            o->reset();
            m_queue_.push(o);
        }
    }

private:
    int m_socket_fd_;
    queue<HttpSocketptr> m_queue_;
};
