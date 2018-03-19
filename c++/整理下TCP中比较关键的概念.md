整理下TCP中比较关键的概念

1.为什么会出现大量closed_wait状态？怎么解决？

#### 场景：

因此说明，是SLB主动关闭了连接但是多台应用服务器都没有响应ack 导致了close_wait。



出现大量close_wait的现象，主要原因是**某种情况下对方关闭了socket链接，但是我方忙与读或者写，没有关闭连接。**代码需要判断socket，一旦读到0，断开连接，read返回负，检查一下errno，如果不是AGAIN，就断开连接。

参考：http://www.cnblogs.com/shengs/p/4495998.html



2.为什么会出现大量time_wait状态？怎么解决？

#### 场景：

​	在高并发短连接的TCP服务器上，当服务器处理完请求后立刻按照主动正常关闭连接。这个场景下，会出现大量socket处于TIMEWAIT状态。如果客户端的并发量持续很高，此时部分客户端就会显示连接不上。

```
我来解释下这个场景。主动正常关闭TCP连接，都会出现TIMEWAIT。为什么我们要关注这个高并发短连接呢？有两个方面需要注意：
1. 高并发可以让服务器在短时间范围内同时占用大量端口，而端口有个0~65535的范围，并不是很多，刨除系统和其他服务要用的，剩下的就更少了。
2. 在这个场景中，短连接表示“业务处理+传输数据的时间 远远小于 TIMEWAIT超时的时间”的连接。这里有个相对长短的概念，比如，取一个web页面，1秒钟的http短连接处理完业务，在关闭连接之后，这个业务用过的端口会停留在TIMEWAIT状态几分钟，而这几分钟，其他HTTP请求来临的时候是无法占用此端口的
```

#### 可行而且必须存在，但是不符合原则的解决方式

1. linux没有在sysctl或者proc文件系统暴露修改这个TIMEWAIT超时时间的接口，可以修改**内核协议栈代码中关于这个TIMEWAIT的超时时间参数**，重编内核，让它缩短超时时间，加快回收；
2. 利用SO_LINGER选项的强制关闭方式，发RST而不是FIN，来越过TIMEWAIT状态，直接进入CLOSED状态。详见我的博文[《TCP之选项](http://blog.chinaunix.net/uid-29075379-id-3904022.html)[SO_LINGER](http://blog.chinaunix.net/uid-29075379-id-3904022.html)[》](http://blog.chinaunix.net/uid-29075379-id-3904022.html)。

```
sysctl改两个内核参数就行了，如下：
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_tw_recycle = 1
简单来说，就是打开系统的TIMEWAIT重用和快速回收
```

参考：https://zhuanlan.zhihu.com/p/29334504



3.RST攻击是什么？怎么解决？





整理下进程切换比较关键的概念

详细描述下进程切换的各种状态



整理下Redis中比较关键的概念

讲一讲Redis跳跃表的实现？