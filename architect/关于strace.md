### 关于strace



来自：http://www.cnblogs.com/ggjucheng/archive/2012/01/08/2316692.html

```
root@ubuntu:/usr# strace cat /dev/null 
execve("/bin/cat", ["cat", "/dev/null"], [/* 22 vars */]) = 0
brk(0)                                  = 0xab1000
access("/etc/ld.so.nohwcap", F_OK)      = -1 ENOENT (No such file or directory)
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f29379a7000
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
...
brk(0) = 0xab1000
brk(0xad2000) = 0xad2000
fstat(1, {st_mode=S_IFCHR|0620, st_rdev=makedev(136, 0), ...}) = 0
open("/dev/null", O_RDONLY) = 3
fstat(3, {st_mode=S_IFCHR|0666, st_rdev=makedev(1, 3), ...}) = 0
read(3, "", 32768) = 0
close(3) = 0
close(1) = 0
close(2) = 0
exit_group(0) = ?
```

每一行都是一条系统调用，**等号左边是系统调用的函数名及其参数，右边是该调用的返回值**。
strace 显示这些调用的参数并返回符号形式的值。strace 从内核接收信息，而且不需要以任何特殊的方式来构建内核。



```c
-c 统计每一系统调用的所执行的时间,次数和出错的次数等. 
-d 输出strace关于标准错误的调试信息. 
-f 跟踪由fork调用所产生的子进程. 
-ff 如果提供-o filename,则所有进程的跟踪结果输出到相应的filename.pid中,pid是各进程的进程号. 
-F 尝试跟踪vfork调用.在-f时,vfork不被跟踪. 
-h 输出简要的帮助信息. 
-i 输出系统调用的入口指针. 
-q 禁止输出关于脱离的消息. 
-r 打印出相对时间关于,,每一个系统调用. 
-t 在输出中的每一行前加上时间信息. 
-tt 在输出中的每一行前加上时间信息,微秒级. 
-ttt 微秒级输出,以秒了表示时间. 
-T 显示每一调用所耗的时间. 
-v 输出所有的系统调用.一些调用关于环境变量,状态,输入输出等调用由于使用频繁,默认不输出. 
-V 输出strace的版本信息. 
-x 以十六进制形式输出非标准字符串 
-xx 所有字符串以十六进制形式输出. 
-a column 
设置返回值的输出位置.默认 为40. 
-e expr 
指定一个表达式,用来控制如何跟踪.格式如下: 
[qualifier=][!]value1[,value2]... 
qualifier只能是 trace,abbrev,verbose,raw,signal,read,write其中之一.value是用来限定的符号或数字.默认的 qualifier是 trace.感叹号是否定符号.例如: 
-eopen等价于 -e trace=open,表示只跟踪open调用.而-etrace!=open表示跟踪除了open以外的其他调用.有两个特殊的符号 all 和 none. 
注意有些shell使用!来执行历史记录里的命令,所以要使用\\. 
-e trace=set 
只跟踪指定的系统 调用.例如:-e trace=open,close,rean,write表示只跟踪这四个系统调用.默认的为set=all. 
-e trace=file 
只跟踪有关文件操作的系统调用. 
-e trace=process 
只跟踪有关进程控制的系统调用. 
-e trace=network 
跟踪与网络有关的所有系统调用. 
-e strace=signal 
跟踪所有与系统信号有关的 系统调用 
-e trace=ipc 
跟踪所有与进程通讯有关的系统调用 
-e abbrev=set 
设定 strace输出的系统调用的结果集.-v 等与 abbrev=none.默认为abbrev=all. 
-e raw=set 
将指 定的系统调用的参数以十六进制显示. 
-e signal=set 
指定跟踪的系统信号.默认为all.如 signal=!SIGIO(或者signal=!io),表示不跟踪SIGIO信号. 
-e read=set 
输出从指定文件中读出 的数据.例如: 
-e read=3,5 
-e write=set 
输出写入到指定文件中的数据. 
-o filename 
将strace的输出写入文件filename 
-p pid 
跟踪指定的进程pid. 
-s strsize 
指定输出的字符串的最大长度.默认为32.文件名一直全部输出. 
-u username 
以username 的UID和GID执行被跟踪的命令
```

```
strace -o output.txt -T -tt -e trace=all -p 28979
```

