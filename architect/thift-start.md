1.先上架构图

![](http://hi.csdn.net/attachment/201006/29/0_12778181692eHO.gif)



实际上Thrift提供了一个支持多语言的Lib库，包括C++、Java、PHP、Python、Ruby等。同时它自定义了一种中间语言(Thrift IDL)，

用于编写统一格式的thrift配置文件，定义数据类型。然后**根据此配置文件调用lib库中的生成器来生成指定语言的服务**，这正体现其跨语言的特性。

 

- **Thrift支持五种数据类型：**
- **Base Types：基本类型**
- **Struct：结构体类型**
- **Container：容器类型，即List、Set、Map**
- **Exception：异常类型**
- **Service：类似于面向对象的接口定义，里面包含一系列的方法**

前3种类型

![](http://hi.csdn.net/attachment/201006/29/0_1277792864s4Gg.gif)

2.Transport和Protocol

Thrift通过两种抽象机制来完成底层客户端和服务器端的通信，分别是:

1)  Transport：抽象了底层网络通信部分的接口
TTransport对Java I/O层进行封装，抽象了底层网络通信部分的接口。主要是对以下6种接口进行了抽象：open, close, isopen, read, write, flush。

2)  Protocol：抽象了对象在网络中传输部分的接口，它规定了thrift的RPC可以调用的接口。这些接口是为了传输不同类型的数据而设计的。

   Transport在传输数据时是不区分所传的数据类型的，一律以流的形式传输。所以Protocol类在Transport类之上实现了按数据类型来传输数据。



**Transport**

![](http://hi.csdn.net/attachment/201006/29/0_1277793651jzEU.gif)



**Protocol**

![](http://hi.csdn.net/attachment/201006/29/0_1277802162IA37.gif)





3.RPC的实现

1. TProcessor

它是thrift中最简单的一个接口了，仅包含一个方法：
bool process(TProtocol in, TProtocol out)throws TException

此方法有两个参数in和out，分别用于从socket中读取数据和向scoket中写数据。每个service对象中都会包含一个实现了此接口的类对象(Process),

这个类就是处理RPC请求的核心。

 

2. TServer

TServer相当于一个容器，拥有生产TProcessor、TTransport、TProtocol的工厂对象。它的工作流如下：

1) 使用TServerTransport来获取一个TTransport(客户端的连接)

2) 使用TTransportFactory来将获取到的原始的transport转化为特定的transport

3) 使用TProtocolFactory来为获取TTransport的输入输出，即TProtocol

4) 唤醒TProcessor的process方法，处理请求并返回结果