## GC

- `PHP 5` 的内存回收原理？请详细描述`ZendMM`的工作原理
- `PHP 7` 的垃圾回收和 `PHP 5` 有什么区别？

## 结构

- `PHP 7` 中对`zVal`做了哪些修改？

- `PHP 7` 中哪些变量类型在**栈**，哪些变量类型在**堆**？变量在栈会有什么优势？`PHP 7`是如何让变量新建在栈的？

- 详细描述`PHP`中`HashMap`的结构是如何实现的？

- 下面代码中，在`PHP 7`下， `$a` 和 `$b`、`$c`、`$d` 分别指向什么`zVal`结构？

  `$d` 被修改的时候，`PHP 7` / `PHP 5` 的内部分别会有哪些操作？

  ```
  $a = 'string';
  $b = &$a;
  $c = &$b;
  $d = $b;
  $d = 'to';
  ```

- JIT 是做了哪些优化，从而对PHP的速度有不少提升？



(以前gc在zval上)

PHP中变量的内存是通过引用计数进行管理的，而且PHP7中引用计数是在zend_value而不是zval上，**变量之间的传递、赋值通常也是针对zend_value。**



zval结构

```
zval结构比较简单，内嵌一个union类型的zend_value保存具体变量类型的值或指针，zval中还有两个union：u1、u2:

u1： 它的意义比较直观，变量的类型就通过u1.v.type区分，另外一个值type_flags为类型掩码，在变量的内存管理、gc机制中会用到，第三部分会详细分析，至于后面两个const_flags、reserved暂且不管
u2： 这个值纯粹是个辅助值，假如zval只有:value、u1两个值，整个zval的大小也会对齐到16byte，既然不管有没有u2大小都是16byte，把多余的4byte拿出来用于一些特殊用途还是很划算的，比如next在哈希表解决哈希冲突时会用到，还有fe_pos在foreach会用到......
从zend_value可以看出，除long、double类型直接存储值外，其它类型都为指针，指向各自的结构。


```

#### 2.1.2.1 标量类型

最简单的类型是true、false、long、double、null，其中true、false、null没有value，直接根据type区分，而long、double的值则直接存在value中：zend_long、double，也就是标量类型不需要额外的value指针。



#### 2.1.2.5 引用

事实上并不是所有的PHP变量都会用到引用计数，标量：true/false/double/long/null是硬拷贝自然不需要这种机制，但是除了这几个还有两个特殊的类型也不会用到：**interned string(内部字符串，就是上面提到的字符串flag：IS_STR_INTERNED)、immutable array，它们的type是`IS_STRING`、`IS_ARRAY`，**与普通string、array类型相同，那怎么区分一个value是否支持引用计数呢？**还记得`zval.u1`中那个类型掩码`type_flag`吗？**正是通过这个字段标识的，这个字段除了标识value是否支持引用计数外还有其它几个标识位，按位分割，注意：`type_flag`与`zval.value->gc.u.flag`不是一个值。