上面的含义是 跟踪28979进程的所有系统调用（-e trace=all），并统计系统调用的花费时间，以及开始时间（并以可视化的时分秒格式显示），最后将记录结果存在output.txt文件里面。



```
strace -c -p $(pgrep -n php-fpm)
```

延伸：

```
prep

-l 列出程序名和进程ID；
-o 进程起始的ID；
-n 进程终止的ID；
```

### brk整个过程

来自https://huoding.com/2013/10/06/288

```
shell> strace -c -p $(pgrep -n php-cgi)
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 30.53    0.023554         132       179           brk
 14.71    0.011350         140        81           mlock
 12.70    0.009798          15       658        16 recvfrom
  8.96    0.006910           7       927           read
  6.61    0.005097          43       119           accept
  5.57    0.004294           4       977           poll
  3.13    0.002415           7       359           write
  2.82    0.002177           7       311           sendto
  2.64    0.002033           2      1201         1 stat
  2.27    0.001750           1      2312           gettimeofday
  2.11    0.001626           1      1428           rt_sigaction
  1.55    0.001199           2       730           fstat
  1.29    0.000998          10       100       100 connect
  1.03    0.000792           4       178           shutdown
  1.00    0.000773           2       492           open
  0.93    0.000720           1       711           close
  0.49    0.000381           2       238           chdir
  0.35    0.000271           3        87           select
  0.29    0.000224           1       357           setitimer
  0.21    0.000159           2        81           munlock
  0.17    0.000133           2        88           getsockopt
  0.14    0.000110           1       149           lseek
  0.14    0.000106           1       121           mmap
  0.11    0.000086           1       121           munmap
  0.09    0.000072           0       238           rt_sigprocmask
  0.08    0.000063           4        17           lstat
  0.07    0.000054           0       313           uname
  0.00    0.000000           0        15         1 access
  0.00    0.000000           0       100           socket
  0.00    0.000000           0       101           setsockopt
  0.00    0.000000           0       277           fcntl
------ ----------- ----------- --------- --------- ----------------
100.00    0.077145                 13066       118 total	
```

看上去「brk」非常可疑，它竟然耗费了三成的时间，保险起见，单独确认一下：

```
shell> strace -T -e brk -p $(pgrep -n php-cgi)
brk(0x1f18000) = 0x1f18000 <0.024025>
brk(0x1f58000) = 0x1f58000 <0.015503>
brk(0x1f98000) = 0x1f98000 <0.013037>
brk(0x1fd8000) = 0x1fd8000 <0.000056>
brk(0x2018000) = 0x2018000 <0.012635>
```

说明：在Strace中和操作花费时间相关的选项有两个，分别是「-r」和「-T」，它们的差别是「-r」表示相对时间，而「-T」表示绝对时间。简单统计可以用「-r」，但是需要注意的是在多任务背景下，CPU随时可能会被切换出去做别的事情，所以相对时间不一定准确，此时最好使用「-T」，在行尾可以看到操作时间，可以发现确实很慢。

在继续定位故障原因前，我们先通过「man brk」来查询一下它的含义：

> brk() sets the end of the data segment to the value specified by end_data_segment, when that value is reasonable, the system does have enough memory and the process does not exceed its max data size (see setrlimit(2)).

