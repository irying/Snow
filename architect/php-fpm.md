

### 总结下CGI，FastCGI, PHP-FPM

严格来说，FastCGI只是一种协议，PHP-FPM是实现这种协议的程序， 功能实现是php-cgi进程的管理器，因为php-cgi作为单纯的CGI程序，本身只能解析请求，返回结果，不会进程管理。



> ### 什么是FastCGI
>
> FastCGI像是一个常驻(long-live)型的CGI，它可以一直执行着，只要激活后，不会**每次都要花费时间去fork一次**(这是CGI最为人诟病的fork-and-execute 模式)。它还支持分布式的运算, 即 FastCGI 程序可以在网站服务器以外的主机上执行并且接受来自其它网站服务器来的请求。
>
> FastCGI是语言无关的、可伸缩架构的CGI开放扩展，其主要行为是将CGI解释器进程保持在内存中并因此获得较高的性能。**众所周知，CGI解释器的反复加载是CGI性能低下的主要原因**，如果CGI解释器保持在内存中并接受FastCGI进程管理器调度，则可以提供良好的性能、伸缩性、Fail- Over特性等等。



参考http://www.phppan.com/2011/01/tipi020203-fastcgi/
http://www.php-internals.com/book/?p=chapt02/02-02-03-fastcgi

### FastCGI的工作原理

1. Web Server启动时载入FastCGI进程管理器（IIS ISAPI或Apache Module)
2. FastCGI进程管理器自身初始化，**启动多个CGI解释器进程**(可见多个php-cgi)并等待来自Web Server的连接。
3. 当客户端请求到达Web Server时，FastCGI进程管理器选择并连接到一个CGI解释器。Web server将CGI环境变量和标准输入发送到FastCGI子进程php-cgi。
4. FastCGI子进程完成处理后将标准输出和错误信息从同一连接返回Web Server。当FastCGI子进程关闭连接时，请求便告处理完成。FastCGI子进程接着等待并处理来自FastCGI进程管理器(运行在Web Server中)的下一个连接。 在CGI模式中，php-cgi在此便退出了。

## PHP中的CGI实现

PHP的cgi实现本质是是以socket编程实现一个tcp或udp协议的服务器，当启动时，创建tcp/udp协议的服务器的socket监听，并接收相关请求进行处理。这只是请求的处理，在此基础上添加模块初始化，sapi初始化，模块关闭，sapi关闭等就构成了整个cgi的生命周期。 程序是从cgi_main.c文件的main函数开始，而在main函数中调用了定义在fastcgi.c文件中的初始化，监听等函数。

> 从main函数开始，看看PHP对于fastcgi的实现。
>
> 将整个流程分为初始化操作，请求处理，关闭操作三个部分。 

### 初始化

对应TCP的操作流程，PHP首先会执行创建socket，绑定套接字，创建监听：

```C
if (bindpath) {
    fcgi_fd = fcgi_listen(bindpath, 128);   //  实现socket监听，调用fcgi_init初始化
    ...
}
```

在fastcgi.c文件中，fcgi_listen函数主要用于创建、绑定socket并开始监听，它走完了前面所列TCP流程的前三个阶段，

```
 if ((listen_socket = socket(sa.sa.sa_family, SOCK_STREAM, 0)) < 0 ||
        ...
        bind(listen_socket, (struct sockaddr *) &sa, sock_len) < 0 ||
        listen(listen_socket, backlog) < 0) {
        ...
    }
```

### 请求处理

当服务端初始化完成后，进程调用accept函数进入阻塞状态，在main函数中我们看到如下代码：

```
     do {
            pid = fork();   //  生成新的子进程
            switch (pid) {
            case 0: //  子进程
                parent = 0;
 
                /* don't catch our signals */
                sigaction(SIGTERM, &old_term, 0);   //  终止信号
                sigaction(SIGQUIT, &old_quit, 0);   //  终端退出符
                sigaction(SIGINT,  &old_int,  0);   //  终端中断符
                break;
                ...
                default:
                /* Fine */
                running++;
                break;
        } while (parent && (running < children));
 
    ...
        while (!fastcgi || fcgi_accept_request(&request) >= 0) {
        SG(server_context) = (void *) &request;
        init_request_info(TSRMLS_C);
        CG(interactive) = 0;
                    ...
            }
```