(创建一个zend_reference结构，其内嵌了一个zval，这个zval的value 指向原来zval的value，原来的zval类型修改为IS_REFERENCE，它的value指向新创建的zend_reference结构)

 ![](https://github.com/pangudashu/php7-internal/raw/master/img/zend_ref.png)



引用是PHP中比较特殊的一种类型，它实际是指向另外一个PHP变量，对它的修改会直接改动实际指向的zval，可以简单的理解为C中的指针，在PHP中通过`&`操作符产生一个引用变量，也就是说不管以前的类型是什么，`&`首先会创建一个`zend_reference`结构，其内嵌了一个zval，这个zval的value指向原来zval的value(如果是布尔、整形、浮点则直接复制原来的值)，然后将原zval的类型修改为IS_REFERENCE，原zval的value指向新创建的`zend_reference`结构。

```
struct _zend_reference {
    zend_refcounted_h gc;
    zval              val;
};
```



#### 2.1.3.1 引用计数

引用计数是指在value中增加一个字段`refcount`记录指向当前value的数量，变量复制、函数传参时并不直接硬拷贝一份value数据，而是将`refcount++`，变量销毁时将`refcount--`，等到`refcount`减为0时表示已经没有变量引用这个value，将它销毁即可。

#### 2.1.3.2 写时复制

(不是所有类型都可以copy的，比如对象、资源，**实时上只有string、array两种支持，与引用计数相同**，也是通过`zval.u1.type_flag`标识value是否可复制)

**多个变量可能指向同一个value，然后通过refcount统计引用数，这时候如果其中一个变量试图更改value的内容则会重新拷贝一份value修改，同时断开旧的指向**，写时复制的机制

![](https://github.com/pangudashu/php7-internal/raw/master/img/zval_sep.png)



#### 2.1.3.3 变量回收

PHP变量的回收主要有两种：**主动销毁、自动销毁**。主动销毁指的就是 **unset** ，而自动销毁就是PHP的自动管理机制，在return时减掉局部变量的refcount，即使没有显式的return，PHP也会自动给加上这个操作，另外一个就是写时复制时会断开原来value的指向，**这时候也会检查断开后旧value的refcount**。**如果refcount减到0则直接释放value，这是变量的简单gc过程**，但是实际过程中出现gc无法回收导致内存泄漏的bug(**循环引用**)。

![](https://github.com/pangudashu/php7-internal/raw/master/img/gc_2.png)

`unset($a)`之后由于数组中有子元素指向`$a`，所以`refcount > 0`，无法通过简单的gc机制回收，这种变量就是垃圾，垃圾回收器要处理的就是这种情况，目前**垃圾只会出现在array、object两种类型中，所以只会针对这两种情况作特殊处理：当销毁一个变量时，如果发现减掉refcount后仍然大于0，且类型是IS_ARRAY、IS_OBJECT则将此value放入gc可能垃圾双向链表中，**等这个链表达到一定数量后启动检查程序将所有变量检查一遍，如果确定是垃圾则销毁释放。



(当变量的refcount减少后大于0，PHP并不会立即进行对这个变量进行垃圾鉴定，而是放入一个缓冲buffer中，等这个buffer满了以后(10000个值)再统一进行处理，加入buffer的是变量zend_value的`zend_refcounted_h`)



1.一个变量只能加入一次buffer，为了防止重复加入，变量加入后会把`zend_refcounted_h.gc_info`置为`GC_PURPLE`，即标为紫色；

2.垃圾缓存区是一个双向链表，等到缓存区满了以后则启动垃圾检查过程：**遍历缓存区，再对当前变量的所有成员进行遍历，然后把成员的refcount减1**(如果成员还包含子成员则也进行递归遍历，其实就是深度优先的遍历)

```C
从buffer链表的roots开始遍历，把当前value标为灰色(zend_refcounted_h.gc_info置为GC_GREY)，然后对当前value的成员进行深度优先遍历，把成员value的refcount减1，并且也标为灰色；
```

3.**重复遍历buffer链表，检查当前value引用是否为0，为0则表示确实是垃圾，把它标为白色(GC_WHITE)，如果不为0则排除了引用全部来自自身成员的可能，表示还有外部的引用，并不是垃圾**，这时候因为步骤(1)对成员进行了refcount减1操作，需要再还原回去，对所有成员进行深度遍历，**把成员refcount加1，同时标为黑色**；

4.再次遍历buffer链表，**将非GC_WHITE的节点从roots链表中删除，最终roots链表中全部为真正的垃圾**，最后将这些垃圾清除。

**这个算法的原理很简单，垃圾是由于成员引用自身导致的，那么就对所有的成员减一遍引用，结果如果发现变量本身refcount变为了0则就表明其引用全部来自自身成员。**



## 3.3 Zend引擎执行过程

Zend引擎主要包含两个核心部分：编译、执行：