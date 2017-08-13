PHP-FPM有3种对子进程的管理方式

#### pm = static

一种是`pm = static`，始终保持一个固定数量的子进程，这个数由`pm.max_children`定义，这种方式很不灵活，也通常不是默认的。



#### pm = dynamic

另一种是`pm = dynamic`，他是这样的，启动时，会产生固定数量的子进程（由`pm.start_servers`控制）可以理解成最小子进程数，而最大子进程数则由`pm.max_children`去控制，OK，这样的话，子进程数会在最大和最小数范围中变化，还没有完，闲置的子进程数还可以由另2个配置控制，分别是`pm.min_spare_servers`和`pm.max_spare_servers`，也就是闲置的子进程也可以有最小和最大的数目，**<u>而如果闲置的子进程超出了pm.max_spare_servers，则会被杀掉。</u>**

可以看到，`pm = dynamic`模式非常灵活，也通常是默认的选项。但是，`dynamic`模式为了最大化地优化服务器响应，会造成更多内存使用，因为这种模式只会杀掉超出最大闲置进程数（`pm.max_spare_servers`）的闲置进程，比如最大闲置进程数是30，最大进程数是50，然后网站经历了一次访问高峰，此时50个进程全部忙碌，0个闲置进程数，接着过了高峰期，可能没有一个请求，于是会有50个闲置进程，但是此时`php-fpm`只会杀掉20个子进程，<u>**始终剩下30个进程继续作为闲置进程来等待请求，这可能就是为什么过了高峰期后即便请求数大量减少服务器内存使用却也没有大量减少，也可能是为什么有些时候重启下服务器情况就会好很多，因为重启后，php-fpm的子进程数会变成最小闲置进程数，而不是之前的最大闲置进程数。**</u>



#### pm = ondemand

第三种就是[这篇文章](https://ma.ttias.be/a-better-way-to-run-php-fpm/)中提到的`pm = ondemand`模式，这种模式和`pm = dynamic`相反，把内存放在第一位，他的工作模式很简单，每个闲置进程，在持续闲置了`pm.process_idle_timeout`秒后就会被杀掉，有了这个模式，到了服务器低峰期内存自然会降下来，如果服务器长时间没有请求，就只会有一个`php-fpm`主进程，当然弊端是，**<u>遇到高峰期或者如果pm.process_idle_timeout的值太短的话，无法避免服务器频繁创建进程的问题</u>**，因此`pm = dynamic`和`pm = ondemand`谁更适合视实际情况而定。



延伸：https://mp.weixin.qq.com/s/7jfBdiZf2-NQJRfQdbUw2A

为了避免`CPU_IDLE`和`MEM_USED`周期波动，同时保持`max_requests`机制，需要在PHP-FPM源码上稍作修改。FastCGI进程在启动时，设置`max_requests`，此时只要将`max_requests`配置参数散列开，使FastCGI进程分别配置不同的值，即可达到效果。

具体代码在`sapi/fpm/fpm/fpm.c`，修改如下：

```C
php_mt_srand(GENERATE_SEED());

*max_requests=fpm_globals.max_requests+php_mt_rand()&8191;
```

至此`CPU_IDLE`和`MEM_USED`已经告别了周期性波动，避免了CPU计算资源产生浪涌效果，内存占用数据也更加真实可靠。