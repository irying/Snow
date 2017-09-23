来自http://blog.csdn.net/chdhust/article/details/9421237



# 1. 为什么进行超时处理

Nginx有必要对可能发生超时的事件进行统一管理，并在事件超时时作出相应的处理，比如回收资源，返回错误等。举例来说，当客户端对nginx发出请求连接后，nginx机会accept()并建立对应的连接对象、读取额护短请求的头部信息。而读取这个头部信息显然是要在一定的时间内完成的。<u>**如果在一个有限的时间内没有读取到头部信息或者读取读取的头部信息不完整，那么nginx就无法进行正常处理，并且认为这是一个错误/非法的请求，直接返回错误信息并释放相应资源**</u>，如果nginx不这样做，那么针对如此的恶意攻击就和容易实施。

# 2. 如何进行超时处理

nginx对于是否存在有超时事件的处理很巧妙。首先，nginx利用红黑树来组织那些等待处理的并且需要关注其是否超时的事件对象（以下称该红黑树为事件计时红黑树：event_timer_rbtree）；其次，nginx提供了两种方案来检测那些等待处理的事件对象是否已经超时，<u>**一种为常规的定时检测机制，也就是设置定时器，每过一定的时间就对红黑树管理的所有事件对象进行一次超时检测**</u>；**<u>另一种是过距离当前最快发生超时的事件对象的时间就进行一次超时检测。</u>**



# 3. nginx中超时处理的实现

超时事件对象的数据结构组织

我们已经知道nginx把事件封装在一个名为ngx_event_s的结构体内，而该结构体有几个字段与本节讲的nginx超时处理相关。
**unsigned         timedout:1;**
**unsigned         timer_set:1;**
**ngx_rbtree_node_t   timer;**
timedout域字段，用于标识该当前事件是否已经超时，0为没有超时。
timer_set域字段，用于标识该当前事件是否已经加入到红黑树，需要对其是否超时做监控，0为没有加入。
tmer字段，很容易看出属于红黑树节点类型变量，红黑树就是通过该字段来组织事件对象。nginx设置了两个全局变量以便在程序的任何地方都能快速的访问到事件计时红黑树：
**ngx_thread_volatile ngx_rbtree_t  ngx_event_timer_rbtree;**
**static ngx_rbtree_node_t  ngx_event_timer_sentinel;**
它们都定义在源文件ngx_event_timer.c内，结构体ngx_rbtree_s类型的全局变量ngx_event_timer_rbtree封装了事件计时红黑树树结构，而ngx_event_timer_sentinel属于红黑树节点类型变量，在红黑树的操作过程中当作哨兵使用，同时它是static的，所以作用域仅限于模块内。



## 超时对象的两种检测方法

```C
void  
ngx_process_events_and_timers(ngx_cycle_t *cycle)  
{  
    ngx_uint_t  flags;  
    ngx_msec_t  timer, delta;  
  
    if (ngx_timer_resolution) {  
        timer = NGX_TIMER_INFINITE;  
        flags = 0;  
  
    } else {  
        timer = ngx_event_find_timer();//将超时检测时间设置为最快发生超时的事件对象的超时时刻与当前时刻之差  
        flags = NGX_UPDATE_TIME;  
...  
(void) ngx_process_events(cycle, timer, flags);  
...  
}  
```



当ngx_timer_resolution不为0时，即方案1.timer为无限大。timer在函数ngx_process_events()内被用作事件机制被阻塞的最长时间。那么timer为无限大会不会导致事件处理机制无限等待而超时事件得不到及时处理呢？不会。因为正常情况下事件处理机制会监控到某些I/O事件的发生。**即便是服务器太闲，没有任何I/O事件发生，工作进程也不会无限等待。因为工作进程一开始就设置好了一个定时器，这实现在初始化函数ngx_event_process_init()内**，看代码：

```c
static ngx_int_t  
ngx_event_process_init(ngx_cycle_t *cycle)  
{  
        ...  
        sa.sa_handler = ngx_timer_signal_handler;  
        sigemptyset(&sa.sa_mask);  
        itv.it_interval.tv_sec = ngx_timer_resolution / 1000;  
        itv.it_interval.tv_usec = (ngx_timer_resolution % 1000) * 1000;  
        itv.it_value.tv_sec = ngx_timer_resolution / 1000;  
        itv.it_value.tv_usec = (ngx_timer_resolution % 1000 ) * 1000;  
  
        if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {  
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,  
                          "setitimer() failed");  
        }  
        ....  
}  
```



回调函数ngx_timer_signal_handler：

```c
static void  
ngx_timer_signal_handler(int signo)  
{  
    ngx_event_timer_alarm = 1;  
  
#if 1  
    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, ngx_cycle->log, 0, "timer signal");  
#endif  
}  
```

可以看出它仅仅是**将标志变量ngx_event_timer_alarm 设置为1**.

只有在ngx_event_timer_alarm 为1 的情况下，工作进程才会更新时间。函数ngx_epoll_process_events中的代码（ngx_epoll_module.c）：

