让我们再强化一下变量值与<u>“**变量设置跟请求绑定**”</u>这个结论



​    所谓“主请求”，就是由 HTTP 客户端从 Nginx 外部发起的请求。我们前面见到的所有例子都只涉及到“主请求”，包括 [（二）](http://blog.sina.com.cn/s/blog_6d579ff40100wk2j.html) 中那两个使用 [echo_exec](http://wiki.nginx.org/HttpEchoModule#echo_exec) 和 [rewrite](http://wiki.nginx.org/HttpRewriteModule#rewrite) 指令发起“内部跳转”的例子。

​    而“子请求”则是由 Nginx 正在处理的请求**在 Nginx 内部发起的一种级联请求**。“子请求”在外观上很像 HTTP 请求，但实现上却和 HTTP 协议乃至网络通信一点儿关系都没有。它是 Nginx 内部的一种抽象调用，<u>**目的是为了方便用户把“主请求”的任务分解为多个较小粒度的“内部请求”，并发或串行地访问多个 `location` 接口，然后由这些 `location` 接口通力协作，共同完成整个“主请求”。**</u>当然，“子请求”的概念是相对的，任何一个“子请求”也可以再发起更多的“子子请求”，甚至可以玩递归调用（即自己调用自己）。当一个请求发起一个“子请求”的时候，按照 Nginx 的术语，习惯把前者称为后者的“父请求”（parent request）。



#### 第1种情况：

​	它们的生命期是与当前请求相关联的。每个请求都有所有变量值容器的独立副本，只不过当前请求既可以是“主请求”，也可以是“子请求”。即便是父子请求之间，**同名变量一般也不会相互干扰**。

```nginx
location /main {
    set $var main;

    echo_location /foo;
    echo_location /bar;

    echo "main: $var";
}

location /foo {
    set $var foo;
    echo "foo: $var";
}

location /bar {
    set $var bar;
    echo "bar: $var";
}
```

Result :

在这个例子中，我们分别在 `/main`，`/foo` 和 `/bar` 这三个 `location` 配置块中为同一名字的变量，`$var`，分别设置了不同的值并予以输出。特别地，我们在 `/main` 接口中，故意在调用过 `/foo` 和 `/bar` 这两个“子请求”之后，再输出它自己的 `$var` 变量的值。请求 `/main` 接口的结果是这样的：

```
    $ curl 'http://localhost:8080/main'
    foo: foo
    bar: bar
    main: main
```

显然，`/foo` 和 `/bar` 这两个“子请求”在处理过程中对变量 `$var` 各自所做的修改都丝毫没有影响到“主请求” `/main`. 于是这成功印证了“**主请求”以及各个“子请求”都拥有不同的变量 `$var` 的值容器副本**。



#### 第2种情况：

一些 Nginx 模块发起的“子请求”却会自动共享其“父请求”的变量值容器，比如第三方模块 [ngx_auth_request](http://mdounin.ru/hg/ngx_http_auth_request_module/). 下面是一个例子：

```nginx
location /main {
    set $var main;
    auth_request /sub;
    echo "main: $var";
}

location /sub {
    set $var sub;
    echo "sub: $var";
}
```

然后使用 [ngx_auth_request](http://mdounin.ru/hg/ngx_http_auth_request_module/) 模块提供的配置指令 `auth_request`，发起一个到 `/sub` 接口的“子请求”，最后利用 [echo](http://wiki.nginx.org/HttpEchoModule#echo) 指令输出变量 `$var` 的值。而我们在`/sub` 接口中则故意把 `$var` 变量的值改写成 `sub`. 访问 `/main` 接口的结果如下：

```
    $ curl 'http://localhost:8080/main'
    main: sub
```

“为什么‘子请求’ `/sub` 的输出没有出现在最终的输出里呢？”答案很简单，那就是因为 `auth_request` 指令会自动忽略“子请求”的响应体，而只检查“子请求”的响应状态码。当状态码是 `2XX` 的时候，<u>**`auth_request` 指令会忽略“子请求”而让 Nginx 继续处理当前的请求，否则它就会立即中断当前（主）请求的执行，返回相应的出错页。**</u>在我们的例子中，`/sub` “子请求”只是使用 [echo](http://wiki.nginx.org/HttpEchoModule#echo) 指令作了一些输出，所以隐式地返回了指示正常的 `200` 状态码。

​    如 [ngx_auth_request](http://mdounin.ru/hg/ngx_http_auth_request_module/) 模块这样父子请求共享一套 Nginx 变量的行为，虽然可以让父子请求之间的数据双向传递变得极为容易，但是对于足够复杂的配置，却也经常导致不少难于调试的诡异 bug. 因为用户时常不知道“父请求”的某个 Nginx 变量的值，其实已经在它的某个“子请求”中被意外修改了。诸如此类的因共享而导致的不好的“副作用”，让包括 [ngx_echo](http://wiki.nginx.org/HttpEchoModule)，[ngx_lua](http://wiki.nginx.org/HttpLuaModule)，以及 [ngx_srcache](http://wiki.nginx.org/HttpSRCacheModule) 在内的许多第三方模块都选择了**禁用父子请求间的变量共享**。