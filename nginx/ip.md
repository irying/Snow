## Nginx GeoIP 用户访问城市地区位置信息

来自 http://www.linuxhub.org/?p=3854

Nginx GeoIP判断城市地区

**1.所需两个组件库模块：**
1.1.Nginx需要添加ngx_http_geoip_module模块
1.2.安装GeoIP library

2.模块与参数介绍
**2.1.ngx_http_geoip_module**
ngx_http_geoip_module模块创建变量，使用预编译的MaxMind数据库解析客户端IP地址，得到变量值，
然后根据变量的值去匹配判断，所以要模块依赖MaxMind GeoIP库，GeoIP数据库支持两种格式CSV格式和二进制格式。

**2.2.geoip_country**
语法：geoip_country path/to/db.dat;
默认值：none
使用字段：http
这个指令为IP地址对应国家的.dat数据库文件指定完整的路径，这样就可以通过客户端的IP来获取地理信息，设置这个值以后可以使用下列变量：

```C
$geoip_country_code; - 两个字母的国家代码，如：”RU”, “US”。 
$geoip_country_code3; - 三个字母的国家代码，如：”RUS”, “USA”。 
$geoip_country_name; - 国家的完整名称，如：”Russian Federation”, “United States”。
```

Nginx将这个数据库缓存到内存中，IP对应国家的数据库很小，大约只有1.4M，所以并不会占用多大内存，但是城市的数据库很大，因此会带来更大的内存占用。

**2.3.geoip_city**
语法：geoip_city path/to/db.dat;
默认值：none
使用字段：http
这个指令为IP地址对应国家，城市以及地区的.dat数据库文件指定完整的路径，这样就可以通过客户端的IP来获取地理信息，设置这个值以后可以使用下列变量：

```
$geoip_country_code -  两个字母的国家代码，如：”RU”, “US”。 
$geoip_country_code3 - 三个字母的国家代码，如：”RUS”, “USA”。 
$geoip_country_name - 国家的完整名称，如：”Russian Federation”, “United States”（如果可用）。 
$geoip_region - 地区的名称（类似于省，地区，州，行政区，联邦土地等），如：”30”。 30代码就是广州的意思  
$geoip_city - 城市名称，如”Guangzhou”, “ShangHai”（如果可用）。
$geoip_postal_code - 邮政编码。
$geoip_city_continent_code。 
$geoip_latitude - 所在维度。 
$geoip_longitude - 所在经度。
```