```c
static ngx_int_t  
ngx_epoll_process_events(ngx_cycle_t *cycle, ngx_msec_t timer, ngx_uint_t flags)  
{  
...  
    events = epoll_wait(ep, event_list, (int) nevents, timer);  
  
    err = (events == -1) ? ngx_errno : 0;  
  
    if (flags & NGX_UPDATE_TIME || ngx_event_timer_alarm) {  
        ngx_time_update();  
    }  
...  
} 
```

### 当ngx_time_update()被执行后就会执行expire检测函数

```
void  
ngx_process_events_and_timers(ngx_cycle_t *cycle)  
{  
...  
    delta = ngx_current_msec;  
    (void) ngx_process_events(cycle, timer, flags);//事件处理函数  
    delta = ngx_current_msec - delta;  
...  
if (delta) {  
        ngx_event_expire_timers();//超时检测函数  
    }  
...  
}  
```



#### 我们的目标是执行这个expire函数，上面是第一种方式，下面讲第2种

当ngx_timer_resolution为0时，timer设置为最快发生超时的事件对象的超时时刻与当前时刻的时间差，这个timer的计算来自函数ngx_event_find_time

```C
ngx_msec_t  
ngx_event_find_timer(void)  
{  
    ngx_msec_int_t      timer;  
    ngx_rbtree_node_t  *node, *root, *sentinel;  
  
    if (ngx_event_timer_rbtree.root == &ngx_event_timer_sentinel) {  
        return NGX_TIMER_INFINITE;  
    }  
  
    ngx_mutex_lock(ngx_event_timer_mutex);  
  
    root = ngx_event_timer_rbtree.root;  
    sentinel = ngx_event_timer_rbtree.sentinel;  
  
    node = ngx_rbtree_min(root, sentinel);  
  
    ngx_mutex_unlock(ngx_event_timer_mutex);  
  
    timer = (ngx_msec_int_t) (node->key - ngx_current_msec);  
  
    return (ngx_msec_t) (timer > 0 ? timer : 0);  
}  
```

该函数从红黑树中找到key值最小的节点，然后用key值减去当前时刻即得到预期timer值。这个值可能是负数，表示已经有事件超时了。因此直接将其设置为0.那么事件处理机制在开始监控I/O事件时会立即返回，以便马上处理这些超时事件。同时flags被设置为NGX_UPDATE_TIME。从ngx_epoll_process_events函数的代码中可以看出ngx_time_update()将被执行，事件被更新。



**如果I/O事件比较多，那么会导致比较频繁地调用gettimeofday()系统函数，这也可以说是超时检测方案2对性能的最大影响。这个时候超时检测函数ngx_event_expire_timers()函数会被执行。**



![](http://lenky.info/article-images/20110909/image007.png)





### 定时事件的使用

```C
static ngx_connection_t dummy;  
static ngx_event_t ev;   
  
  
static void  
ngx_http_hello_print(ngx_event_t *ev)   
{  
    printf("hello world\n");  
  
    ngx_add_timer(ev, 1000);  
}  
  
  
static ngx_int_t  
ngx_http_hello_process_init(ngx_cycle_t *cycle)  
{  
    dummy.fd = (ngx_socket_t) -1;   
  
    ngx_memzero(&ev, sizeof(ngx_event_t));  
  
    ev.handler = ngx_http_hello_print;  
    ev.log = cycle->log;  
    ev.data = &dummy;  
  
    ngx_add_timer(&ev, 1000);  
  
    return NGX_OK;  
}  
```

这段代码将注册一个定时事件——每过一秒钟打印一次hello world。ngx_add_timer函数就是用来完成将一个新的定时事件加入定时器红黑树中，定时事件被执行后，就会从树中移除，**因此要想不断的循环打印hello world，就需要在事件回调函数被调用后再将事件给添加到定时器红黑树中。 ngx_http_hello_process_init是注册在模块的进程初始化阶段的回调函数上。**由于，ngx_even_core_module模块排在自定义模块的前面，所以我们在进程初始化阶段添加定时事件时，定时器已经被初始化好了。



### 最后不要忘了初始化部分

```C
if (ngx_event_timer_init(cycle->log) == NGX_ERROR) {  
        return NGX_ERROR;  
}   
```

调用ngx_event_timer_init函数完成定时器红黑树的建树操作，这棵红黑树在存储定时器的同时，也为epoll_wait提供了等待时间。



#### 结论就是：

使用setitimer系统调用设置系统定时器，每当到达时间点后将发生SIGALRM信号，同时epoll_wait的阻塞将被信号中断从而被唤醒执行定时事件。其实，这段初始化并不是一定会被执行的，它的条件ngx_timer_resolution就是通过配置指令timer_resolution来设置的，如果没有配置此指令，就不会执行这段初始化代码了。也就是说，配置文件中使用了timer_resolution指令后，epoll_wait将使用信号中断的机制来驱动定时器，否则将使用定时器红黑树的最小时间作为epoll_wait超时时间来驱动定时器。