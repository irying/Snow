字符串API http://c.biancheng.net/cpp/u/string_h/



## 1.字符串指针

```
char * str1;
```

str1 只是一个指针，指针指向的空间还没有分配，所以**此时用strcpy向str1所指向的内存中拷贝内容将出错**。利用malloc动态分配指向的内存（在堆中）。

```
char  str2[10];
```

strcpy(str2,"aaaaaa")这种数组初始化方式，是将"aaaaaa"拷贝到以str2为开始地址的内存空间当中。

## 字符串指针和数组的关系

```
char *str1;
char str2[10]="dfdf";
str1=str2;
```

//将指针str1指向以str2为首地址的内存空间，即**现在str2和str1表示内存中的同一区域**。

## strcpy在字符串指针的应用

```
char *  str3="aaaaaaaaaaaaaaa";
char * str2="bbbb";
strcpy(str3,str2);
```

//这时的**str2和str3就不指向同一内存空间**，**<u>因为str3在初始化时已经分配了指向的内存空间</u>**；此时只是将str2所指向的内存的内容拷贝到str3所指向的内存空间的内容（注意：str3指向的内存空间的大小最好大于str2所指向的内存空间的大小,否则，可能将其他变量的内存覆盖。另外，c语言对数组不做越界检查，使用时候小心，否则出现不可预料的错误）。

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main()
{
    // 不可以声明为 char *str = "http://c.biancheng.net";
    char str[] = "http://c.biancheng.net";
    memset(str, '-', 7);
    puts(str);
    system("pause");
    return EXIT_SUCCESS;
}
执行结果：
-------c.biancheng.net

注意：字符数组是可以被修改的，字符串是只读的，不能被修改，而 memset() 又必须修改 str，所以不能将 char str[] = "http://c.biancheng.net"; 声明为 char *str = "http://c.biancheng.net";，否则运行时会报错。
```



## 2.字符串的操作

```
char str[10];
struct aa   bb;
........
memset(str,'\0',sizeof(str));//清空内存，赋值为10个'\0'字符 
memset(&bb,0,sizeof(struct aa));
```

   从文件中逐行读取，可以处理完一行，memset一下，再读取下一行    
​     **strcat(str1,str2);//也要注意str1最好是定长的数组**，如果是指针还要初始化，还要长度。 
​    字符串比较大小只能用strcmp,strncmp
​    strcmp(str1,str2)//    =0相等，否则不等 
​    strncmp(str1,str2,n)//    比较str1和str2的前n个字符     

​    atoi//将字符串转为整数 ， char *itoa(int value, char *string, int radix); //将整数转为字符串

```c
连接字符串并输出。
#include <stdio.h>
#include <string.h>
int main ()
{
    char str[80];
    strcpy (str,"these ");
    strcat (str,"strings ");
    strcat (str,"are ");
    strcat (str,"concatenated.");
    puts (str);
    return 0;
}
输出结果：
these strings are concatenated.
```



```c
void *memcpy1(void *desc,const void * src,size_t size)
{
 if((desc == NULL) && (src == NULL))
 {
  return NULL;
 }
 unsigned char *desc1 = (unsigned char*)desc;
 unsigned char *src1 = (unsigned char*)src;
 while(size-- >0)
 {
  *desc1 = *src1;
  desc1++;
  src1++;
 }
 return desc;
}

int _tmain(int argc, _TCHAR* argv[])
{
 int dest[2] = {0};
 const char src[5] = "1234";
 //printf(src);
 memcpy1(dest,src,sizeof(src));
 //*(dest+5) = '/0';
 printf((char *)dest);
 int m = -1;
 return 0;
}
```

注意事项：

（1）void* 一定要返回一个值（指针），这个和void不太一样！

（2）首先要判断指针的值不能为空，desc为空的话肯定不能拷贝内存空间,src为空相当于没有拷贝；所以之间return掉；

（3）""空串是指内容为0，NULL是0，不是串；两个不等价；

（4）**int dest[2] = {0};这是对int 类型的数组初始化的方法；如果是char类型，就用char a[5] = "1234";  注意数组下标要多于实际看到的字符数，因为还有'/0'**

（5）printf((char *)dest);这句话，是把 char 类型 src 传到 int 类型的 dest的内存强制**<u>转化成char类型，然后打印出来；因为直接看int类型的dest是看不到里面的内容的</u>**；因为有unsigned char *desc1 = (unsigned char*)desc;所以字符可以传到dest里面保存起来，dest所指向的内存长度4个字节，**强制转化为char 就是把四个字节分成一个一个的字节，这样就可以看到 一个个字符了，如果定义成char dest[5] = "1234";就不用转化；**呵呵，表达起来真累人；

（6）memcpy1(dest,src,sizeof(src));注意里面的sizeof(src)，这个是包括字符串的结束符'/0'的；所以不用担心printf(dest);但是如果用memcpy1(dest,src,4);没**有'/0'就要*(dest+5) = '/0';**这样保证是一个完整的字符串；

   (7）如果初始化的时候：

```
 char dest[1024] = "12345666";//{0};
 const char src[5] = "3333";
