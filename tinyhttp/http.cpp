/*
 * @Author: your name
 * @Date: 2021-05-08 14:18:55
 * @LastEditTime: 2021-05-10 18:29:27
 * @LastEditors: Please set LastEditors
 * @Description: 
 * @FilePath: /QuickHTTP/tinyhttp/http.cpp
 */

#include <http.h>

//accept_request(int) 处理从套接字上监听到的一个http请求
static void* accept_request(void *arg)
{
    struct stat st; // stat结构体是用来描述一个linux系统文件中文件属性的结构
    HttpSocketptr socket = (HttpSocketptr)arg;
    socket->parseMethod();
    if(!socket->isPOST() && !socket->isGET())
    {
        socket->error501();
        return NULL;
    }

    // 解析URL
    socket->parseUrl();
    // 解析header
    socket->parseHeader();
    LOG("content-length: %d", socket->getContentLength);

    char path[512]; // 文件的相对路径
    snprintf(path, sizeof(path) - 1, "htdocs%s", socket->getUrl());
    // int snprintf(char *str, size_t size, const char *format, ...)
    // 设将可变参数(...)按照 format 格式化成字符串，并将字符串复制到 str 中，size 为要写入的字符的最大数目，超过 size 会被截断。
    if(path[strlen(path) - 1] == '/')
    {
        strcat(path, "index.html");
    }
    // 读取文件失败
    //函数原型：int stat(const char *path, struct stat *buf)， ​ 返回值：成功返回0，失败返回-1；作用是为了获取文件信息
    if(stat(path, &st) == -1)
    {
        socket->discardBody();
        socket->error404();
    } else {
        if((st.st_mode & S_IFMT) == S_IFDIR)
        {
            strcat(path, "/index.html");
        }
        LOG("cgi:%d, path:%s", socket->cgi(), path);
        // 不采用cgi
        if(!(socket->cgi()))
        {
            socket->serveFile(path); // 静态页面请求，直接返回文件信息给客户端，静态页面返回
        } else {
            socket->executeCGI(path); // 动态页面请求，执行cgi脚本
        }
    }
    socket->close();
    //释放对象
    socket->getHttpd()->freeObject(socket);
    return NULL;
}

void HttpSocket::serveFile(const char *path)
{
    FILE *resource = NULL;
    discardBody();

    resource = ::fopen(path, "r");
    LOG("path:%s", path);
    if(resource == NULL)
    {
        error404();
    } else {
        string s = string("HTTP/1.0 200 OK\r\n") +
                   SERVER_STRING +
                   "Content-Type: text/html\r\n" +
                   "\r\n";
        ::send(m_client_fd_, s.c_str(), strlen(s.c_str()), 0);
        while(!::feof(resource))
        {
            m_buffer_[0] = '\0';
            ::fgets(m_buffer_, sizeof(m_buffer_), resource);
            LOG('buffer:%s', m_buffer_);
            ::send(m_client_fd_, m_buffer_, strlen(m_buffer_), 0);
        }
    }

    ::fclose(resource);
}
// 运行cgi(公共网卡接口)脚本，需要设定合适的环境变量
/*
    executeCGI()负责将请求传递给cgi程序处理
    服务器与cgi之间通过管道pipe通信，首先初始化两个管道，并创建子进程执行cgi函数
    子进程执行cgi程序,获取cgi的标准输出 通过管道传给父进程，由父进程发送给客户端
*/
void HttpSocket::executeCGI(const char* path)
{
    int cgi_output[2], cgi_input[2];
    pid_t pid;
    int status;
    char c;

    string s = string("HTTP/1.0 200 OK\r\n") +
               SERVER_STRING +
               "Content-Type: text/html\r\n" +
               "\r\n";
    ::send(m_client_fd_, s.c_str(), strlen(s.c_str()), 0);
    //这里函数原型为int send(SOCKET sock, const char* buf, int len, int flags); 所以这里s 是缓冲区？需要自己加上报头HTTP/1.0 200 200返回成功？？？

    //pipe函数定义中的fd参数是一个大小为2的一个数组类型的指针。该函数成功时返回0，并将一对打开的文件描述符值填入fd参数指向的数组。失败时返回 -1并设置errno。
    // 通过pipe函数创建的这两个文件描述符 fd[0] 和 fd[1] 分别构成管道的两端
    if(pipe(cgi_output) < 0)
    {
        error500();
        return;
    }
    if(pipe(cgi_input) < 0)
    {
        error500();
        return;
    }

    if((pid = fork()) < 0)
    {
        error500();
        return;
    }

    int content_len = getContentLength();
    // 这是子进程， 运行cgi脚本
    if(pid == 0)
    {
        char meth_env[255], query_env[255], length_env[255]; //不清楚这些数组的含义

        ::dup2(cgi_output[1], 1);   // 1表示stdout, 0表示stdin，将系统标准输出重定向为cgi_output[1]
        ::dup2(cgi_input[0], 0);    // 将系统标准输入重定向为cgi_input[0]

        ::close(cgi_output[0]);     //关闭cgi_output中的out端
        ::close(cgi_input[1]);      //关闭cgi_input中的in端

        //cgi需要将请求的方法存储到环境变量中，然后和cgi脚本进行互动
        snprintf(meth_env, sizeof(meth_env) - 1, "REQUEST_METHOD=%s", m_method_);
        putenv(meth_env);
        // putenv()用来改变或增加环境变量的内容. 参数string 的格式为name＝value, 
        if(isGET())
        {   
            //设置query_string的环境变量
            sprintf(query_env, "QUERY_STRING=%s", m_query_);
            putenv(query_env);
        } else {
            //设置content_Length的环境变量
            sprintf(length_env, "CONTENT_LENGTH=%d", content_len);
            putenv(length_env);
        }
        execl(path, path, NULL);    //execl函数簇，执行cgi脚本，获取cgi的标准输出作为相应内容发送给客户端
        exit(0);
    } else {
        /*
            通过关闭对应管道的端口通道，然后重定向子进程的某端，这样就在父子进程之间构建了一条单双工通道
        */
        ::close(cgi_output[1]);
        ::close(cgi_input[0]);

        if(isPOST())
        {
            for (int i = 0; i < content_len; i++)
            {
                ::recv(m_client_fd_, &c, 1, 0);
                ::write(cgi_input[1], &c, 1); // 这里是要干嘛呀
            }
        }
        LOG("pid:%d, content_len:%d", pid, content_len);
        while(::read(cgi_output[0], &c, 1) > 0)
        {
            ::send(m_client_fd_, &c, 1, 0);
        }
        ::close(cgi_output[0]);
        ::close(cgi_input[1]);

        ::waitpid(pid, &status, 0);
        LOG("status:%d", status);
    }
}

