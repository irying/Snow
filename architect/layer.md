来自58沈剑架构师之路

https://mp.weixin.qq.com/s/4UB4AZe2R0lVCT6f5ExEMw

![](https://mmbiz.qpic.cn/mmbiz_png/YrezxckhYOxsH4QG9XwyWxE8PIogeSocd7oRicBIDN3TbM83nPngicWel4bgIMOOzzATPKzVrgOo1EkuZF8up2icQ/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1)

- **客户端层**：典型调用方是browser或者APP
- **站点应用层**：实现核心业务逻辑，从下游获取数据，对上游返回html或者json
- **服务层**
- **数据-缓存层**：加速访问存储
- **数据-数据库层**：固化数据存储

![](https://mmbiz.qpic.cn/mmbiz_png/YrezxckhYOxsH4QG9XwyWxE8PIogeSocXe8WZ17gYvt8femEZOXnJm4OTEJp7dh15qUPYrc7ukK7ewh0kTvLdQ/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1)



**那么，互联网分层架构的本质究竟是什么呢？**

如果我们仔细思考会发现，不管是跨进程的分层架构，还是进程内的MVC分层，都是一个**“数据移动”**，然后**“被处理”**和**“被呈现”**的过程，归根结底一句话：互联网分层架构，是一个数据移动，处理，呈现的过程，其中**数据移动是整个过程的核心**。



### Data Moving

**数据是移动的**：

- 跨进程移动：数据从数据库和缓存里，转移到service层，到web-server层，到client层
- 同进程移动：数据从model层，转移到control层，转移到view层

数据要移动，所以有两个东西很重要：

1.数据传输的格式

```
service与db/cache之间，二进制协议/文本协议是数据传输的载体
web-server与service之间，RPC的二进制协议是数据传输的载体
client和web-server之间，http协议是数据传输的载体
```

2.数据在各层次的形态

```
db层，数据是以“行”为单位存在的row(uid, name, age)
cache层，数据是以kv的形式存在的kv(uid -> User)
service层，会把row或者kv转化为对程序友好的User对象
web-server层，会把对程序友好的User对象转化为对http友好的json对象
client层：最终端上拿到的是json对象
```



### 核心原则

“分层架构演进”的核心原则与方法：

- 让上游更高效的获取与处理数据，**复用**
- 让下游能屏蔽数据的获取细节，**封装**