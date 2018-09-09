## list

Redis 早期版本存储 list 列表数据结构使用的是压缩列表 ziplist 和普通的双向链表 linkedlist，也就是元素少时用 ziplist，元素多时用 linkedlist。



考虑到链表的附加空间相对太高，prev 和 next 指针就要占去 16 个字节 (64bit 系统的指针是 8 个字节)，另外每个节点的内存都是单独分配，会加剧内存的碎片化，影响内存管理效率。后续版本对列表数据结构进行了改造，使用 quicklist 代替了 ziplist 和 linkedlist。...

![](https://user-gold-cdn.xitu.io/2018/7/29/164e3b0b953f2fc7?imageView2/0/w/1280/h/960/format/webp/ignore-error/1)



**quicklist 是 ziplist 和 linkedlist 的混合体，它将 linkedlist 按段切分，每一段使用 ziplist 来紧凑存储，多个 ziplist 之间使用双向指针串接起来。**



## zset

有序集合的编码可以是ziplist或者skiplist。

ziplist编码的压缩列表对象使用压缩列表作为底层实现，每个集合元素使用两个紧挨在一起的压缩列表节点来保存，第1个节点保存元素的成员（member）,而第2个保存元素的分值（score）

```
Typedef struct set {
	zskiplist *zsl
	dict *dict
}

```



值得一提的是，虽然zset结构同时使用跳跃表和字典来保存有序集合元素，但这两种数据结构都会**通过指针来共享相同元素的成员和分值**，所以同时使用跳跃表和字典来保存集合元素不会产生任何重复成员或者分值，也不会因此而浪费额外的内存。



采用skiplist与字典的组合实现有序集合，既考虑了字典O(1)的查找效率，也考虑了跳跃表范围查询，插入和删除O(logn)



### Rehash



### 应用场景

位图记录用户365天的签到情况

hyperlog记录uv，去重，还可以合并

Geo模块算地图上附近的人

分布式锁，设置一个值，还有过期时间

布隆过滤器，优化缓存穿透，比如推荐系统新用户先经过布隆过滤器，直接返回默认值，不用走缓存-存储



