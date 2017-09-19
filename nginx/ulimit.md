http://www.cnblogs.com/kongzhongqijing/p/5784293.html

ulimit -a 用来显示当前的各种用户进程限制。
某linux用户的最大进程数设为10000个：ulimit -u 10000


对于需要做许多 socket 连接并使它们处于打开状态的Java 应用程序而言，最好通过使用 ulimit -n xx 修改每个进程可打开的文件数，缺省值是 1024。

ulimit -n 4096 将每个进程可以打开的文件数目加大到4096，缺省为1024

其他建议设置成无限制（unlimited）的一些重要设置是：
数据段长度：ulimit -d unlimited
最大内存大小：ulimit -m unlimited
堆栈大小：ulimit -s unlimited
CPU 时间：ulimit -t unlimited
虚拟内存：ulimit -v unlimited

 

    暂时地，适用于通过 ulimit 命令登录 shell 会话期间。
    永久地，通过将一个相应的 ulimit 语句添加到由登录 shell 读取的文件中， 即特定于 shell 的用户资源文件，如：
1)、解除 Linux 系统的最大进程数和最大文件打开数限制：
        vi /etc/security/limits.conf
        # 添加如下的行
        * soft noproc 11000
        * hard noproc 11000
        * soft nofile 4100
        * hard nofile 4100
      说明：* 代表针对所有用户，noproc 是代表最大进程数，nofile 是代表最大文件打开数
2)、让 SSH 接受 Login 程式的登入，方便在 ssh 客户端查看 ulimit -a 资源限制：
        a、vi /etc/ssh/sshd_config
            把 UserLogin 的值改为 yes，并把 # 注释去掉
        b、重启 sshd 服务：
              /etc/init.d/sshd restart
3)、修改所有 linux 用户的环境变量文件：
    vi /etc/profile
    ulimit -u 10000
    ulimit -n 4096
    ulimit -d unlimited
    ulimit -m unlimited
    ulimit -s unlimited
    ulimit -t unlimited
    ulimit -v unlimited
 保存后运行#source /etc/profile 使其生效

 

公司服务器需要调整 ulimit的stack size 参数调整为unlimited 无限，使用ulimit -s unlimited时只能在当时的shell见效，重开一个shell就失效了。。于是得在/etc/profile 的最后面添加ulimit -s unlimited 就可以了，source /etc/profile使修改文件生效。


如果碰到类似的错误提示ulimit: max user processes: cannot modify limit: 不允许的操作 ulimit: open files: cannot modify limit: 不允许的操作
为啥root用户是可以的？普通用户又会遇到这样的问题？
看一下/etc/security/limits.conf大概就会明白。
linux对用户有默认的ulimit限制，而这个文件可以配置用户的硬配置和软配置，硬配置是个上限。超出上限的修改就会出“不允许的操作”这样的错误。
在limits.conf加上
*        soft    noproc 10240
*        hard    noproc 10240
*        soft    nofile 10240
*        hard    nofile 10240
就是限制了任意用户的最大线程数和文件数为10240。

 

如何设置普通用户的ulimit值
1、vim /etc/profile
增加 ulimit -n 10240
source /etc/profile 重新启动就不需要运行这个命令了。
2、修改/etc/security/limits.conf
增加
*      hard     nofile     10240   
\\限制打开文件数10240
3、测试，新建普通用户，切换到普通用户使用ulit -a 查看是否修改成功。
