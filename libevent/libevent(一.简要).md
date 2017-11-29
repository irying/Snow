笔记来源：http://blog.csdn.net/sparkliang/article/category/660506

#### Reactor

从Reactor模式讲起，Reactor由下面4个关键词组成。

![](http://my.csdn.net/uploads/201207/19/1342687408_4917.jpg)

1） 事件源
Linux上是文件描述符，Windows上就是Socket或者Handle了，这里统一称为“句柄集”；程序在指定的句柄上注册关心的事件，比如I/O事件。

2） event demultiplexer——事件多路分发机制
由操作系统提供的I/O多路复用机制，比如select和epoll。
**程序首先将其关心的句柄（事件源）及其事件注册到event demultiplexer上；**

**当有事件到达时，event demultiplexer会发出通知“在已经注册的句柄集中，一个或多个句柄的事件已经就绪”；**

**程序收到通知后，就可以在非阻塞的情况下对事件进行处理了。**
对应到libevent中，依然是select、poll、epoll等，但是libevent使用结构体eventop进行了封装，以统一的接口来支持这些I/O多路复用机制，达到了对外隐藏底层系统机制的目的。

3） Reactor——反应器
Reactor，是事件管理的接口，**内部使用event demultiplexer注册、注销事件；并运行事件循环，当有事件进入“就绪”状态时，调用注册事件的回调函数处理事件。**
对应到libevent中，就是event_base结构体。

```c++
class Reactor  
{  
public:  
    int register_handler(Event_Handler *pHandler, int event);  
    int remove_handler(Event_Handler *pHandler, int event);  
    void handle_events(timeval *ptv);  
    // ...  
};  
```



4） Event Handler——事件处理程序
**事件处理程序提供了一组接口，每个接口对应了一种类型的事件，供Reactor在相应的事件发生时调用，执行相应的事件处理。通常它会绑定一个有效的句柄。**
对应到libevent中，就是event结构体。



下面是两种典型的Event Handler类声明方式，二者互有优缺点。

```c++
class Event_Handler  
{  
public:  
    virtual void handle_read() = 0;  
    virtual void handle_write() = 0;  
    virtual void handle_timeout() = 0;  
    virtual void handle_close() = 0;  
    virtual HANDLE get_handle() = 0;  
    // ...  
};  
class Event_Handler  
{  
public:  
    // events maybe read/write/timeout/close .etc  
    virtual void handle_events(int events) = 0;  
    virtual HANDLE get_handle() = 0;  
    // ...  
};  
```

流程

![](http://p.blog.csdn.net/images/p_blog_csdn_net/sparkliang/EntryImages/20091207/sequence633957998327656250.JPG)



#### 几个数据结构

##### 1.Event

Libevent是基于事件驱动（event-driven）的，从名字也可以看到event是整个库的核心。**event就是Reactor框架中的事件处理程序组件；它提供了函数接口，供Reactor在事件发生时调用，以执行相应的事件处理，通常它会绑定一个有效的句柄。**

```C
struct event {  
 TAILQ_ENTRY (event) ev_next;  
 TAILQ_ENTRY (event) ev_active_next;  
 TAILQ_ENTRY (event) ev_signal_next;  
 unsigned int min_heap_idx; /* for managing timeouts */  
 struct event_base *ev_base;  
 int ev_fd;  
 short ev_events;  
 short ev_ncalls;  
 short *ev_pncalls; /* Allows deletes in callback */  
 struct timeval ev_timeout;  
 int ev_pri;  /* smaller numbers are higher priority */  
 void (*ev_callback)(int, short, void *arg);  
 void *ev_arg;  
 int ev_res;  /* result passed to event callback */  
 int ev_flags;  
};  
```

事件类型可以使用“|”运算符进行组合，需要说明的是，信号和I/O事件不能同时设置；可以看出libevent使用event结构体将这[3种事件的处理统一起来]()；

```
ev_events：event关注的事件类型，它可以是以下3种类型：
I/O事件： EV_WRITE和EV_READ
定时事件：EV_TIMEOUT
信号：    EV_SIGNAL
辅助选项：EV_PERSIST，表明是一个永久事件
```

```C
#define EV_TIMEOUT 0x01  
#define EV_READ  0x02  
#define EV_WRITE 0x04  
#define EV_SIGNAL 0x08  
#define EV_PERSIST 0x10 /* Persistant event */  
```



##### 2.Event_base，reactor，事件处理框架

```C
struct event_base {  
 const struct eventop *evsel;  
 void *evbase;　  
 int event_count;  /* counts number of total events */  
 int event_count_active; /* counts number of active events */  
 int event_gotterm;  /* Set to terminate loop */  
 int event_break;  /* Set to terminate loop immediately */  
 /* active event management */  
 struct event_list **activequeues;  
 int nactivequeues;  
 /* signal handling info */  
 struct evsignal_info sig;  
 struct event_list eventqueue;  
 struct timeval event_tv;  
 struct min_heap timeheap;  
 struct timeval tv_cache;  
};  
```

1）evsel和evbase这两个字段的设置可能会让人有些迷惑，这里你可以把evsel和evbase看作是类和静态函数的关系，比如添加事件时的调用行为：**evsel->add(evbase, ev)，实际执行操作的是evbase；这相当于class::add(instance, ev)，instance就是class的一个对象实例。**

**evsel指向了全局变量static const struct eventop *eventops[]中的一个；**
前面也说过，libevent将系统提供的I/O demultiplex机制统一封装成了eventop结构；因此eventops[]包含了select、poll、kequeue和epoll等等其中的若干个全局实例对象。
**evbase实际上是一个eventop实例对象；**

```C
看看eventop结构体，它的成员是一系列的函数指针, 在event-internal.h文件中：
struct eventop {
 const char *name;
 void *(*init)(struct event_base *); // 初始化
 int (*add)(void *, struct event *); // 注册事件
 int (*del)(void *, struct event *); // 删除事件
 int (*dispatch)(struct event_base *, void *, struct timeval *); // 事件分发
 void (*dealloc)(struct event_base *, void *); // 注销，释放资源
 /* set if we need to reinitialize the event base */
 int need_reinit;
};
```

```C
2）activequeues是一个二级指针，前面讲过libevent支持事件优先级，因此你可以把它看作是数组，其中的元素activequeues[priority]是一个链表，链表的每个节点指向一个优先级为priority的就绪事件event。 
 
3）eventqueue，链表，保存了所有的注册事件event的指针。
4）sig是由来管理信号的结构体，将在后面信号处理时专门讲解；
5）timeheap是管理定时事件的小根堆，将在后面定时事件处理时专门讲解；
6）event_tv和tv_cache是libevent用于时间管理的变量，将在后面讲到；
```



创建一个event_base对象也既是创建了一个新的libevent实例，程序需要通过调用**<u>event_init()（内部调用event_base_new函数执行具体操作）函数来创建，该函数同时还对新生成的libevent实例进行了初始化。</u>**

**该函数首先为event_base实例申请空间，然后初始化timer mini-heap，选择并初始化合适的系统I/O 的demultiplexer机制，初始化各事件链表；**
函数还检测了系统的时间设置，为后面的时间管理打下基础。



Reactor接口

```
int  event_add(struct event *ev, const struct timeval *timeout);  
int  event_del(struct event *ev);  
int  event_base_loop(struct event_base *base, int loops);  
void event_active(struct event *event, int res, short events);  
void event_process_active(struct event_base *base); 
```

```
1）注册事件
函数原型：
int event_add(struct event *ev, const struct timeval *tv)
参数：ev：指向要注册的事件；
tv：超时时间；
函数将ev注册到ev->ev_base上，事件类型由ev->ev_events指明，如果注册成功，ev将被插入到已注册链表中；如果tv不是NULL，则会同时注册定时事件，将ev添加到timer堆上；
如果其中有一步操作失败，那么函数保证没有事件会被注册，可以讲这相当于一个原子操作。这个函数也体现了libevent细节之处的巧妙设计，且仔细看程序代码，部分有省略，注释直接附在代码中。
```

```
2）删除事件：
函数原型为：int  event_del(struct event *ev);
该函数将删除事件ev，对于I/O事件，从I/O 的demultiplexer上将事件注销；对于Signal事件，将从Signal事件链表中删除；对于定时事件，将从堆上删除；
同样删除事件的操作则不一定是原子的，比如删除时间事件之后，有可能从系统I/O机制中注销会失败。
```



#### 事件处理主循环

Libevent的事件主循环主要是通过event_base_loop ()函数完成的，其主要操作如下面的流程图所示，event_base_loop所作的就是持续执行下面的循环。

![](http://p.blog.csdn.net/images/p_blog_csdn_net/sparkliang/EntryImages/20091211/main-loop.JPG)

```c++
int event_base_loop(struct event_base *base, int flags)  
{  
    const struct eventop *evsel = base->evsel;  
    void *evbase = base->evbase;  
    struct timeval tv;  
    struct timeval *tv_p;  
    int res, done;  
    // 清空时间缓存  
    base->tv_cache.tv_sec = 0;  
    // evsignal_base是全局变量，在处理signal时，用于指名signal所属的event_base实例  
    if (base->sig.ev_signal_added)  
        evsignal_base = base;  
    done = 0;  
    while (!done) { // 事件主循环  
        // 查看是否需要跳出循环，程序可以调用event_loopexit_cb()设置event_gotterm标记  
        // 调用event_base_loopbreak()设置event_break标记  
        if (base->event_gotterm) {  
            base->event_gotterm = 0;  
            break;  
        }  
        if (base->event_break) {  
            base->event_break = 0;  
            break;  
        }  
        // 校正系统时间，如果系统使用的是非MONOTONIC时间，用户可能会向后调整了系统时间  
        // 在timeout_correct函数里，比较last wait time和当前时间，如果当前时间< last wait time  
        // 表明时间有问题，这是需要更新timer_heap中所有定时事件的超时时间。  
        timeout_correct(base, &tv);  
     
        // 根据timer heap中事件的最小超时时间，计算系统I/O demultiplexer的最大等待时间  
        tv_p = &tv;  
        if (!base->event_count_active && !(flags & EVLOOP_NONBLOCK)) {  
            timeout_next(base, &tv_p);  
        } else {  
            // 依然有未处理的就绪时间，就让I/O demultiplexer立即返回，不必等待  
            // 下面会提到，在libevent中，低优先级的就绪事件可能不能立即被处理  
            evutil_timerclear(&tv);  
        }  
        // 如果当前没有注册事件，就退出  
        if (!event_haveevents(base)) {  
            event_debug(("%s: no events registered.", __func__));  
            return (1);  
        }  
        // 更新last wait time，并清空time cache  
        gettime(base, &base->event_tv);  
        base->tv_cache.tv_sec = 0;  
        // 调用系统I/O demultiplexer等待就绪I/O events，可能是epoll_wait，或者select等；  
        // 在evsel->dispatch()中，会把就绪signal event、I/O event插入到激活链表中  
        res = evsel->dispatch(base, evbase, tv_p);  
        if (res == -1)  
            return (-1);  
        // 将time cache赋值为当前系统时间  
        gettime(base, &base->tv_cache);  
        // 检查heap中的timer events，将就绪的timer event从heap上删除，并插入到激活链表中  
        timeout_process(base);  
        // 调用event_process_active()处理激活链表中的就绪event，调用其回调函数执行事件处理  
        // 该函数会寻找最高优先级（priority值越小优先级越高）的激活事件链表，  
        // 然后处理链表中的所有就绪事件；  
        // 因此低优先级的就绪事件可能得不到及时处理；  
        if (base->event_count_active) {  
            event_process_active(base);  
            if (!base->event_count_active && (flags & EVLOOP_ONCE))  
                done = 1;  
        } else if (flags & EVLOOP_NONBLOCK)  
            done = 1;  
    }  
    // 循环结束，清空时间缓存  
    base->tv_cache.tv_sec = 0;  
    event_debug(("%s: asked to terminate loop.", __func__));  
    return (0);  
}  
```



#### 集成

###  1.I/O和Timer事件的统一

​     Libevent将Timer和Signal事件都统一到了系统的I/O 的demultiplex机制中了，相信读者从上面的流程和代码中也能窥出一斑了，下面就再啰嗦一次了。
​     **首先将Timer事件融合到系统I/O多路复用机制中，还是相当清晰的，因为系统的I/O机制像select()和epoll_wait()都允许程序制定一个最大等待时间（也称为最大超时时间）timeout，即使没有I/O事件发生，它们也保证能在timeout时间内返回。**
那么根据所有Timer事件的最小超时时间来设置系统I/O的timeout时间；当系统I/O返回时，再激活所有就绪的Timer事件就可以了，这样就能将Timer事件完美的融合到系统的I/O机制中了。
​     这是在Reactor和Proactor模式（主动器模式，比如Windows上的IOCP）中处理Timer事件的经典方法了，ACE采用的也是这种方法，大家可以参考POSA vol2书中的Reactor模式一节。
​     堆是一种经典的数据结构，向堆中插入、删除元素时间复杂度都是O(lgN)，N为堆中元素的个数，而获取最小key值（小根堆）的复杂度为O(1)；因此变成了管理Timer事件的绝佳人选（当然是非唯一的），libevent就是采用的堆结构。

### 2. I/O和Signal事件的统一

​     Signal是异步事件的经典事例，将Signal事件统一到系统的I/O多路复用中就不像Timer事件那么自然了，Signal事件的出现对于进程来讲是完全随机的，进程不能只是测试一个变量来判别是否发生了一个信号，而是必须告诉内核“在此信号发生时，请执行如下的操作”。
**如果当Signal发生时，并不立即调用event的callback函数处理信号，而是设法通知系统的I/O机制，让其返回，然后再统一和I/O事件以及Timer一起处理，不就可以了嘛。**是的，这也是libevent中使用的方法。
​     问题的核心在于，当Signal发生时，如何通知系统的I/O多路复用机制，这里先买个小关子，放到信号处理一节再详细说明，我想读者肯定也能想出通知的方法，**比如使用pipe。**

http://blog.csdn.net/sparkliang/article/details/5011400

Socket pair就是一个socket对，包含两个socket，一个读socket，一个写socket。工作方式如下图所示：

![](http://p.blog.csdn.net/images/p_blog_csdn_net/sparkliang/EntryImages/20091215/sock%20pair.JPG)

创建一个socket pair并不是复杂的操作，可以参见下面的流程图，清晰起见，其中忽略了一些错误处理和检查。

![](http://p.blog.csdn.net/images/p_blog_csdn_net/sparkliang/EntryImages/20091215/sock%20pair%20flow.JPG)

完整流程

![](http://p.blog.csdn.net/images/p_blog_csdn_net/sparkliang/EntryImages/20091215/frame.JPG)