作者：LinuxHub   发布：2016-06-21 16:49   分类：[Nginx](http://www.linuxhub.org/?cat=76)    

Nginx GeoIP判断城市地区

**1.所需两个组件库模块：**
1.1.Nginx需要添加ngx_http_geoip_module模块
1.2.安装GeoIP library

2.模块与参数介绍
**2.1.ngx_http_geoip_module**
ngx_http_geoip_module模块创建变量，使用预编译的MaxMind数据库解析客户端IP地址，得到变量值，
然后根据变量的值去匹配判断，所以要模块依赖MaxMind GeoIP库，GeoIP数据库支持两种格式CSV格式和二进制格式。

**2.2.geoip_country**
语法：geoip_country path/to/db.dat;
默认值：none
使用字段：http
这个指令为IP地址对应国家的.dat数据库文件指定完整的路径，这样就可以通过客户端的IP来获取地理信息，设置这个值以后可以使用下列变量：

| 123  | $geoip_country_code; - 两个字母的国家代码，如：”RU”, “US”。 $geoip_country_code3; - 三个字母的国家代码，如：”RUS”, “USA”。 $geoip_country_name; - 国家的完整名称，如：”Russian Federation”, “United States”。 |
| ---- | ---------------------------------------- |
|      |                                          |

Nginx将这个数据库缓存到内存中，IP对应国家的数据库很小，大约只有1.4M，所以并不会占用多大内存，但是城市的数据库很大，因此会带来更大的内存占用。

**2.3.geoip_city**
语法：geoip_city path/to/db.dat;
默认值：none
使用字段：http
这个指令为IP地址对应国家，城市以及地区的.dat数据库文件指定完整的路径，这样就可以通过客户端的IP来获取地理信息，设置这个值以后可以使用下列变量：

| 123456789 | $geoip_country_code -  两个字母的国家代码，如：”RU”, “US”。 $geoip_country_code3 - 三个字母的国家代码，如：”RUS”, “USA”。 $geoip_country_name - 国家的完整名称，如：”Russian Federation”, “United States”（如果可用）。 $geoip_region - 地区的名称（类似于省，地区，州，行政区，联邦土地等），如：”30”。 30代码就是广州的意思  $geoip_city - 城市名称，如”Guangzhou”, “ShangHai”（如果可用）。$geoip_postal_code - 邮政编码。$geoip_city_continent_code。 $geoip_latitude - 所在维度。 $geoip_longitude - 所在经度。 |
| --------- | ---------------------------------------- |
|           |                                          |

**3.安装 GeoIP library **(在Nginx添加模块前安装)
安装方法: [http://www.linuxhub.org/?p=3843](http://www.linuxhub.org/?p=3843)
如果不安装，Nginx编译时会报以下错误:
./configure: error: the GeoIP module requires the GeoIP library.
You can either do not enable the module or install the library.

**4.Nginx添加ngx_http_geoip_modules模块**
模块添加方法: [http://www.linuxhub.org/?p=3220](http://www.linuxhub.org/?p=3220)

 --with-http_geoip_module 

**添加个测试日志格式**，增加

```
（$geoip_country_name $geoip_region $geoip_city）

log_format access ‘$remote_addr – $remote_user [$time_local] “$request” ‘

‘$status $body_bytes_sent “$http_referer” ‘

‘”$http_user_agent” $http_x_forwarded_for’

‘$geoip_country_name $geoip_region $geoip_city'

```



## PHP获取用户的真实的IP

```php
**
 * 获得用户的真实IP地址
 *
 * @access  public
 * @return  string
 */
function real_ip()
{
    static $realip = NULL;
 
    if ($realip !== NULL)
    {
        return $realip;
    }
 
    if (isset($_SERVER))
    {
        if (isset($_SERVER['HTTP_X_FORWARDED_FOR']))
        {
            $arr = explode(',', $_SERVER['HTTP_X_FORWARDED_FOR']);
 
            /* 取X-Forwarded-For中第一个非unknown的有效IP字符串 */
            foreach ($arr AS $ip)
            {
                $ip = trim($ip);
 
                if ($ip != 'unknown')
                {
                    $realip = $ip;
 
                    break;
                }
            }
        }
        elseif (isset($_SERVER['HTTP_CLIENT_IP']))
        {
            $realip = $_SERVER['HTTP_CLIENT_IP'];
        }
        else
        {
            if (isset($_SERVER['REMOTE_ADDR']))
            {
                $realip = $_SERVER['REMOTE_ADDR'];
            }
            else
            {
                $realip = '0.0.0.0';
            }
        }
    }
    else
    {
        if (getenv('HTTP_X_FORWARDED_FOR'))
        {
            $realip = getenv('HTTP_X_FORWARDED_FOR');
        }
        elseif (getenv('HTTP_CLIENT_IP'))
        {
            $realip = getenv('HTTP_CLIENT_IP');
        }
        else
        {
            $realip = getenv('REMOTE_ADDR');
        }
    }
 
    preg_match("/[\d\.]{7,15}/", $realip, $onlineip);
    $realip = !empty($onlineip[0]) ? $onlineip[0] : '0.0.0.0';
 
    return $realip;
```

顺便说下$_SERVER和getenv的区别，getenv不支持IIS的isapi方式运行的php

一、没有使用代理服务器的情况：

​      **REMOTE_ADDR = 您的 IP**
​      **HTTP_VIA = 没数值或不显示**
​      **HTTP_X_FORWARDED_FOR = 没数值或不显示**

二、使用透明代理服务器的情况：Transparent Proxies

​      REMOTE_ADDR = 最后一个代理服务器 IP
​      HTTP_VIA = 代理服务器 IP
​      **HTTP_X_FORWARDED_FOR = 您的真实 IP ，经过多个代理服务器时，这个值类似如下：203.98.182.163, 203.98.182.163, 203.129.72.215。**

  这类代理服务器还是将您的信息转发给您的访问对象，无法达到隐藏真实身份的目的。

三、使用普通匿名代理服务器的情况：Anonymous Proxies

​      REMOTE_ADDR = 最后一个代理服务器 IP
​      HTTP_VIA = 代理服务器 IP
​      **HTTP_X_FORWARDED_FOR = 代理服务器 IP ，经过多个代理服务器时，这个值类似如下：203.98.182.163, 203.98.182.163, 203.129.72.215。**

 隐藏了您的真实IP，但是向访问对象透露了您是使用代理服务器访问他们的。

四、使用欺骗性代理服务器的情况：Distorting Proxies

​      REMOTE_ADDR = 代理服务器 IP
​      HTTP_VIA = 代理服务器 IP
​      HTTP_X_FORWARDED_FOR = 随机的 IP ，经过多个代理服务器时，这个值类似如下：203.98.182.163, 203.98.182.163, 203.129.72.215。

  告诉了访问对象您使用了代理服务器，但编造了一个虚假的随机IP代替您的真实IP欺骗它。

五、使用高匿名代理服务器的情况：High Anonymity Proxies (Elite proxies)

​      REMOTE_ADDR = 代理服务器 IP
​      HTTP_VIA = 没数值或不显示
​      HTTP_X_FORWARDED_FOR = 没数值或不显示 ，经过多个代理服务器时，这个值类似如下：203.98.182.163, 203.98.182.163, 203.129.72.215。

  完全用代理服务器的信息替代了您的所有信息，就象您就是完全使用那台代理服务器直接访问对象。

REMOTE_ADDR 是你的客户端跟你的服务器“握手”时候的IP。如果使用了“匿名代理”，REMOTE_ADDR将显示代理服务器的IP。
HTTP_CLIENT_IP 是代理服务器发送的HTTP头。如果是“超级匿名代理”，则返回none值。同样，REMOTE_ADDR也会被替换为这个代理服务器的IP。
$_SERVER['REMOTE_ADDR']; //访问端（有可能是用户，有可能是代理的）IP
$_SERVER['HTTP_CLIENT_IP'];  //代理端的（有可能存在，可伪造）
$_SERVER['HTTP_X_FORWARDED_FOR']; //用户是在哪个IP使用的代理（有可能存在，也可以伪造）



#### PHP里用来获取客户端IP的变量有这些:[#](https://laravel-china.org/topics/3905/do-you-really-know-ip-php-how-to-get-real-user-ip#PHP里用来获取客户端IP的变量有这些)

- `$_SERVER['HTTP_CLIENT_IP']`这个头是有的，但是很少，不一定服务器都实现了。客户端可以伪造。

- `$_SERVER['HTTP_X_FORWARDED_FOR']` 是有标准定义，用来识别经过HTTP代理后的客户端IP地址，格式：clientip,proxy1,proxy2。详细解释见 [http://zh.wikipedia.org/wiki/X-Forwarded-For](http://zh.wikipedia.org/wiki/X-Forwarded-For)。 客户端可以伪造。

- `$_SERVER['REMOTE_ADDR']` 是可靠的， 它是最后一个跟你的服务器握手的IP，可能是用户的代理服务器，也可能是自己的反向代理。客户端不能伪造。

  **客户端可以伪造的参数必须过滤和验证！**很多人以为$_SERVER变量里的东西都是可信的，其实并不不然，`$_SERVER['HTTP_CLIENT_IP']`和`$_SERVER['HTTP_X_FORWARDED_FOR']`都来自客户端请求的header里面。

#### 如果要严格获取用户真实ip[#](https://laravel-china.org/topics/3905/do-you-really-know-ip-php-how-to-get-real-user-ip#如果要严格获取用户真实ip)

在反爬虫，防刷票的时候，客户端可以伪造的东西，我们一律不信任，此为严格获取。

1. 没有套CDN，用户直连我们的PHP服务器

   这种情况下用tcp层握手的ip，`$_SERVER['REMOTE_ADDR']`

2. 自建集群用nginx实现负载均衡的时候

   **这种情况下，PHP应用服务器不能对外暴露，我们在nginx中实现获取真实IP再换发给PHP服务器。**

   ```
   location /{
      proxy_set_header client-real-ip $remote_addr;
   }
   ```

   client-real-ip 可以随意自己命名，我们将tcp层中跟nginx握手的ip转发给PHP。

3. 使用CDN，从PHP服务器取源的时候

   CDN会转发客户端的握手ip过来，各家策略有差异，具体去查CDN的文档。

   当然我们也可以把需要严格核查的业务绑一个二级域名，单独走我们自己的nginx服务器，避开CDN。

#### 如果要宽松获取用户ip[#](https://laravel-china.org/topics/3905/do-you-really-know-ip-php-how-to-get-real-user-ip#如果要宽松获取用户ip)

这种情况比较简单，也是大部分开源程序使用的方式，因为他们要适应最广泛的部署环境，
依次获取和过滤，`$_SERVER['HTTP_CLIENT_IP']`，`$_SERVER['HTTP_X_FORWARDED_FOR']`的第一个ip，`$_SERVER['REMOTE_ADDR']`，谁先有值先用谁。**注意这种方式，客户端可以提交假ip来欺骗服务器。**

#### PHP如何验证和过滤客户端提交过来的ip[#](https://laravel-china.org/topics/3905/do-you-really-know-ip-php-how-to-get-real-user-ip#PHP如何验证和过滤客户端提交过来的ip)

推荐使用PHP自带的过滤器，[http://php.net/manual/zh/function.filter-var.php](http://php.net/manual/zh/function.filter-var.php)

```
$ip = filter_var($originIp, FILTER_VALIDATE_IP)
```

#### 一点小技巧[#](https://laravel-china.org/topics/3905/do-you-really-know-ip-php-how-to-get-real-user-ip#一点小技巧)

我们存IP到数据库的时候可以使用`ip2long()`把ip地址转换成数字，搜索和排序可以更快。要显示到前端的时候再`long2ip()`转换回来

加个博客链接：[https://blog.haitun.me/get-real-client-ip/](https://blog.haitun.me/get-real-client-ip/)



## 阿里云文档

### 使用“Web应用防火墙”后获取访问者真实IP配置指南

很多时候，网站并不是简单的从用户的浏览器直达服务器， 中间可能会加入CDN、WAF、高防。

变成如下的架构：

**用户 —–> CDN/WAF/高防 ——> 源站服务器**

那么，经过这么多层加速，服务器如何才能得到发起请求的真实客户端IP呢？

当一个透明代理服务器把用户的请求转到后面服务器的时候，会在HTTP的头中加入一个记录：

X-Forwarded-For:用户真实IP

如果中间经历了不止一个代理服务器，那么X-Forwarded-For可能会为以下的形式：

X-Forwarded-For:用户IP, 代理服务器1-IP, 代理服务器2-IP, 代理服务器3-IP, ……

那么如何获取X-Forwarded-For的内容？

# 获取来访真实IP方法

常见应用服务器获取来访者真实IP的方法.

- 使用X-Forwarded-For的方式获取访问者真实IP

以下针对常见的应用服务器配置方案进行介绍：

## Nginx配置方案

**1. 确认http_realip_module模块已安装**

Nginx作为负载均衡获取真实ip是使用http_realip_module，默认一键安装包安装的Nginx是没有安装这个模块的，但一般服务器都会安装。可以使用：

`# nginx -V | grep http_realip_module` 查看有无安装。

如没有，则需要重新重新编译Nginx并加装：

```
wget http://soft.phpwind.me/top/nginx-1.0.12.tar.gztar zxvf nginx-1.0.12.tar.gzcd nginx-1.0.12./configure --user=www --group=www --prefix=/alidata/server/nginx --with-http_stub_status_module --without-http-cache --with-http_ssl_module --with-http_realip_modulemakemake installkill -USR2 `cat /alidata/server/nginx/logs/nginx.pid`kill -QUIT `cat /alidata/server/nginx/logs/ nginx.pid.oldbin`
```

**2. 修改nginx对应server的配置（如默认的是default.conf）**

在`location / {}`中添加：

```
set_real_ip_from ip_range1;
set_real_ip_from ip_range2;
...
set_real_ip_from ip_rangex;
real_ip_header    X-Forwarded-For;
```

这里的ip_range1,2,…指的是WAF的回源IP地址，需要添加多条。

**3. 修改日志记录格式 log_format**

log_format一般在nginx.conf中的http配置中：

```
log_format  main  '$http_x_forwarded_for - $remote_user [$time_local] "$request" ' '$status $body_bytes_sent "$http_referer" ' '"$http_user_agent" ';
```

即将x-forwarded-for字段加进去，替换掉原来的remote-address，添加后效果如下图：

![log_format](http://docs-aliyun.cn-hangzhou.oss.aliyun-inc.com/assets/pic/42205/cn_zh/1483686764717/Nginx-log_format.png)

最后重启Nginx使配置生效：`nginx -s reload`