```

 那么拷贝的时候，如果用memcpy1(dest,src,sizeof(src));则printf(dest);出来是3333如果memcpy1(dest,src,4);则printf(dest);出来是33335666；因为上面的sizeof(src)，包含'/0',所以拷贝过去的字符串以'/0'结束，就只有3333，而如果传4个字符，'/0'是第五个字符，那就遇到dest[1024] 的'/0'结束，所以是33335666

#### 字符串的'/0'问题一定要注意啊!!!

看一个实现（https://my.oschina.net/u/2400526/blog/491549）

```C
unsigned char g_pData[1024] = "";

DWORD g_dwOffset = 0;

bool PackDatatoServer(const unsigned char *pData, const unsigned int uSize)

{

 memcpy(g_pData+g_dwOffset, pData, uSize);

 g_dwOffset += uSize;

 //g_pData += uSize;

 return true;

}

void main()

{

 const unsigned char a[4] = "123";

 PackDatatoServer(a, 3);

 PackDatatoServer(a, 1111);

 int b = -1;

}

```

PackDatatoServer（）函数的作用是把每次的资源内存拷贝到目标内存里面，而且是累加的拷贝；也就是后一次紧接着上一次的拷贝；
显然用到了memcpy函数；
实现原理是用到了一个全局变量g_dwOffset 保存之前拷贝的长度，最开始没有想到这一点，结果每次拷贝都是一次性的，下一次拷贝把
上一次的冲掉了；所以用全局变量记录拷贝的长度；
第二个需要注意的是，拷贝的过程中注意不要改变目标指针的指向，即目标指针始终指向初始化的时候指向的位置；那么怎么实现累积拷贝呢？
就是用的指针偏移；**第一次实现的时候，把g_pData += uSize;写到了函数里面，这样写是能够实现指针位移的目标，但是指针指向也发生改变；**
 **另外：g_pData += uSize;也有报错：left operand must be l-value，原因是：把地址赋值给一个不可更改的指针！**



```C
char   a[100];  
char   *p  =  new   char[10];  
a   =   p;   //这里出错，注意了：数组的首地址也是一个常量指针，指向固定不能乱改的~~
char   *   const   pp   =   new   char[1];  
pp   =   a;   //也错  

```


**所以既不能改变首地址，又要满足累积赋值（就是赋值的时候要从赋过值的地方开始向下一个内存块赋值，想到指针加），所以想到把指针加写到**
**函数参数里面**，这时就要充分了解memcpy的实现过程，里面是一个一个字符的赋值的，想连续赋值，就要把指针指向连续的内存的首地址。



## 内存分配的三种方式

（1）静态内存分配。在内存的数据区上创建。内存在程序编译的时候就已经分配好。 
​    这块内存在程序的整个运行期间都存在。例如 已经初始化的变量，全局变量，static变量。 
（2）在栈上创建。在执行函数时，函数内局部变量的存储单元都可以在栈上创建，函数执行结束时这些存储单元自动被释放。栈内存分配运算内置于处理器的指令集中，效率很高，但是分配的内存容量有限，也属于动态内存分配。 
（3）从堆上分配，亦称动态内存分配。程序在运行的时候用malloc或new申请任意多少的内存，程序员自己负责在何时用free或delete释放内存。动态内存的生存期由我们决定，使用非常灵活， 
但问题也最多。 
按照编译原理的观点,程序运行时的内存分配有三种策略,分别是静态的,栈式的,和堆式的。 
   静态存储分配是指在编译时就能确定每个数据目标在运行时刻的存储空间需求,因而在编译时就可以给他们分配固定的内存空间。**这种分配策略要求程序代码中不允许有可变数据结构(比如可变数组)的存在,也不允许有嵌套或者递归的结构出现,因为它们都会导致编译程序无法计算准确的存储空间需求。** 
   栈式存储分配也可称为动态存储分配,是由一个类似于堆栈的运行栈来实现的。和静态存储分配相反,在栈式存储方案中,程序对数据区的需求在编译时是完全未知的,只有到运行的时候才能够知道,但是规定在运行中进入一个程序模块时,必须知道该程序模块所需的数据区大小才能够为其分配内存。和我们在数据结构所熟知的栈一样,栈式存储分配按照先进后出的原则进行分配。 
   静态存储分配要求在编译时能知道所有变量的存储要求,**栈式存储分配要求在过程的入口处必须知道所有的存储要求,**而堆式存储分配则专门负责在编译时或运行时模块入口处都无法确定存储要求的数据结构的内存分配,比如可变长度串和对象实例。堆由大片的可利用块或空闲块组成，堆中的内存可以按照任意顺序分配和释放。