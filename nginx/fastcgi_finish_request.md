当PHP运行在FastCGI模式时,PHP FPM提供了一个名为fastcgi_finish_request的方法.按照文档上的说法,此方法可以提高请求的处理速度,如果有些处理可以在页面生成完后再进行,就可以使用这个方法.

听起来可能有些茫然,我们通过几个例子来说明一下：

<?php
 
echo '例子：';
fastcgi_finish_request(); /* 响应完成, 关闭连接 */
 
/* 记录日志 */
file_put_contents('log.txt', '生存还是毁灭,这是个问题.');
?>
通过浏览器访问此脚本, 结果发现并没有输出相应的字符串,但却生成了相应的文件.由此说明在调用fastcgi_finish_request后,客户端响应就已经结束,但与此同时服务端脚本却继续运行！

合理利用这个特性可以大大提升用户体验,趁热打铁再来一个例子：

<?php
 
echo '例子：';
 
file_put_contents('log.txt', date('Y-m-d H:i:s') . " 上传视频\n", FILE_APPEND);
 
fastcgi_finish_request();
 
sleep(1);
file_put_contents('log.txt', date('Y-m-d H:i:s') . " 转换格式\n", FILE_APPEND);
 
sleep(1);
file_put_contents('log.txt', date('Y-m-d H:i:s') . " 提取图片\n", FILE_APPEND);
 
?>
代码里用sleep模拟一些耗时的操作,浏览时没有被堵塞,程序却都执行了,具体看日志.
