#### IO多路复用的统一

 Libevent的核心是**事件驱动、同步非阻塞**，为了达到这一目标，必须采用系统提供的I/O多路复用技术，而这些在Windows、Linux、Unix等不同平台上却各有不同，如何能提供优雅而统一的支持方式，是首要关键的问题。

```C
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

在libevent中，每种I/O demultiplex机制的实现都必须提供这五个函数接口，来完成自身的初始化、销毁释放；对事件的注册、注销和分发。
比如对于epoll，libevent实现了5个对应的接口函数，并在初始化时并将eventop的5个函数指针指向这5个函数，那么程序就可以使用epoll作为I/O demultiplex机制了。



**Libevent把所有支持的I/O demultiplex机制存储在一个全局静态数组eventops中**，并在初始化时选择使用何种机制，数组内容根据优先级顺序声明如下：

```C
/* In order of preference */  
static const struct eventop *eventops[] = {  
#ifdef HAVE_EVENT_PORTS  
    &evportops,  
#endif  
#ifdef HAVE_WORKING_KQUEUE  
    &kqops,  
#endif  
#ifdef HAVE_EPOLL  
    &epollops,  
#endif  
#ifdef HAVE_DEVPOLL  
    &devpollops,  
#endif  
#ifdef HAVE_POLL  
    &pollops,  
#endif  
#ifdef HAVE_SELECT  
    &selectops,  
#endif  
#ifdef WIN32  
    &win32ops,  
#endif  
    NULL  
};   
```

然后libevent根据系统配置和编译选项决定使用哪一种I/O demultiplex机制，这段代码在函数event_base_new()中：

```
base->evbase = NULL;  
    for (i = 0; eventops[i] && !base->evbase; i++) {  
        base->evsel = eventops[i];  
        base->evbase = base->evsel->init(base);  
    }   
```

可以看出，libevent在编译阶段选择系统的I/O demultiplex机制，而不支持在运行阶段根据配置再次选择。
​    以Linux下面的epoll为例，实现在源文件epoll.c中，eventops对象epollops定义如下：

```C
const struct eventop epollops = {  
    "epoll",  
    epoll_init,  
    epoll_add,  
    epoll_del,  
    epoll_dispatch,  
    epoll_dealloc,  
    1 /* need reinit */  
};  
```



#### 时间管理

 Libevent在初始化时会检测系统时间的类型，通过调用函数detect_monotonic()完成，它**通过调用clock_gettime()来检测系统是否支持monotonic时钟类型**：

```C
static void detect_monotonic(void)  
{  
#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)  
    struct timespec    ts;  
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)  
        use_monotonic = 1; // 系统支持monotonic时间  
#endif  
}  
```

结构体event_base中的tv_cache，用来记录时间缓存。这个还要从函数gettime()说起，先来看看该函数的代码：

```C
static int gettime(struct event_base *base, struct timeval *tp)  
{  
    // 如果tv_cache时间缓存已设置，就直接使用  
    if (base->tv_cache.tv_sec) {  
        *tp = base->tv_cache;  
        return (0);  
    }  
    // 如果支持monotonic，就用clock_gettime获取monotonic时间  
#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)  
    if (use_monotonic) {  
        struct timespec    ts;  
        if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)  
            return (-1);  
        tp->tv_sec = ts.tv_sec;  
        tp->tv_usec = ts.tv_nsec / 1000;  
        return (0);  
    }  