简单点说就是内存不够用时通过它来申请新内存（[data segment](http://en.wikipedia.org/wiki/Data_segment)），可是为什么呢？

```
shell> strace -T -p $(pgrep -n php-cgi) 2>&1 | grep -B 10 brk
stat("/path/to/script.php", {...}) = 0 <0.000064>
brk(0x1d9a000) = 0x1d9a000 <0.000067>
brk(0x1dda000) = 0x1dda000 <0.001134>
brk(0x1e1a000) = 0x1e1a000 <0.000065>
brk(0x1e5a000) = 0x1e5a000 <0.012396>
brk(0x1e9a000) = 0x1e9a000 <0.000092>
```

通过「grep」我们很方便就能获取相关的上下文，**反复运行几次，发现每当请求某些PHP脚本时，就会出现若干条耗时的「brk」，而且这些PHP脚本有一个共同的特点，就是非常大，甚至有几百K**，为何会出现这么大的PHP脚本？实际上是程序员为了避免数据库操作，把非常庞大的数组变量通过「[var_export](http://www.php.net/var_export)」持久化到PHP文件中，然后在程序中通过「[include](http://www.php.net/include)」来获取相应的变量，因为变量太大，所以PHP不得不频繁执行「brk」，不幸的是在本例的环境中，此操作比较慢，从而导致处理请求的时间过长，加之PHP进程数有限，于是乎在Nginx上造成请求拥堵，最终导致高负载故障。

下面需要验证一下推断似乎否正确，首先查询一下有哪些地方涉及问题脚本：

```
shell> find /path -name "*.php" | xargs grep "script.php"
```

直接把它们都禁用了，看看服务器是否能缓过来，或许大家觉得这太鲁蒙了，但是特殊情况必须做出特殊的决定，不能像个娘们儿似的优柔寡断，没过多久，服务器负载恢复正常，接着再统计一下系统调用的耗时：

```
shell> strace -c -p $(pgrep -n php-cgi)
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 24.50    0.001521          11       138         2 recvfrom
 16.11    0.001000          33        30           accept
  7.86    0.000488           8        59           sendto
  7.35    0.000456           1       360           rt_sigaction
  6.73    0.000418           2       198           poll
  5.72    0.000355           1       285           stat
  4.54    0.000282           0       573           gettimeofday
  4.41    0.000274           7        42           shutdown
  4.40    0.000273           2       137           open
  3.72    0.000231           1       197           fstat
  2.93    0.000182           1       187           close
  2.56    0.000159           2        90           setitimer
  2.13    0.000132           1       244           read
  1.71    0.000106           4        30           munmap
  1.16    0.000072           1        60           chdir
  1.13    0.000070           4        18           setsockopt
  1.05    0.000065           1       100           write
  1.05    0.000065           1        64           lseek
  0.95    0.000059           1        75           uname
  0.00    0.000000           0        30           mmap
  0.00    0.000000           0        60           rt_sigprocmask
  0.00    0.000000           0         3         2 access
  0.00    0.000000           0         9           select
  0.00    0.000000           0        20           socket
  0.00    0.000000           0        20        20 connect
  0.00    0.000000           0        18           getsockopt
  0.00    0.000000           0        54           fcntl
  0.00    0.000000           0         9           mlock
  0.00    0.000000           0         9           munlock
------ ----------- ----------- --------- --------- ----------------
100.00    0.006208                  3119        24 total
```

显而易见，「brk」已经不见了，取而代之的是「recvfrom」和「accept」，不过这些操作本来就是很耗时的，所以可以定位「brk」就是故障的原因。



### lstat整个过程

来自https://xnow.me/ops/php-debug.html



收到报警线上运行PHP程序的8核服务器load average达到200多，这个太吓人了。赶紧登录系统运行top，看到好几个php-fpm进程CPU利用率达到100%，**看到大多数CPU都消耗在内核态，即sys模式，运行状态良好的程序大多时候应该运行在用户态才对呀。**

```
awk '{if($0~/lstat/){a++}}END{print a,NR,a/NR}' /tmp/php.debug
97183 106370 0.913632
```

10万多个系统调用，其中97万多是lstat，占了全部的91%还多，为什么会有产生这个问题？经过一番Google，找到这篇文章

文章中解释的很清楚，PHP有个bug，在**php.ini中启用open_basedir之后，会导致realpath_cache_size和realpath_cache_ttl失效。realpath_cache参数的作用是缓存文件绝对路径，如果该参数失效，php会在每次处理请求的时候使用lstat系统调用去获取文件信息，这就是大量lstat的由来**，一般服务器上都是尽量调大这两个参数。但是，前几天，安全部门让加上open_basedir限制php访问的系统路径。这就解释了为什么每到访问高峰，系统的load就能一路狂飚到200多。

### php的结论

那么通过 strace 拿到了所有程序去调用系统过程所产生的痕迹后，我们能用来定位哪些问题呢？

1. 调试性能问题，查看系统调用的频率，找出**耗时的程序段**
2. 查看程序读取的是哪些文件从而定位比如配置文件加载错误问题
3. 查看某个 php 脚本长时间运行“假死”情况
4. 当程序出现 “Out of memory” 时被系统发出的 SIGKILL 信息所 kill


# **strace案例**


## 限制strace只跟踪特定的系统调用**

如果你已经知道你要找什么，你可以让strace只跟踪一些类型的系统调用。例如，你需要看看在configure脚本里面执行的程序，你需要监视的系统调 用就是execve。让strace只记录execve的调用用这个命令：

```
strace -f -o configure-strace.txt -e execve ./configure
```