// 解析方法
void HttpSocket::parseMethod()
{
    // 获取数据
    if(m_buffer_xi_ > m_buffer_len_ || m_buffer_len_ == 0)
    {
        getLine();
        m_buffer_xi_ = 0;

        LOG("body:%s", m_buffer_);
    }

    // 跳过空格
    while(isspace(m_buffer_[m_buffer_xi_]) && (m_buffer_xi_ < sizeof(m_buffer_)))
        m_buffer_xi_++;

    size_t xi = 0;
    while(!isspace(m_buffer_[m_buffer_xi_]) && (xi < sizeof(m_method_) - 1) && (m_buffer_xi_ < sizeof(m_buffer_)))
    {
        m_method_[xi] = m_buffer_[m_buffer_xi_];
        xi++;
        m_buffer_xi_++;
    }

    m_method_[m_buffer_xi_] = '\0';

    LOG("method:%s, m_buffer_xi_:%d, m_buffer_len_:%d",
        m_method_, m_buffer_xi_, m_buffer_len_);
}

void HttpSocket::parseUrl()
{
    // 获取数据
    if (m_buffer_xi_ >= m_buffer_len_ || m_buffer_len_ == 0)
    {
        getLine();
        m_buffer_xi_ = 0;

        LOG("body:%s", m_buffer_);
    }

    while (isspace(m_buffer_[m_buffer_xi_]) && (m_buffer_xi_ < sizeof(m_buffer_))) m_buffer_xi_++;

    size_t xi = 0;
    while (!isspace(m_buffer_[m_buffer_xi_]) && 
        (xi < sizeof(m_url_) - 1) && 
        (m_buffer_xi_ < sizeof(m_buffer_)))
    {
        m_url_[xi] = m_buffer_[m_buffer_xi_];
        xi++; 
        m_buffer_xi_++;
    }

    m_url_[xi] = '\0';

    if (strcasecmp(m_method_, "GET") == 0)
    {
        m_query_ = m_url_;
        while ((*m_query_ != '?') && (*m_query_ != '\0')) m_query_++;
        if (*m_query_ == '?')
        {
            *m_query_ = '\0';
            m_query_++;
        }
    }

    LOG("m_url_:%s, m_buffer_xi_:%d, m_buffer_len_:%d", 
        m_url_, m_buffer_xi_, m_buffer_len_);
    // 跳过解析协议
    m_buffer_xi_ = m_buffer_len_;
}