**如上的代码是一个生成子进程，并等待用户请求。在fcgi_accept_request函数中，程序会调用accept函数阻塞新创建的进程。** 当用户的请求到达时，fcgi_accept_request函数会判断是否处理用户的请求，其中会过滤某些连接请求，忽略受限制客户的请求， 如果程序受理用户的请求，它将分析请求的信息，将相关的变量写到对应的变量中。 其中在读取请求内容时调用了safe_read方法。

**[main() -> fcgi_accept_request() -> fcgi_read_request() -> safe_read()]**

```
在请求初始化完成，读取请求完毕后，就该处理请求的PHP文件了。 假设此次请求为PHP_MODE_STANDARD则会调用php_execute_script执行PHP文件。 在此函数中它先初始化此文件相关的一些内容，然后再调用zend_execute_scripts函数，对PHP文件进行词法分析和语法分析，生成中间代码， 并执行zend_execute函数，从而执行这些中间代码。
```

### 关闭

在处理完用户的请求后，服务器端将返回信息给客户端，此时在main函数中调用的是fcgi_finish_request(&request, 1); fcgi_finish_request函数定义在fastcgi.c文件中，其代码如下：

```
int fcgi_finish_request(fcgi_request *req, int force_close)
{
int ret = 1;
 
if (req->fd >= 0) {
    if (!req->closed) {
        ret = fcgi_flush(req, 1);
        req->closed = 1;
    }
    fcgi_close(req, force_close, 1);
}
return ret;
}
```



如上，**当socket处于打开状态，并且请求未关闭，则会将执行后的结果刷到客户端，并将请求的关闭设置为真。** 将数据刷到客户端的程序调用的是fcgi_flush函数。在此函数中，关键是在于答应头的构造和写操作。 程序的写操作是调用的safe_write函数，而safe_write函数中对于最终的写操作针对win和linux环境做了区分， 在Win32下，如果是TCP连接则用send函数，如果是非TCP则和非win环境一样使用write函数。如下代码：

```
#ifdef _WIN32
if (!req->tcp) {
    ret = write(req->fd, ((char*)buf)+n, count-n);
} else {
    ret = send(req->fd, ((char*)buf)+n, count-n, 0);
    if (ret <= 0) {
            errno = WSAGetLastError();
    }
}
#else
ret = write(req->fd, ((char*)buf)+n, count-n);
#endif
```

在发送了请求的应答后，**服务器端将会执行关闭操作，仅限于CGI本身的关闭，程序执行的是fcgi_close函数。** fcgi_close函数在前面提的fcgi_finish_request函数中，在请求应答完后执行。同样，对于win平台和非win平台有不同的处理。 其中对于非win平台调用的是write函数。

```
...
php_request_shutdown((void *) 0);   //  php请求关闭函数
...
fcgi_shutdown();    //  fcgi的关闭 销毁fcgi_mgmt_vars变量
php_module_shutdown(TSRMLS_C);  //  模块关闭    清空sapi,关闭zend引擎 销毁内存，清除垃圾等
sapi_shutdown();    //  sapi关闭  sapi全局变量关闭等
...
```



### 现在到PHP-FPM

参考 http://blog.csdn.net/Mijar2016/article/details/53414402

```
PHP-CGI
PHP-CGI is one kind of the Process Manager of FastCGI, which is within php itself.
The command to boot is as follow:
php-cgi -b 127.0.0.1:9000
shortcuts

After changing php.ini, you should reboot PHP-CGI to make the new php.ini work.
When a PHP-CGI process is killed, all the PHP code will cannot run.(PHP-FPM and Spawn-FCGI do not have the same problem)

PHP-FPM
PHP-FPM is another kind of the Process Manager of FastCGI, which can be downloaded here.
It's actually a patch for PHP, which is used to integrate the Process Manager of FastCGI into PHP, which should be make into PHP before version 5.3.2.
PHP-FPM can be used to control sub processes of PHP-CGI:
/usr/local/php/sbin/php-fpm [options]

# options
# --start: start a fastcgi process of php
# --stop: force to kill a fastcgi process of php
# --quit: smooth to kill a fastcgi process of php
# --restart: restart a fastcgi process of php
# --reload: smooth to reload php.ini
# --logrotate: enable log files again

```