#endif  
    // 否则只能取得系统当前时间  
    return (evutil_gettimeofday(tp, NULL));  
}  
```

如果tv_cache已经设置，那么就直接使用缓存的时间；否则需要再次执行系统调用获取系统时间。
​     **函数evutil_gettimeofday()用来获取当前系统时间，在Linux下其实就是系统调用gettimeofday()；Windows没有提供函数gettimeofday，而是通过调用_ftime()来完成的。**
​     在每次系统事件循环中，时间缓存tv_cache将会被相应的清空和设置，再次来看看下面event_base_loop的主要代码逻辑：

```C
int event_base_loop(struct event_base *base, int flags)  
{  
    // 清空时间缓存  
    base->tv_cache.tv_sec = 0;  
    while(!done){  
        timeout_correct(base, &tv); // 时间校正  
        // 更新event_tv到tv_cache指示的时间或者当前时间（第一次）  
         // event_tv <--- tv_cache  
        gettime(base, &base->event_tv);  
        // 清空时间缓存-- 时间点1  
        base->tv_cache.tv_sec = 0;  
        // 等待I/O事件就绪  
        res = evsel->dispatch(base, evbase, tv_p);  
        // 缓存tv_cache存储了当前时间的值-- 时间点2  
         // tv_cache <--- now  
        gettime(base, &base->tv_cache);  
        // .. 处理就绪事件  
    }  
    // 退出时也要清空时间缓存  
    base->tv_cache.tv_sec = 0;  
    return (0);  
}  

