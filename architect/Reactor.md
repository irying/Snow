### Reactor

IO复用异步非阻塞程序使用经典的**Reactor**模型，Reactor顾名思义就是反应堆的意思，它本身不处理任何数据收发。只是可以监视一个socket句柄的事件变化。

![](http://rango.swoole.com/static/io/5.png)

Reactor有4个核心的操作：

1. add添加socket监听到reactor，可以是listen socket也可以是客户端socket，也可以是管道、eventfd、信号等
2. set修改事件监听，可以设置监听的类型，如可读、可写。可读很好理解，对于listen socket就是有新客户端连接到来了需要accept。对于客户端连接就是收到数据，需要recv。可写事件比较难理解一些。**一个SOCKET是有缓存区的，如果要向客户端连接发送2M的数据，一次性是发不出去的，操作系统默认TCP缓存区只有256K。一次性只能发256K，缓存区满了之后send就会返回EAGAIN错误。**这时候就要监听可写事件，在纯异步的编程中，必须去监听可写才能保证send操作是完全非阻塞的。
3. del从reactor中移除，不再监听事件
4. callback就是事件发生后对应的处理逻辑，一般在add/set时制定。C语言用函数指针实现，JS可以用匿名函数，PHP可以用匿名函数、对象方法数组、字符串函数名。

**Reactor只是一个事件发生器，实际对socket句柄的操作，如connect/accept、send/recv、close是在callback中完成的。**



Reactor模型还可以与多进程、多线程结合起来用，既实现异步非阻塞IO，又利用到多核。目前流行的异步服务器程序都是这样的方式：如

- Nginx：多进程Reactor
- Nginx+Lua：多进程Reactor+协程
- Golang：单线程Reactor+多线程协程
- Swoole：多线程Reactor+多进程Worker

# Reactor线程

> Swoole的主进程是一个多线程的程序。其中有一组很重要的线程，称之为Reactor线程。它就是真正处理TCP连接，收发数据的线程。
>
> swoole的主线程在Accept新的连接后，会将这个连接分配给一个固定的Reactor线程，并由这个线程负责监听此socket。在socket可读时读取数据，并进行协议解析，将请求投递到Worker进程。在socket可写时将数据发送给TCP客户端。
>
> > 分配的计算方式是fd % serv->reactor_num

### 协程是什么

协程从底层技术角度看实际上还是异步IO Reactor模型，应用层自行实现了任务调度，借助Reactor切换各个当前执行的用户态线程，但用户代码中完全感知不到Reactor的存在。



补充：

## 异步非阻塞方式

> Nginx 的 worker 进程采用**异步非阻塞**的方式来处理请求。对 Nginx 来说，每进来一个 request，就会有一个 worker 进程去处理。但不是一次性对这个 request 进行全程的处理，而是处理到这个 request 需要等待的地方，比如向上游（后端）服务器转发 request，并等待请求返回。
>
> 而这时，处理这个 request 的 worker 不会这么傻等着而是在发送完请求后，注册一个事件：“如果 upstream 返回了，告诉我一声，我再接着干”（异步）。这时候他就又有能力区处理其他的 request（非阻塞，不会等待网络 IO 就绪，worker 进程**不会进入睡眠状态**），而不是阻塞。
>
> 如果再有 request 进来，它就可以很快再按这种方式处理。而一旦上游服务器返回了，就会触发注册的事件，worker 就回来对这个 request 继续进行处理。可以看到，虽然一个 worker 同一时刻只能处理一个 request ，但是，对于一个 worker 所承担的所有 request ， worker 是轮流进行处理的，不断的对可进一步处理的 request 进行切换， 而且切换也是因为异步事件未准备好，而主动让出的，这里的切换是没有任何代价，可以理解为循环处理多个准备好的事件。

http://blog.csdn.net/mqfcu8/article/details/70226915