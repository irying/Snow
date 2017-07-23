A记录就是把一个域名解析到一个IP地址（Address，特制数字IP地址），而CNAME记录就是把域名解析到另外一个域名。



127.0.0.1 kyrie.com



访问www.kyrieup.com或者www.kyrieupup.com都会跑到kyrie.com这就是cname记录。

1.这样设置有什么好处？修改ip的时候改一个地方就可以了。坏处呢？**一个cName要执行两次查询，会增加DNS服务器的负担，也会轻微影响查询速度**。使用何种方式站长自己确定，我的建议是都用A记录，不会增加多少麻烦。

2.但是有些地方少了cName还不行，例如你要用一个第三方平台或者是CDN，经常需要设置cName，为什么呢？**因为这些网站一般采用了CDN，采取智能解析，不同地区解析的IP是不一样的，这个时候就非cName莫属了**。



![](https://mengkang.net/upload/image/2016/1110/1478785430303547.png)

回源的时候，我们会要求 CDN 服务商，**不能所有节点直接回源到我们源站，协商要求他们使用<u>统一代理</u>回源我们源站，也就是说同一个资源只许他们回源一次**。之后，其他边缘节点没有缓存，**请求他们自身的代理**。

也就是说他们的 CDN 是有多级缓存的。



（笔记来自梦康博客https://mengkang.net/641.html）



2.批量添加CDN

业务需求：现在需要将某个域名（a.mengkang.net）下的图片访问的流量切换到 CDN 上。

操作步骤：

1. 先对原域名下访问日志做统计，统计出访问频次较高的图片地址（比如20万个地址），把这些地址交给cdn服务商。
2. 让他们先去预热抓取这20万个地址的资源。
3. 预热完毕后，我们再把（a.mengkang.net）的一部分域名换为(b.mengkang.net)。然后把b.mengkang.net做`cname`解析到cdn服务器给定的域名地址上去（比如b.mengkang.ccgslb.com.cn）。
4. 通过`wget`测试是访问域名`b.mengkang.net`下的图片是否能够被cdn缓存住。
5. cache测试没有问题之后，我再把`a.mengkang.net`下的部分流量切到`b.mengkang.net`上去，同事运维的同事监控流量回源的情况，根据回源情况再对分配流量的大小做调整。



## Wget定位问题

### HTTP 选项参数

- -http-user=USER 设定HTTP用户名为 USER.
- -http-passwd=PASS 设定http密码为 PASS
- -C, –cache=on/off 允许/不允许服务器端的数据缓存 (一般情况下允许)
- **-E, –html-extension 将所有text/html文档以.html扩展名保存**
- -ignore-length 忽略 ‘Content-Length’头域
- -header=STRING 在headers中插入字符串 STRING
- -proxy-user=USER 设定代理的用户名为 USER
- proxy-passwd=PASS 设定代理的密码为 PASS
- referer=URL 在HTTP请求中包含 ‘Referer: URL’头
- -s, –save-headers 保存HTTP头到文件
- **-U, –user-agent=AGENT 设定代理的名称为 AGENT而不是 Wget/VERSION**
- no-http-keep-alive 关闭 HTTP活动链接 (永远链接)
- cookies=off 不使用 cookies
- load-cookies=FILE 在开始会话前从文件 FILE中加载cookie
- save-cookies=FILE 在会话结束后将 cookies保存到 FILE文件中

# 用Wget下载受限内容

Wget可用于下载网站上登陆页面之后的内容，或避开HTTP参照位址(referer)和User Agent字符串对抓屏的限制。

14) 下载网站上的文件，假设此网站检查User Agent和HTTP参照位址(referer)

```
wget --referer=/5.0 --user-agent="Firefox/4.0.1" http://nytimes.com

```

15) 从密码保护网站上下载文件

```
wget --http-user=labnol --http-password=hello123 http://example.com/secret/file.zip

```

16) 抓取登陆界面后面的页面。你需要将用户名和密码替换成实际的表格域值，而URL应该指向(实际的)表格提交页面

```
wget --cookies=on --save-cookies cookies.txt --keep-session-cookies --post-data 'user=labnol&password=123' http://example.com/login.php
wget --cookies=on --load-cookies cookies.txt --keep-session-cookies http://example.com/paywall
```

##### 图片大面积无法加载，同一图片地址，时而能打开，时而无法访问。无法访问时，单独访问图片地址发现还跳转到了一个游戏网站主页。

###### 1.首先我们确定我们源站资源是可访问的，在 CDN 回源上不存在问题

```
wget -S -0 /dev/null --header="Host: f4:topit.me" http://111.1.23.214/4/2d/d1/1133196716aead12d4s.jpg
```

###### 2.然后通过`wget -S`打印详细的 http 头信息

```
wget -S  http://f4.topit.me/4/2d/d1/1133196716aead12d4s.jpg
```



#### 访问网页时 css 里面的图片都无法访问,单独打开图片地址能访问。使用 wget --referer 定位是防盗链错误设置



###### 1.首先确认源站没问题，模拟浏览器访问时带上`referer`

```
wget -S -O /dev/null --header="Host: img.topit.me" --referer="http://static.topitme.com/s/css/main21.css" http://211.155.84.132/img/bar/next.png
```

同时，绑定`host`的也采用另一种方式`wget -e http_proxy`

```
wget -SO /dev/null --referer="http://static.topitme.com/s/css/main21.css" http://img.topit.me/img/style/icon_heart.png -e http_proxy=211.155.84.137
```

###### 2. 然后直接请求，不绑定`host`再请求

```
wget -S  -O /dev/null  --referer="http://static.topitme.com/s/css/main21.css"  http://img.topit.me/img/bar/next.png
```
## wget —- 如何对服务器友好一些？

wget工具本质上是一个抓取网页的网络爬虫，但有些网站主机通过`robots.txt`文件来屏幕这些网络爬虫。另外，对于使用了[`rel-nofollow`](http://pkuwwt.github.io/linux/2015-09-26-all-the-wget-commands-you-should-know/)属性的网页，wget也不会扒取它的链接。

不过，你可以强迫wget忽略`robots.txt'和nofollow指令，只需在所有wget命令行中加上`–execute robots=off`选项即可。如果一个网页主机通过查看User Agent字段来屏幕wget请求，你也总是可以用`–user-agent=Mozilla`选项来伪装成火狐浏览器。

wget命令会增加网站服务器的负担，因为它不断地追踪链接，并下载文件。因而，一个好的网页抓取工具应该限制下载速度，而且还要在连接的抓取请求之间设置一个停顿，以缓解服务器的负担。

```
wget --limit-rate=20k --wait=60 --random-wait --mirror example.com

```

在上面的示例中，我们将下载带宽限制在了20KB/s，而且wget会在任意位置随机停顿30s至90s时间，然后再开始下一次下载请求。