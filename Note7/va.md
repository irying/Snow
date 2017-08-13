### vprintf

> 头文件：#include <stdio.h>    #include <stdarg.h>
>
> 定义函数：int vprintf(const char * format, va_list ap);
>

**函数说明：vprintf()作用和printf()相同, 参数format 格式也相同。va_list 为不定个数的参数列, 用法及范例请参考附录C。**



```c
#include <stdio.h>
#include <stdarg.h>
int my_printf(const char *format, ...)
{
    va_list ap;
    int retval;
    va_start(ap, format);
    printf("my_printf():");
    retval = vprintf(format, ap);
    va_end(ap);
    return retval;
}

main()
{
    int i = 150, j = -100;
    double k = 3.14159;
    my_printf("%d %f %x\n", j, k, i);
    my_printf("%2d %*d\n", i, 2, i);
}

执行：
my_printf() : -100 3.14159 96
my_printf() : 150 150
```



### vfprintf

> 输出到文件
>
> 头文件：#include <stdio.h>    #include <stdarg.h>
>
> 定义函数：int vfprintf(FILE *stream, const char * format, va_list ap);
>

**函数说明：vfprintf()会根据参数format 字符串来转换并格式化数据, 然后将结果输出到参数stream 指定的文件中, 直到出现字符串结束('\0')为止. 关于参数format 字符串的格式请参考printf(). va_list 用法请参考附录C 或vprintf()范例.**



va在这里是variable-argument(可变参数)的意思.这些宏定义在stdarg.h中,所以用到可变参数的程序应该包含这个头文件.下面我们写一个简单的可变参数的函数,改函数至少有一个整数参数,第二个参数也是整数,是可选的.函数只是打印这两个参数的值.



```C
void simple_va_fun(int i, ...) 
{ 
　　  va_list arg_ptr; 

　　  int j=0; 　  　
     va_start(arg_ptr, i); 
　  　j=va_arg(arg_ptr, int); 

　   va_end(arg_ptr); 　  　
     printf("%d %d\n", i, j); 

　  　return; 

}

```


​      我们可以在我们的头文件中这样声明我们的函数:    　

```C
extern void simple_va_fun(int i, ...);
```

　   我们在程序中可以这样调用: 　   

​	simple_va_fun(100);       

​	simple_va_fun(100,200);
　#####从这个函数的实现可以看到,我们使用**可变参数应该有以下步骤**: 

1. 首先在函数里定义一个va_list型的变量,这里是arg_ptr,这个变量是指向参数的指针. 

2. **然后用va_start宏初始化变量arg_ptr,这个宏的第二个参数是第一个可变参数的前一个参数,是一个固定的参数**. 
3. 然后用va_arg返回可变的参数,并赋值给整数j. va_arg的第二个参数是你要返回的参数的类型,这里是int型.
4. 最后用va_end宏结束可变参数的获取.然后你就可以在函数里使用第二个参数了.如果函数有多个可变参数的,依次调用va_arg获取各个参数.　　    

  #####如果我们用下面三种方法调用的话,都是合法的,但结果却不一样: 
1. simple_va_fun(100);     结果是:100 -123456789(**会变的值**) 　　

2. simple_va_fun(100,200);　　  结果是:100 200 
3. simple_va_fun(100,200,300);　　  结果是:100 200 



参考：http://blog.csdn.net/edonlii/article/details/8497704



### 举个例子

```c
#ifndef MYEXT_H
#define MYEXT_H

#include <stdio.h>
#include <stdarg.h>

#include "php.h"
#include "ext/standard/info.h"

#define TRACE(fmt, ...) do { trace(__FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); } while (0)

static inline void trace(const char *file, int line, const char* function, const char *fmt, ...) {
    fprintf(stderr, "%s(%s:%d) - ", function, file, line);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

#endif
```



这个例子的inline疑问



引入内联函数的目的是为了解决程序中函数调用的效率问题。

调用函数实际上将程序执行顺序转移到函数所存放在内存中某个地址，将函数的程序内容执行完后，再返回到转去执行该函数前的地方。这**<u>种转移操作要求在转去前要保护现场并记忆执行的地址，转回后先要恢复现场，并按原来保存地址继续执行。因此，函数调用要有一定的时间和空间方面的开销，于是将影响其效率。</u>**特别是对于一些函数体代码不是很大，但又频繁地被调用的函数来讲，解决其效率问题更为重要。**引入内联函数实际上就是为了解决这一问题。**



#### 在程序编译时，编译器将程序中出现的内联函数的调用表达式用内联函数的函数体来进行替换。显然，这种做法不会产生转去转回的问题，但是由于在编译时将函数休中的代码被替代到程序中，因此会增加目标程序代码量，进而增加空间开销，而在时间代销上不象函数调用时那么大，可见它是以目标代码的增加为代价来换取时间的节省。



内联函数具有一般函数的特性，它与一般函数所不同之处公在于函数调用的处理。一般函数进行调用时，要将程序执行权转到被调用函数中，然后再返回到调用它的函数中；而内联函数在调用时，是将调用表达式用内联函数体来替换。在使用内联函数时，应注意如下几点：

##### 1.在内联函数内不允许用循环语句和开关语句。

##### 2.内联函数的定义必须出现在内联函数第一次被调用之前。

##### 3.本栏目讲到的类结构中所有在类说明内部定义的函数是内联函数。



inline定义的函数 和 宏定义一样，只在本地文件可见。**<u>所以建议Inline定义的函数放在头文件中</u>**

gcc中的Inline函数法则

​    1. static inline --->编译器本地展开。

​    2. inline--->本地展开，外地为Inline函数生成独立的汇编代码

​    3. extern inline--->不会生成独立的汇编代码。

​        特性1.即使是通过指针应用或者是递归调用也不会让编译器为它生成汇编码，在这种时候对此函数的调用会被处理成一个外部引用

​        特性2.extern inline的函数允许和外部函数重名，即在存在一个外部定义的全局库函数的情况下，再定义一个同名的extern inline函数也是合法的。 

### 第1点

static inline函数和static函数一样，其定义的范围是local的，即可以在程序内有多个同名的定义（只要不位于同一个文件内即可）。 

#### 看看第2点

foo.c:
/* 这里定义了一个inline的函数foo() */
inline foo() {
​    ...;   <- **编译器会像非inline函数一样为foo()生成独立的汇编码**
}
void func1() {
​    foo(); <- **同文件内foo()可能被编译器内联展开编译而不是直接call上面生成的汇编码**
}
而在另一个文件里调用foo()的时候，则直接call的是上面文件内生成的汇编码：
bar.c:
extern foo(); <- 声明foo()，**注意不能在声明内带inline关键字**
void func2() {
​    foo();    <- **这里就是直接call在foo.c内为foo()函数生成的汇编码了**
}

#### 第3点的extern inline用处：

在一个库函数的c文件内，定义一个普通版本的库函数foo：

```
mylib.c:
void foo()
{  
	...;
}
```

然后再在其头文件内，定义（注意不是声明！）一个实现相同的exterin inline的版本：

```
mylib.h:
extern foo()
{   
    ...;
}
```

那么在别的文件要使用这个库函数的时候，只要include了mylib.h，在能内联展开的地方，编译器都会使用头文件内extern inline的版本来展开。

而在无法展开的时候（函数指针引用等情况），编译器就会引用mylib.c中的那个独立编译的普通版本。

即看起来似乎是个可以在外部被内联的函数一样，所以这应该是gcc的extern inline意义的由来。
