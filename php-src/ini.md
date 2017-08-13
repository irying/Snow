PHP总共有4个配置指令作用域，分别是PHP_INI_USER，PHP_INI_PERDIR，PHP_INI_SYSTEM，PHP_INI_ALL。这些作用域限制了指令是否可以被修改，在那里可以被修改。php的每个配置项都会有一个作用域。下面是对四种作用域的说明。



```C
作用域类型         说明
PHP_INI_USER     可在用户脚本(如 ini_set())或Windows注册表(PHP 5.3 起)以及.user.ini中设定
PHP_INI_PERDIR   可在 php.ini，.htaccess 或 httpd.conf 中设定
PHP_INI_SYSTEM   可在 php.ini 或 httpd.conf 中设定
PHP_INI_ALL      可在任何地方设定
```

例如 **output_buffering 指令是属于 PHP_INI_PERDIR，因而就不能用 ini_set() 来设定**。但是 display_errors 指令是属于 PHP_INI_ALL 因而就可以在任何地方被设定，包括 ini_set()。



```C
#include "php_ini.h" 
PHP_INI_BEGIN()
    PHP_INI_ENTRY("sample4.greeting", "Hello World", PHP_INI_ALL, NULL)
PHP_INI_END()
```



PHP_INI_ENTRY 这个宏里面设置的前面的两个参数，分别代表着INI设置的名称和它的默认值。第二个参数决定设置是否允许被修改，以及它能被修改的作用域。最后**<u>一个参数是一个回调函数</u>**，当INI的值被修改时候触发此回调函数。你将会在某些修改事件的地方详细的了解这个参数。

把刚刚设置的ini值用到函数里面

```C
PHP_FUNCTION(sample4_hello_world) 
{
    const char *greeting = INI_STR("sample4.greeting");
    php_printf("%s\n", greeting); 
}
```



有一点很重要：`char*`类型的值被认为是属于`ZEND`引擎的，是不能修改的。正因为如此，你将本地变量设置进INI时候，它将在你的方法中被声明为`const`。并不是所有的INI值都是基于字符串的；也有其他的一些用于整数、浮点数、或布尔值的宏。







### auto_prepend和auto_append_file

**auto_prepend_file与auto_append_file使用方法**

如果需要将文件require到所有页面的顶部与底部。

**第一种方法：**在所有页面的顶部与底部都加入require语句。

例如：

1. require('header.php');  
2. 页面内容  
3. require('footer.php');  

但这种方法如果需要修改顶部或底部require的文件路径，则需要修改所有页面文件。而且需要每个页面都加入require语句，比较麻烦。

**第二种方法：**使用auto_prepend_file与auto_append_file在所有页面的顶部与底部require文件。

[PHP](http://lib.csdn.net/base/php).ini中有两项

**auto_prepend_file** 在页面顶部加载文件

**auto_append_file**  在页面底部加载文件

使用这种方法可以不需要改动任何页面，当需要修改顶部或底部require文件时，只需要修改auto_prepend_file与auto_append_file的值即可。

例如：修改[php](http://lib.csdn.net/base/php).ini，修改auto_prepend_file与auto_append_file的值。

1. auto_prepend_file = "/home/fdipzone/header.php"  
2. auto_append_file = "/home/fdipzone/footer.php"  

修改后重启服务器，这样所有页面的顶部与底部都会require /home/fdipzone/header.php 与 /home/fdipzone/footer.php

注意：auto_prepend_file 与 auto_append_file 只能require一个php文件，但这个php文件内可以require多个其他的php文件。

如果不需要所有页面都在顶部或底部require文件，可以指定某一个文件夹内的页面文件才调用auto_prepend_file与auto_append_file

在需要顶部或底部加载文件的文件夹中加入.htaccess文件，内容如下：

1. php_value auto_prepend_file "/home/fdipzone/header.php"  
2. php_value auto_append_file "/home/fdipzone/footer.php"  

这样在指定.htaccess的文件夹内的页面文件才会加载 /home/fdipzone/header.php 与 /home/fdipzone/footer.php，其他页面文件不受影响。

使用.htaccess设置，比较灵活，不需要重启服务器，也不需要管理员权限，唯一缺点是目录中每个被读取和被解释的文件每次都要进行处理，而不是在启动时处理一次，所以性能会有所降低。

#### 如果是nginx+php组合，可以加入如下指令

```
fastcgi_param PHP_VALUE "auto_prepend_file=/home/www/bo56.com/header.php";
```

注意，nginx中多次使用 PHP_VALUE时，最后的一个会覆盖之前的。如果想设置多个配置项，需要写在一起，然后用换行分割。如：

```c
fastcgi_param PHP_VALUE "auto_prepend_file=/home/www/bo56.com/header.php \n auto_append_file=/home/www/bo56.com/external/footer.php";
```