void HttpSocket::parseHeader()
{
    while (true)
    {
        // 获取数据
        if (m_buffer_xi_ >= m_buffer_len_ || m_buffer_len_ == 0)
        {
            getLine();
            m_buffer_xi_ = 0;

            LOG("body:%s, m_buffer_len_:%d", m_buffer_, m_buffer_len_);
        }

        while (isspace(m_buffer_[m_buffer_xi_]) && (m_buffer_xi_ < sizeof(m_buffer_))) m_buffer_xi_++;

        if (m_buffer_len_ <= 0)
        {
            break;
        }
        // int strcmp(const char *str1, const char *str2) 把 str1 所指向的字符串和 str2 所指向的字符串进行比较。
        if (strcmp("\r", m_buffer_) == 0 ||
            strcmp("\n", m_buffer_) == 0)
        {
            // header的解析终止
            break;
        }

        LOG("header:%s", m_buffer_);
        string s1, s2;
        m_buffer_xi_ = split(m_buffer_, s1, s2);
        if (m_buffer_xi_ > 1)
        {
            m_header_[s1] = s2;
        }

        LOG("m_buffer_xi_:%d, m_buffer_len_:%d", m_buffer_xi_, m_buffer_len_);
    }

    LOG("parseHeader end");
}

void HttpSocket::error501()
{
    string s = string("HTTP/1.0 501 Method Not Implemented\r\n") + 
        SERVER_STRING + 
        "Content-Type: text/html\r\n" +
        "\r\n" + 
        "<HTML><HEAD><TITLE>Method Not Implemented\r\n" +
        "</TITLE></HEAD>\r\n" +
        "<BODY><P>HTTP request method not supported.\r\n" +
        "</BODY></HTML>\r\n";

    ::send(m_client_fd_, s.c_str(), strlen(s.c_str()), 0);
}

void HttpSocket::error500()
{
    string s = string("HTTP/1.0 500 Internal Server Error\r\n") + 
        "Content-Type: text/html\r\n" +
        "\r\n" + 
        "<P>Error prohibited CGI execution.\r\n";

    ::send(m_client_fd_, s.c_str(), strlen(s.c_str()), 0);
}

void HttpSocket::error400()
{
    string s = string("HTTP/1.0 400 BAD REQUEST\r\n") + 
        "Content-type: text/html\r\n" +
        "\r\n" + 
        "<P>Your browser sent a bad request, " +
        "such as a POST without a Content-Length.\r\n";

    ::send(m_client_fd_, s.c_str(), strlen(s.c_str()), 0);
}

void HttpSocket::error404()
{
    string s = string("HTTP/1.0 404 NOT FOUND\r\n") + 
        SERVER_STRING + 
        "Content-type: text/html\r\n" +
        "\r\n" + 
        "<HTML><TITLE>Not Found</TITLE>\r\n" +
        "<BODY><P>The server could not fulfill\r\n" +
        "your request because the resource specified\r\n" +
        "is unavailable or nonexistent.\r\n" +
        "</BODY></HTML>\r\n";

    ::send(m_client_fd_, s.c_str(), strlen(s.c_str()), 0);
}

void Http::startup(u_short port)
{
    struct sockaddr_in name;

    m_socket_fd_ = socket(PF_INET, SOCK_STREAM, 0);
    if (m_socket_fd_ == -1)
    {
        ERROR_DIE("socket");
    }
    
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (::bind(m_socket_fd_, (struct sockaddr *)&name, sizeof(name)) < 0)
    {
        ERROR_DIE("bind");
    }
    
    if (port == 0)
    {
        socklen_t namelen = sizeof(name);
        if (getsockname(m_socket_fd_, (struct sockaddr *)&name, &namelen) == -1)
        {
            ERROR_DIE("getsockname");
        }
        port = ntohs(name.sin_port);
    }

    if (listen(m_socket_fd_, 5) < 0)
    {
        ERROR_DIE("listen");
    }

    // 执行循环处理
    loop();
}

void Http::loop()
{
    pthread_t pthread;

    while (true)
    {
        struct sockaddr_in client_name;
        socklen_t client_name_len = sizeof(client_name);

        int fd = accept(m_socket_fd_,
                        (struct sockaddr *)&client_name,
                        &client_name_len);
        if (fd == -1)
        {
            ERROR_DIE("accept");
        }

        HttpSocketptr s = newObject();
        s->setCientFd(fd);
        s->setClientName(client_name);
        s->setHttp(this);

        if (pthread_create(&pthread , NULL, accept_request, s) != 0)
        {
            ERROR("pthread_create");
        }
    }

    ::close(m_socket_fd_);
}