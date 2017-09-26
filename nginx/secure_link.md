来自：http://www.ttlsa.com/nginx/nginx-modules-secure_link/

先安装

```
 # ./configure --with-http_secure_link_module \
 --prefix=/usr/local/nginx-1.4.2 --with-http_stub_status_module
 # make
 # make install
```

配置

```c
server {
    listen       80;
    server_name  s1.down.ttlsa.com;
    access_log  /data/logs/nginx/s1.down.ttlsa.com.access.log  main;
 
    index index.html index.php index.html;
    root /data/site/s1.down.ttlsa.com;
 
    location / {
        secure_link $arg_st,$arg_e;
        secure_link_md5 ttlsa.com$uri$arg_e;
 
        if ($secure_link = "") {
            return 403;
        }
 
        if ($secure_link = "0") {
            return 403;
        }
    }
}
```

php下载页面

```c
<?php
 # 作用：生成nginx secure link链接
 # 站点：www.ttlsa.com
 # 作者：凉白开
 # 时间：2013-09-11
$secret = 'ttlsa.com'; # 密钥
 $path = '/web/nginx-1.4.2.tar.gz'; # 下载文件
 # 下载到期时间,time是当前时间,300表示300秒,也就是说从现在到300秒之内文件不过期
 $expire = time()+300;
# 用文件路径、密钥、过期时间生成加密串
 $md5 = base64_encode(md5($secret . $path . $expire, true));
 $md5 = strtr($md5, '+/', '-_');
 $md5 = str_replace('=', '', $md5);
# 加密后的下载地址
 echo '<a href=http://s1.down.ttlsa.com/web/nginx-1.4.2.tar.gz?st='.$md5.'&e='.$expire.'>nginx-1.4.2</a>';
 echo '<br>http://s1.down.ttlsa.com/web/nginx-1.4.2.tar.gz?st='.$md5.'&e='.$expire;
 ?>
```

### 测试nginx防盗链

打开http://test.ttlsa.com/down.php点击上面的连接下载
下载地址如下：
http://s1.down.ttlsa.com/web/nginx-1.4.2.tar.gz?st=LSVzmZllg68AJaBmeK3E8Q&e=1378881984
页面不要刷新，等到5分钟后在下载一次，你会发现点击下载会跳转到403页面。

### secure link 防盗链原理

- 用户访问down.php


- <u>**down.php根据secret密钥、过期时间、文件uri生成加密串**</u>


- <u>**将加密串与过期时间作为参数跟到文件下载地址的后面**</u>


- nginx下载服务器接收到了过期时间，也使用过期时间、配置里密钥、文件uri生成加密串


- 将用户传进来的加密串与自己生成的加密串进行对比，一致允许下载，不一致403

整个过程实际上很简单，类似于用户密码验证. 尤为注意的一点是大家一定不要泄露了自己的密钥，否则别人就可以盗链了，除了泄露之外最好能经常更新密钥.



```
secure link 指令

secure_link
语法: secure_link md5_hash[,expiration_time]
默认: none
配置段: location
variables: yes
这个指令由uri中的MD5哈希值和过期时间组成. md5哈希必须由base64加密的,过期时间为unix时间.如果不加过期时间,那么这个连接永远都不会过期.
secure_link_md5
语法: secure_link_md5 secret_token_concatenated_with_protected_uri
默认: none
配置段: location
variables: yes
md5值对比结果,使用上面提供的uri、密钥、过期时间生成md5哈希值.如果它生成的md5哈希值与用户提交过来的哈希值一致，那么这个变量的值为1，否则为0
```