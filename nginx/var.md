#### 一.set

1.**set属于ngx_rewite模块，echo属于ngx_echo模块，geo属于ngx_geo模块**；前两者支持“变量插值”，geo不支持。

```nginx
geo $dollar {
  default "$";
}
 
server {
  listen 8080;

  location /test {
    echo "This is a dollar sign: $dollar";
  }
}
```

Result: This is a dollar sign: $

​	如果我们想通过 [echo](http://wiki.nginx.org/HttpEchoModule#echo) 指令直接输出含有“美元符”（`$`）的字符串，那么有没有办法把特殊的 `$` 字符给转义掉呢？答案是否定的（至少到目前最新的 Nginx 稳定版 `1.0.10`）。不过幸运的是，我们可以绕过这个限制，比如通过不支持“变量插值”的模块配置指令专门构造出取值为 `$` 的 Nginx 变量，然后再在 [echo](http://wiki.nginx.org/HttpEchoModule#echo) 中使用这个变量。

2.**变量插值：让专门的字符串拼接运算符变得不再那么必要**

```nginx
 set $a hello;
 set $b "$a, $a";
```

 `$b` 的值则是 `hello, hello`

3.**声明式**

```Nginx
server {
	listen 8080;

	location /foo {
		echo "foo = [$foo]";
	}

	location /bar {
		set $foo 32;
		echo "foo = [$foo]";
	}
}

```

- 这里我们在 `location /bar` 中用 `set` 指令创建了变量 `$foo`，于是在整个配置文件中这个变量都是可见的，因此我们可以在 `location /foo` 中直接引用这个变量而不用担心 Nginx 会报错。
- `set` 指令因为是在 `location /bar` 中使用的，所以赋值操作只会在访问 `/bar` 的请求中执行。而请求 `/foo` 接口时，我们总是得到空的 `$foo` 值，**因为用户变量未赋值就输出的话，得到的便是空字符串。**
- [set](http://wiki.nginx.org/HttpRewriteModule#set) 指令（以及前面提到的 [geo](http://wiki.nginx.org/HttpGeoModule#geo) 指令）不仅有赋值的功能，它还有创建 Nginx 变量的副作用，即当作为赋值对象的变量尚不存在时，它会自动创建该变量。

Result:**<u>Nginx 变量名的可见范围虽然是整个配置，但每个请求都有所有变量的独立副本，或者说都有各变量用来存放值的容器的独立副本，彼此互不干扰</u>**。比如前面我们请求了`/bar` 接口后，`$foo` 变量被赋予了值 `32`，但它丝毫不会影响后续对 `/foo` 接口的请求所对应的 `$foo` 值（它仍然是空的！），因为各个请求都有自己独立的 `$foo` 变量的副本。



#### 二.内部跳转 echo_exec（属于ngx_echo模块）

```nginx
server {
  listen 8080;

  location /foo {
    set $a hello;
    echo_exec /bar;
  }

  location /bar {
    echo "a = [$a]";
  }
}
```

Result：跳转过来的/bar里面的a变量值变成hello，单独访问依然为空

一个请求在其处理过程中，**<u>即使经历多个不同的 `location` 配置块，它使用的还是同一套 Nginx 变量的副本。这里</u>**，我们也首次涉及到了“内部跳转”这个概念。值得一提的是，标准 [ngx_rewrite](http://wiki.nginx.org/HttpRewriteModule) 模块的 [rewrite](http://wiki.nginx.org/HttpRewriteModule#rewrite) 配置指令其实也可以发起“内部跳转”，例如上面那个例子用 [rewrite](http://wiki.nginx.org/HttpRewriteModule#rewrite) 配置指令可以改写成下面这样的形式：

```nginx
 server {
        listen 8080;
 
        location /foo {
            set $a hello;
            rewrite ^ /bar;
        }
 
        location /bar {
            echo "a = [$a]";
        }
    }
```

##### 结论：Nginx 变量值容器的生命期是与当前正在处理的请求绑定的，而与 `location` 无关。



#### 三.有用户自定义变量，也有内建变量

Nginx 内建变量最常见的用途就是获取关于请求或响应的各种信息。

- 例如由 [ngx_http_core](http://nginx.org/en/docs/http/ngx_http_core_module.html) 模块提供的内建变量 [$uri](http://wiki.nginx.org/HttpCoreModule#.24uri)，可以用来获取当前请求的 URI（经过解码，并且不含请求参数），而 [$request_uri](http://wiki.nginx.org/HttpCoreModule#.24request_uri) 则用来获取请求最原始的 URI （未经解码，并且包含请求参数）；
-  如果你想对 URI 参数值中的 `%XX` 这样的编码序列进行解码，可以使用第三方 [ngx_set_misc](http://wiki.nginx.org/HttpSetMiscModule) 模块提供的 [set_unescape_uri](http://wiki.nginx.org/HttpSetMiscModule#set_unescape_uri) 配置指令；
- 另一个特别常用的内建变量其实并不是单独一个变量，而是有无限多变种的一群变量，即名字以 `arg_` 开头的所有变量，我们估且称之为 [$arg_XXX](http://wiki.nginx.org/HttpCoreModule#.24arg_PARAMETER) 变量群。一个例子是 `$arg_name`，这个变量的值是当前请求名为 `name` 的 URI 参数的值，而且还是未解码的原始形式的值；
- 像 [$arg_XXX](http://wiki.nginx.org/HttpCoreModule#.24arg_PARAMETER) 这种类型的变量拥有无穷无尽种可能的名字，所以它们并不对应任何存放值的容器。而且这种变量在 Nginx 核心中是经过特别处理的，第三方 Nginx 模块是不能提供这样充满魔法的内建变量的。类似 [$arg_XXX](http://wiki.nginx.org/HttpCoreModule#.24arg_PARAMETER) 的内建变量还有不少，比如用来取 cookie 值的 [$cookie_XXX](http://wiki.nginx.org/HttpCoreModule#.24cookie_COOKIE) 变量群，用来取请求头的 [$http_XXX](http://wiki.nginx.org/HttpCoreModule#.24http_HEADER) 变量群，以及用来取响应头的 [$sent_http_XXX](http://wiki.nginx.org/HttpCoreModule#.24sent_http_HEADER) 变量群；
- 许多内建变量都是只读的。

```Nginx
location /test {
    echo "uri = $uri";
    echo "request_uri = $request_uri";
 }
#其实 $arg_name 不仅可以匹配 name 参数，也可以匹配 NAME 参数，抑或是 Name
location /test_name {
    echo "name: $arg_name";
    echo "class: $arg_class";
}

location /test_unescape_name {
    set_unescape_uri $name $arg_name;
    set_unescape_uri $class $arg_class;

    echo "name: $name";
    echo "class: $class";
}
```



#### 四.也有些内建变量的值是可以改的

 也有一些内建变量是支持改写的，其中一个例子是 [$args](http://wiki.nginx.org/HttpCoreModule#.24args). 这个变量在读取时返回当前请求的 URL 参数串（即请求 URL 中问号后面的部分，如果有的话 ），而在赋值时可以直接修改参数串。我们来看一个例子：

```nginx
    location /test {
        set $orig_args $args;
        set $args "a=3&b=4";
 
        echo "original args: $orig_args";
        echo "args: $args";
    }
```

这里我们把原始的 URL 参数串先保存在 `$orig_args` 变量中，然后通过改写 [$args](http://wiki.nginx.org/HttpCoreModule#.24args) 变量来修改当前的 URL 参数串，最后我们用 [echo](http://wiki.nginx.org/HttpEchoModule#echo) 指令分别输出 `$orig_args` 和 [$args](http://wiki.nginx.org/HttpCoreModule#.24args) 变量的值。接下来我们这样来测试这个 `/test`接口：

```nginx
    $ curl 'http://localhost:8080/test'
    original args: 
    args: a=3&b=4
 
    $ curl 'http://localhost:8080/test?a=0&b=1&c=2'
    original args: a=0&b=1&c=2
    args: a=3&b=4
```



#### 五.map缓存,惰性求值

 在设置了“取处理程序”的情况下，**<u>Nginx 变量也可以选择将其值容器用作缓存，这样在多次读取变量的时候，就只需要调用“取处理程序”计算一次。</u>**我们下面就来看一个这样的例子：

```nginx
map $args $foo {
        default     0;
        debug       1;
    }
 #应当注意到 map 指令是在 server 配置块之外，也就是在最外围的 http 配置块中定义的
    server {
        listen 8080;
 
        location /test {
            set $orig_foo $foo;
            set $args debug;
 
            echo "orginal foo: $orig_foo";
            echo "foo: $foo";
        }
    }
```

result：

```nginx
 $ curl 'http://localhost:8080/test?debug'
    original foo: 1
    foo: 1
   
  $ curl 'http://localhost:8080/test'
   original foo: 0
   foo: 0
```

​    很多 Nginx 新手都会担心如此“全局”范围的 [map](http://wiki.nginx.org/HttpMapModule#map) 设置会让访问所有虚拟主机的所有 `location` 接口的请求都执行一遍变量值的映射计算，然而事实并非如此。前面我们已经了解到 [map](http://wiki.nginx.org/HttpMapModule#map) 配置指令的工作原理是为用户变量注册 “取处理程序”，并且实际的映射计算是在“取处理程序”中完成的，**而“取处理程序”只有在该用户变量被实际读取时才会执行（当然，因为缓存的存在，只在请求生命期中的第一次读取中才被执行），所以对于那些根本没有用到相关变量的请求来说，就根本不会执行任何的无用计算。**

​    这种只在实际使用对象时才计算对象值的技术，在计算领域被称为“**惰性求值**”（lazy evaluation）。提供“惰性求值” 语义的编程语言并不多见，最经典的例子便是 Haskell. 与之相对的便是“主动求值” （eager evaluation）。我们有幸在 Nginx 中也看到了“惰性求值”的例子，但“主动求值”语义其实在 Nginx 里面更为常见，例如下面这行再普通不过的 [set](http://wiki.nginx.org/HttpRewriteModule#set) 语句：

```
    set $b "$a,$a";
```