```

 时间event_tv指示了dispatch()上次返回，也就是I/O事件就绪时的时间，**第一次进入循环时，由于tv_cache被清空，因此gettime()执行系统调用获取当前系统时间；而后将会更新为tv_cache指示的时间。**
​     时间tv_cache在dispatch()返回后被设置为当前系统时间，因此它缓存了本次I/O事件就绪时的时间（event_tv）。
从代码逻辑里可以看出event_tv取得的是tv_cache上一次的值，因此**event_tv应该小于tv_cache的值。**
​     **设置时间缓存的优点是不必每次获取时间都执行系统调用，这是个相对费时的操作；在上面标注的时间点2到时间点1的这段时间（处理就绪事件时），调用gettime()取得的都是tv_cache缓存的时间。**



> 关于时间缓存我的理解，不知道对不对？
> 进行时时间缓存的目的？
> 当系统使用monotonic时钟，每次取得的时间一定是递增的，没有必要有timeout_correct函数，也就没必要有tv_cache。
> 为此我们引进固定时间点的概念！libevent在每次进行dispatch操作的时候，需要一个固定的时间（逻辑上的系统当前时间）与所有event的timeout时间进行比较。libevent亦是用event_tv来记录这个固定时间点。
>
> 但当系统为非monotonic时钟的时候，事情变得复杂。可能由于人为的干预，系统时间向前调整了（时间变得旧了），相应地我们必须将所有的事件的timeout时间也相应地向前调整两样的偏移。
>
> 难题在于我们如何判断出这个情况？？
>
> 我们必须通过比较才能得到结论！直观的，我们需要先记录两次时间点，然后进行比较，才能判断出时间向前调整的情况！于是，确定哪两个时间点，如何确定这两个时间点？在有了event_tv这个变量以后，我们又引进了tv_cache。
> while循环除了第一次之外，以后每次的循环开始之时，event_tv代表上次固定时间点(上次dispatch之前)，而tv_cache代表这次的固定时间点（上次dispatch之后）！
>
> 设置时间缓存(tv_cache)的另一个优点是不必每次获取时间都执行系统调用，这是个相对费时的操作

```C
static void timeout_correct(struct event_base *base, struct timeval *tv)  
{  
    struct event **pev;  
    unsigned int size;  
    struct timeval off;  
    if (use_monotonic) // monotonic时间就直接返回，无需调整  
        return;  
    gettime(base, tv); // tv <---tv_cache  
    // 根据前面的分析可以知道event_tv应该小于tv_cache  
    // 如果tv < event_tv表明用户向前调整时间了，需要校正时间  
    if (evutil_timercmp(tv, &base->event_tv, >=)) {  
        base->event_tv = *tv;  
        return;  
    }  
    // 计算时间差值  
    evutil_timersub(&base->event_tv, tv, &off);  
    // 调整定时事件小根堆  
    pev = base->timeheap.p;  
    size = base->timeheap.n;  
    for (; size-- > 0; ++pev) {  
        struct timeval *ev_tv = &(**pev).ev_timeout;  
        evutil_timersub(ev_tv, &off, ev_tv);  
    }  
    base->event_tv = *tv; // 更新event_tv为tv_cache  
}  
```

 在调整小根堆时，因为所有定时事件的时间值都会被减去相同的值，因此虽然堆中元素的时间键值改变了，**但是相对关系并没有改变，不会改变堆的整体结构。因此只需要遍历堆中的所有元素，将每个元素的时间键值减去相同的值即可完成调整，不需要重新调整堆的结构。**
当然调整完后，要将event_tv值重新设置为tv_cache值了。



#### 多线程

### 支持多线程的几种模式

​     Libevent并不是线程安全的，但这不代表libevent不支持多线程模式，其实方法在前面已经将signal事件处理时就接触到了，那就是消息通知机制。
一句话，“你发消息通知我，然后再由我在合适的时间来处理”；
​     说到这就再多说几句，再打个比方，把你自己比作一个工作线程，而你的头是主线程，你有一个消息信箱来接收别人发给你的消息，当时头有个新任务要指派给你。

#### 1 暴力抢占

那么第一节中使用的多线程方法相当下面的流程：
1 当时你正在做事，比如在写文档；
2 你的头找到了一个任务，要指派给你，比如帮他搞个PPT，哈；
3 头命令你马上搞PPT，你这是不得不停止手头的工作，把PPT搞定了再接着写文档；
…

#### 2 纯粹的消息通知机制

那么基于纯粹的消息通知机制的多线程方式就像下面这样：
1 当时你正在写文档；
2 你的头找到了一个任务，要指派给你，帮他搞个PPT；
3 头发个消息到你信箱，有个PPT要帮他搞定，这时你并不鸟他；
4 你写好文档，接着检查消息发现头有个PPT要你搞定，你开始搞PPT；
…
​     第一种的好处是消息可以立即得到处理，但是很方法很粗暴，你必须立即处理这个消息，所以你必须处理好切换问题，省得把文档上的内容不小心写到PPT里。在操作系统的进程通信中，消息队列（消息信箱）都是操作系统维护的，你不必关心。
第二种的优点是通过消息通知，切换问题省心了，不过消息是不能立即处理的（基于消息通知机制，这个总是难免的），而且所有的内容都通过消息发送，比如PPT的格式、内容等等信息，这无疑增加了通信开销。

#### 3 消息通知+同步层

​     有个折中机制可以减少消息通信的开销，就是提取一个同步层，还拿上面的例子来说，你把工作安排都存放在一个工作队列中，而且你能够保证“任何人把新任务扔到这个队列”，“自己取出当前第一个任务”等这些操作都能够保证不会把队列搞乱（其实就是个加锁的队列容器）。
再来看看处理过程和上面有什么不同：
1 当时你正在写文档；
2 你的头找到了一个任务，要指派给你，帮他搞个PPT；
2 头有个PPT要你搞定，他把任务push到你的工作队列中，包括了PPT的格式、内容等信息；
3 头发个消息（一个字节）到你信箱，有个PPT要帮他搞定，这时你并不鸟他；
4 你写好文档，发现有新消息（这预示着有新任务来了），检查工作队列知道头有个PPT要你搞定，你开始搞PPT；
…
工作队列其实就是一个加锁的容器（队列、链表等等），这个很容易实现实现；而消息通知仅需要一个字节，具体的任务都push到了在工作队列中，因此想比2.2减少了不少通信开销。
多线程编程有很多陷阱，线程间资源的同步互斥不是一两句能说得清的，而且出现bug很难跟踪调试；这也有很多的经验和教训，因此如果让我选择，在绝大多数情况下都会选择机制3作为实现多线程的方法。



memcached的实现

![](http://p.blog.csdn.net/images/p_blog_csdn_net/sparkliang/EntryImages/20100104/Picture1.jpg)