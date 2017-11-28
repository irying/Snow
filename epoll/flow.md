

#### 1.**int epoll_create(int size)**

创建一个epoll句柄，参数size用来告诉内核监听的数目，size为epoll所支持的最大句柄数

#### 2.int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)

函数功能： epoll事件注册函数
参数**epfd为epoll的句柄，即epoll_create返回值**
参数op表示动作，用3个宏来表示：  

EPOLL_CTL_ADD(注册新的fd到epfd)， 
EPOLL_CTL_MOD(修改已经注册的fd的监听事件)，
EPOLL_CTL_DEL(从epfd删除一个fd)；
其中参数fd为需要监听的标示符；
参数event告诉内核需要监听的事件，event的结构如下：

```
struct epoll_event {
    _uint32t events; //Epoll events
    epoll_data_t data; //User data variable
};
```

**events是宏的集合，主要使用EPOLLIN**(表示对应的文件描述符可以读，即读事件发生)



#### 3.int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout)

等待事件的产生，函数返回需要处理的事件数目（该数目是就绪事件的数目，就是前面所说漂亮女孩的个数N）





#### utility.h

1.服务端存储所有在线用户socket, 便于广播信息
`list<int> clients_list;`

2.服务器ip地址，为测试使用本地机地址，可以更改为其他服务端地址

``define SERVER_IP "127.0.0.1"``

3.服务器端口号

`define SERVER_PORT 8888`

4.int epoll_create(int size)中的size，为epoll支持的最大句柄数

`define EPOLL_SIZE 5000`

5.缓冲区大小65535

`define BUF_SIZE 0xFFFF`

6.一些宏

``define SERVER_WELCOME "Welcome you join to the chat room! Your chat ID is: Client #%d"``

`define SERVER_MESSAGE "ClientID %d say >> %s"`

`define EXIT "EXIT"`

`define CAUTION "There is only one int the char room!"`

7.一些函数 

`int setnonblocking(int sockfd)`；// 设置非阻塞
`void addfd( int epollfd, int fd, bool enable_et )；`//将文件描述符fd添加到epollfd标示的内核事件表
`int sendBroadcastmessage(int clientfd)；`//服务端发送广播信息，使所有用户都能收到消息



##### Client

通过调用 int pipe(int fd[2]) 函数创建管道, 其中 fd[0] 用于父进程读， fd[1] 用于子进程写。

```
//client.cpp代码（管道模块）
   // 创建管道.
    int pipe_fd[2];
    if(pipe(pipe_fd) < 0) { perror("pipe error"); exit(-1); }

```

通过 int pid = fork() 函数，创建子进程，当 pid < 0 错误；当 pid = 0, 说明是子进程；当 pid > 0 说明是父进程。根据 pid 的值，我们可以父子进程，从而实现对应的功能！