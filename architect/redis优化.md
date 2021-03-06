### Redis优化

> 每一段使用一个Hash结构存储，由于Hash结构会在单个Hash元素在不足一定数量时进行压缩存储，所以可>以大量节约内存。这一点在String结构里是不存在的。而这个一定数量是由配置文件中的hash-zipmap-max-entries参数来控制的。

尽可能的使用 hashes ，时间复杂度低，查询效率高。同时还节约内存。Instagram 最开始用string来存`图
片id=>uid`的关系数据，用了21g，后来改为水平分割，图片id 1000 取模，然后将分片的数据存在一个hashse 里面，这样最后的内容减少了5g，四分之一基本上。

来自：http://blog.nosqlfan.com/html/3379.html

具体的做法就是将数据分段，每一段使用一个Hash结构存储，由于Hash结构会在单个Hash元素在不足一定数>量时进行压缩存储，所以可以大量[节约内存](http://blog.nosqlfan.com/tags/%e8%8a%82%e7%ba%a6%e5%86%85%e5%ad%98)。这一点在上面的String结构里是不存在的。而这个一定数量是由配置文件中的[hash](http://blog.nosqlfan.com/tags/hash)-zipmap-max-entries参数来控制的。经过开发者们的实验，将hash-zipmap-max-entries设置为1000时，性能比较好，超过1000后HSET命令就会导致CPU消耗变得非常大。

于是他们改变了方案，将数据存成如下结构：

```
HSET "mediabucket:1155" "1155315" "939"
HGET "mediabucket:1155" "1155315"
> "939"
```

**通过取7位的图片ID的前四位为Hash结构的key值，保证了每个Hash内部只包含3位的key，也就是1000个。**

再做一次实验，结果是每1,000,000个key只消耗了16MB的内存。总内存使用也降到了5GB，满足了应用需求。

（NoSQLFan：同样的，这里我们还是可以再进行优化，首先是将Hash结构的key值变成纯数字，这样key长度>减少了12个字节，其次是将Hash结构中的subkey值变成三位数，这又减少了4个字节的开销，如下所示。经过
实验，内存占用量会降到10MB，总内存占用为3GB）

```
HSET "1155" "315" "939"
HGET "1155" "315"
> "939"
```
