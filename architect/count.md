#### 一.先上结论

小小的计数，在数据量大，并发量大的时候，其架构实践思路为：

1. **计数外置**：由“count计数法”升级为“计数外置法”
2. 读多写少，甚至写多但一致性要求不高的计数，需要进行**缓存优化**，降低数据库压力
3. **缓存kv设计优化**，可以由[key:type]->[count]，优化为[key]->[c1:c2:c3]

即：

![](https://mmbiz.qpic.cn/mmbiz_png/YrezxckhYOyhdjRm6mNM9SsHXFZU0z6VRibOj1XuUUIiaicAvBeVvCNtu3hn4BUBR5ygtfiaciccqtU5lhSkfq13RgA/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1)

优化成：

![](https://mmbiz.qpic.cn/mmbiz_png/YrezxckhYOyhdjRm6mNM9SsHXFZU0z6VUhiahPklOpazicF0Nicvqs6ZPHrpkVHuDnfvOFhMo9jByibUXzWRkhBhSA/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1)

4.**数据库扩展性优化**，可以由列扩展优化为行扩展

即：

 ![](https://mmbiz.qpic.cn/mmbiz_png/YrezxckhYOyhdjRm6mNM9SsHXFZU0z6VZdjfDxlZqicQHlyGAPheGxF8Zzk22MzDlzOJBf1rWuGTwjD9rTEDtKQ/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1)

优化为：

 ![](https://mmbiz.qpic.cn/mmbiz_png/YrezxckhYOyhdjRm6mNM9SsHXFZU0z6Vx0VW7ibpNSGvazUbYdHBm14Q3HTJCaXwYdqL4oFpuibLflK1JDd7gqMQ/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1)



#### **二.一开始是这样的**

![](https://mmbiz.qpic.cn/mmbiz_png/YrezxckhYOyhdjRm6mNM9SsHXFZU0z6VGb1icKvLEQBciahu6vJibSODxWkKuh5CQcAJVzfibGRWhvdJmHUFB3vibEQ/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1)

和

![](https://mmbiz.qpic.cn/mmbiz_png/YrezxckhYOyhdjRm6mNM9SsHXFZU0z6VZkTwaw115b3sLJX70bK7nSNRF9MSibWIAicmxkW3ccdCwYLhHtAarHkw/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1)



整个拉取计数的伪代码如下：

list<msg_id> = getHomePageMsg(uid);// 获取首页所有消息

for( msg_id in list<msg_id>){ // 对每一条消息

​         getReadCount(msg_id);  // 阅读计数

​         getForwordCount(msg_id); // 转发计数

​         getCommentCount(msg_id); // 评论计数

​         getPraiseCount(msg_id); // 赞计数

}

其中：

- 每一个微博消息的若干个计数，都对应4个后端服务访问
- 每一个访问，对应一条count的数据库访问（count要了老命了）

其效率之低，资源消耗之大，处理时间之长，可想而知。



#### **三、计数外置的架构设计**

于是可以抽象出两个表，针对这两个维度来进行计数的存储：

**t_user_count** (uid, gz_count, fs_count, wb_count);

**t_msg_count** (msg_id, forword_count, comment_count, praise_count);

存储抽象完，再抽象出一个计数服务对这些数据进行管理，提供**友善的RPC接口**



 ![](https://mmbiz.qpic.cn/mmbiz_png/YrezxckhYOyhdjRm6mNM9SsHXFZU0z6V7mGTCyQdaccPciccAaDQc2x6xViaCTQicoYBlGicnBmc9nmBQ7AokhZF4w/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1)



这样，在查询一条微博消息的若干个计数的时候，不用进行多次数据库count操作，而会转变为一条数据的多个属性的查询：

for(msg_id in list<msg_id>) {

select forword_count, comment_count, praise_count 

​    from t_msg_count 

​    where msg_id=$msg_id;

}

 

甚至，可以将微博首页所有消息的计数，转变为一条IN语句（不用多次查询了）的批量查询：

select * from t_msg_count 

​    where msg_id IN

​    ($msg_id1, $msg_id2, $msg_id3, …);

**IN查询可以命中msg_id聚集索引**，效率很高。



#### 四.加个缓存

![](https://mmbiz.qpic.cn/mmbiz_png/YrezxckhYOyhdjRm6mNM9SsHXFZU0z6VjbrOM0p2we3tkMr0SUdG5XQ5zbMgoicGiaeQ6vjn5E6cjRxRv45nr0xw/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1)



for(uid in list<uid>) {

 	memcache::get($uid:c1, $uid:c2, $uid:c3);

}

改改缓存的键，换成上面结论3这种，于是就可以memcache::get($uid1, $uid2, $uid3, …);