下面笔记来自http://www.jianshu.com/p/542935a3bfa8

```
FPM多进程轮廓：
FPM大致的多进程模型就是：一个master进程,多个worker进程.
master进程负责管理调度，worker进程负责处理客户端(nginx)的请求.
master负责创建并监听(listen)网络连接，worker负责接受(accept)网络连接.
对于一个工作池，只有一个监听socket, 多个worker共用一个监听socket.

master进程与worker进程之间，通过信号(signals)和管道(pipe)通信．
FPM支持多个工作池(worker pool), FPM的工作池可以简单的理解为监听多个网络的多个FPM实例，只不过多个池都由一个master进程管理．
这里只考虑一个工作池的情况，理解了一个工作池，多个工作池也容易.
```

- **master进程负责管理调度，worker进程负责处理客户端(nginx)的请求.**
  **master负责创建并监听(listen)网络连接，worker负责接受(accept)网络连接.**
- **FPM支持多个工作池(worker pool), FPM的工作池可以简单的理解为监听多个网络的多个FPM实例，只不过多个池都由一个master进程管理．**

```
由于FPM 默认是以守护进程方式运行，这里做个简单的介绍：
为了和控制台tty分离，fpm启动进程，会创建子进程（这个子进程就是后来的master进程）
启动进程创建一个管道pipe 用于和子进程通信，子进程完成初始化后，会通过这个管道给启动进程发消息，
启动进程收到消息后，简单处理后退出，由这个子进程负责后续工作．
平时，我们看到的 fpm master进程,其实是第一个子进程．
```

文件fpm_main.c 里的main函数，是fpm服务启动入口，依次调用函数：
main -> fpm_init -> fpm_unix_init_main ,代码如下：

```
//fpm_unix.c
if (fpm_global_config.daemonize) {
    ．．．
        if (pipe(fpm_globals.send_config_pipe) == -1) {
            zlog(ZLOG_SYSERROR, "failed to create pipe");
            return -1;
        }
        /* then fork */
        pid_t pid = fork();
　   ．．．
｝

```

### worker进程的创建

worker进程创建函数为fpm_children_make，依据fpm配置pm = static 或 ondemand 或 dynamic有三种创建worker进程的情况：

```
static: 启动时创建：main -> fpm_run -> fpm_children_create_initial ->  fpm_children_make

ondemand: 按需创建，有请求才创建．启动时，注册创建事件．事件的细节是：监听socket(listening_socket) 可读时：调用创建函数 fpm_pctl_on_socket_acceptmain -> fpm_run -> fpm_children_create_initial

dynamic: 依据配置动态创建．
fpm_pctl_perform_idle_server_maintenance -> fpm_children_make
fpm_pctl_perform_idle_server_maintenance 会定时重复运行，依据配置创建worker进程
启动时这个逻辑会加到timer队列．
后面两个ondemand和dynamic是把创建逻辑加队列里，一个是IO事件，一个是timer队列．
有条件触发，有连接或是运行时间到．
两者都是在fpm_event_loop函数内部触发运行．

```

...

1. cgi初始化阶段：分别调用`fcgi_init()`和 `sapi_startup()`函数，注册进程信号以及初始化sapi_globals全局变量。 
2. php环境初始化阶段：由`cgi_sapi_module.startup` 触发。实际调用`php_cgi_startup`函数，而php_cgi_startup内部又调用`php_module_startup`执行。 php_module_startup主要功能：a).加载和解析php配置；b).加载php模块并记入函数符号表(function_table)；c).加载zend扩展 ; d).设置禁用函数和类库配置；e).注册回收内存方法； 
3. php-fpm初始化阶段：执行`fpm_init()`函数。负责解析php-fpm.conf文件配置，获取进程相关参数（允许进程打开的最大文件数等）,初始化进程池及事件模型等操作。 
4. php-fpm运行阶段：执行`fpm_run()` 函数，运行后主进程发生阻塞。该阶段分为两部分：fork子进程 和 循环事件。fork子进程部分交由`fpm_children_create_initial`函数处理（ **注：ondemand模式在fpm_pctl_on_socket_accept函数创建**）。循环事件部分通过`fpm_event_loop`函数处理，其内部是一个死循环，负责事件的收集工作。