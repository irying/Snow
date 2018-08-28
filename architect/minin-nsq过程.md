1. 每个channel的堆积上限是多少？超过上限之后处理逻辑是什么？
2. 多个consumer消费同一个channel采用什么样的分配策略，是否公平？
3. 从topic到多个channel，从channel到多个consumer，**如何保证消息不会丢失**？
4. 如何保证同一channel下**一条消息不会发送到多个consumer**?

channel的性质

整个流程

有序性

幂等性



1.先写tcphandler

2.配置解析

3.nsqlookupd

4.nsq的扫描

如果连接失败了，怎么恢复