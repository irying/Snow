#### 1.请求阶段和debug



Nginx 的请求处理阶段共有 11 个之多，按照它们执行时的先后顺序，依次是 `rewrite` 阶段、`access` 阶段以及 `content` 阶段。

现在可以检查一下前面配置的 Nginx 错误日志文件中的输出。因为文件中的输出比较多（在我的机器上有 700 多行），所以不妨用 `grep` 命令在终端上过滤出我们感兴趣的部分：

```
    grep -E 'http (output filter|script (set|value))' logs/error.log
```

在我机器上的输出是这个样子的（为了方便呈现，这里对 `grep` 命令的实际输出作了一些简单的编辑，略去了每一行的行首时间戳）：

```
    [debug] 5363#0: *1 http script value: "32"
    [debug] 5363#0: *1 http script set $a
    [debug] 5363#0: *1 http script value: "56"
    [debug] 5363#0: *1 http script set $a
    [debug] 5363#0: *1 http output filter "/test?"
    [debug] 5363#0: *1 http output filter "/test?"
    [debug] 5363#0: *1 http output filter "/test?"
```

这里需要稍微解释一下这些调试信息的具体含义。[set](http://wiki.nginx.org/HttpRewriteModule#set) 配置指令在实际运行时会打印出两行以 `http script` 起始的调试信息，其中第一行信息是 [set](http://wiki.nginx.org/HttpRewriteModule#set) 语句中被赋予的值，而第二行则是 [set](http://wiki.nginx.org/HttpRewriteModule#set) 语句中被赋值的 Nginx 变量名。于是上面首先过滤出来的

```
    [debug] 5363#0: *1 http script value: "32"
    [debug] 5363#0: *1 http script set $a
```

这两行就对应我们例子中的配置语句

```
    set $a 32;
```

而接下来这两行调试信息

```
    [debug] 5363#0: *1 http script value: "56"
    [debug] 5363#0: *1 http script set $a
```

则对应配置语句

```
    set $a 56;
```

此外，**凡在 Nginx 中输出响应体数据时，都会调用 Nginx 的所谓“输出过滤器”（output filter）**，我们一直在使用的 [echo](http://wiki.nginx.org/HttpEchoModule#echo) 指令自然也不例外。而一旦调用 Nginx 的“输出过滤器”，便会产生类似下面这样的调试信息：

```
    [debug] 5363#0: *1 http output filter "/test?"
```

当然，这里的 `"/test?"` 部分对于其他接口可能会发生变化，因为它显示的是当前请求的 URI. 这样联系起来看，就不难发现，上例中的那两条 [set](http://wiki.nginx.org/HttpRewriteModule#set) 语句确实都是在那两条 [echo](http://wiki.nginx.org/HttpEchoModule#echo) 语句之前执行的。

​    细心的读者可能会问，为什么这个例子明明只使用了两条 [echo](http://wiki.nginx.org/HttpEchoModule#echo) 语句进行输出，但却有三行 `http output filter` 调试信息呢？其实，前两行 `http output filter` 信息确实分别对应那两条 [echo](http://wiki.nginx.org/HttpEchoModule#echo) 语句，**<u>而最后那一行信息则是对应 [ngx_echo](http://wiki.nginx.org/HttpEchoModule) 模块输出指示响应体末尾的结束标记。正是为了输出这个特殊的结束标记，才会多出一次对 Nginx “输出过滤器”的调用。</u>**包括 [ngx_proxy](http://wiki.nginx.org/HttpProxyModule) 在内的许多模块在输出响应体数据流时都具有此种行为。



**值得一提的是，并非所有的配置指令都与某个处理阶段相关联，例如我们先前在 [Nginx 变量漫谈（一）](http://blog.sina.com.cn/s/blog_6d579ff40100wi7p.html) 中提到过的 [geo](http://wiki.nginx.org/HttpGeoModule#geo) 指令以及在 [Nginx 变量漫谈（四）](http://blog.sina.com.cn/s/blog_6d579ff40100woyb.html) 中介绍过的 [map](http://wiki.nginx.org/HttpMapModule#map) 指令。这些不与处理阶段相关联的配置指令基本上都是“声明性的”（declarative），即不直接产生某种动作或者过程。Nginx 的作者 Igor Sysoev 在公开场合曾不止一次地强调，Nginx 配置文件所使用的语言本质上是“声明性的”，而非“过程性的”（procedural）。**



