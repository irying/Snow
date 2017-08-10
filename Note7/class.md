上PHP代码

```php
class ParentClass {
}
 
interface Ifce {
        public function iMethod();
}
 
final class Tipi extends ParentClass implements Ifce {
        public static $sa = 'aaa';
        const CA = 'bbb';
 
        public function __constrct() {
        }
 
        public function iMethod() {
        }
 
        private function _access() {
        }
 
        public static function access() {
        }
}
```



定义了一个父类ParentClass，一个接口Ifce，一个子类Tipi。子类继承父类ParentClass， 实现接口Ifce，并且有一个静态变量$sa，一个类常量 CA，一个公用方法，一个私有方法和一个公用静态方法。 



#### 1.类的内部结构

```C
struct _zend_class_entry {
    char type;     // 类型：ZEND_INTERNAL_CLASS / ZEND_USER_CLASS
    char *name;// 类名称
    zend_uint name_length;                  // 即sizeof(name) - 1
    struct　_zend_class_entry *parent; // 继承的父类
    int　refcount;  // 引用数
    zend_bool constants_updated;
 
    zend_uint ce_flags; // ZEND_ACC_IMPLICIT_ABSTRACT_CLASS: 类存在abstract方法
    // ZEND_ACC_EXPLICIT_ABSTRACT_CLASS: 在类名称前加了abstract关键字
    // ZEND_ACC_FINAL_CLASS
    // ZEND_ACC_INTERFACE
    HashTable function_table;      // 方法
    HashTable default_properties;          // 默认属性
    HashTable properties_info;     // 属性信息
    HashTable default_static_members;// 类本身所具有的静态变量
    HashTable *static_members; // type == ZEND_USER_CLASS时，取&default_static_members;
    // type == ZEND_INTERAL_CLASS时，设为NULL
    HashTable constants_table;     // 常量
    struct _zend_function_entry *builtin_functions;// 方法定义入口
 
 
    union _zend_function *constructor;
    union _zend_function *destructor;
    union _zend_function *clone;
 
 
    /* 魔术方法 */
    union _zend_function *__get;
    union _zend_function *__set;
    union _zend_function *__unset;
    union _zend_function *__isset;
    union _zend_function *__call;
    union _zend_function *__tostring;
    union _zend_function *serialize_func;
    union _zend_function *unserialize_func;
    zend_class_iterator_funcs iterator_funcs;// 迭代
 
    /* 类句柄 */
    zend_object_value (*create_object)(zend_class_entry *class_type TSRMLS_DC);
    zend_object_iterator *(*get_iterator)(zend_class_entry *ce, zval *object,
        intby_ref TSRMLS_DC);
 
    /* 类声明的接口 */
    int(*interface_gets_implemented)(zend_class_entry *iface,
            zend_class_entry *class_type TSRMLS_DC);
 
 
    /* 序列化回调函数指针 */
    int(*serialize)(zval *object， unsignedchar**buffer, zend_uint *buf_len,
             zend_serialize_data *data TSRMLS_DC);
    int(*unserialize)(zval **object, zend_class_entry *ce, constunsignedchar*buf,
            zend_uint buf_len, zend_unserialize_data *data TSRMLS_DC);
 
 
    zend_class_entry **interfaces;  //  类实现的接口
    zend_uint num_interfaces;   //  类实现的接口数
 
 
    char *filename; //  类的存放文件地址 绝对地址
    zend_uint line_start;   //  类定义的开始行
    zend_uint line_end; //  类定义的结束行
    char *doc_comment;
    zend_uint doc_comment_len;
 
 
    struct _zend_module_entry *module; // 类所在的模块入口：EG(current_module)
};
```



有2个type：用户定义的类和模块或者内置的类

```c
#define ZEND_INTERNAL_CLASS         1
#define ZEND_USER_CLASS             2
```

**常规的成员方法存放在函数结构体的哈希表中， 而魔术方法则单独保存。 如在类定义中的 union _zend_function *constructor; 定义就是类的构造魔术方法， 它是以函数的形式存在于类结构中，并且与常规的方法分隔开来了。在初始化时，这些魔术方法都会被设置为NULL。**



#### 2.从编译开始

类的定义是以class关键字开始，在Zend/zend_language_scanner.l文件中，找到class对应的**token为T_CLASS**。 根据此token，在Zend/zend_language_parser.y文件中，找到编译时调用的函数：

```C
unticked_class_declaration_statement:
        class_entry_type T_STRING extends_from
            { zend_do_begin_class_declaration(&$1, &$2, &$3 TSRMLS_CC); }
            implements_list
            '{'
                class_statement_list
            '}' { zend_do_end_class_declaration(&$1, &$2 TSRMLS_CC); }
    |   interface_entry T_STRING
            { zend_do_begin_class_declaration(&$1, &$2, NULL TSRMLS_CC); } interface_extends_list
            '{'
                class_statement_list
            '}' { zend_do_end_class_declaration(&$1, &$2 TSRMLS_CC); }
;
 
 
class_entry_type:
        T_CLASS         { $$.u.opline_num = CG(zend_lineno); $$.u.EA.type = 0; }
    |   T_ABSTRACT T_CLASS { $$.u.opline_num = CG(zend_lineno); $$.u.EA.type = ZEND_ACC_EXPLICIT_ABSTRACT_CLASS; }
    |   T_FINAL T_CLASS { $$.u.opline_num = CG(zend_lineno); $$.u.EA.type = ZEND_ACC_FINAL_CLASS; }
;
```

上面的class_entry_type语法说明在语法分析阶段将类分为三种类型：**常规类(T_CLASS)， 抽象类(T_ABSTRACT T_CLASS)和final类(T_FINAL T_CLASS )**。 他们分别对应的类型在内核中为:

- 常规类(T_CLASS) 对应的type=0
- 抽象类(T_ABSTRACT T_CLASS) 对应type=ZEND_ACC_EXPLICIT_ABSTRACT_CLASS
- final类(T_FINAL T_CLASS) 对应type=ZEND_ACC_FINAL_CLASS

除了上面的三种类型外，类还包含有另外两种类型**没有加abstract关键字的抽象类和接口**：

- 没有加abstract关键字的抽象类，它对应的type=ZEND_ACC_IMPLICIT_ABSTRACT_CLASS。 由于在class前面没有abstract关键字，在语法分析时并没有分析出来这是一个抽象类，但是由于类中拥有抽象方法， 在函数注册时判断成员函数是抽象方法或继承类中的成员方法是抽象方法时，会将这个类设置为此种抽象类类型。
- 接口，其type=ZEND_ACC_INTERFACE。接口类型的区分是在interface关键字解析时设置，见interface_entry:对应的语法说明。

```
#define ZEND_ACC_IMPLICIT_ABSTRACT_CLASS    0x10
#define ZEND_ACC_EXPLICIT_ABSTRACT_CLASS    0x20
#define ZEND_ACC_FINAL_CLASS                0x40
#define ZEND_ACC_INTERFACE                  0x80
```



#### 2.1解析完成之后

语法解析完后就可以知道一个类是抽象类还是final类，普通的类，又或者接口。 **定义类时调用了zend_do_begin_class_declaration和zend_do_end_class_declaration函数**， 从这两个函数传入的参数，**<u>zend_do_begin_class_declaration函数用来处理类名，类的类别和父类</u>**， **<u>zend_do_end_class_declaration函数用来处理接口和类的中间代码</u>** 



这两个函数在Zend/zend_complie.c文件中可以找到其实现。

1.在zend_do_begin_class_declaration中，首先会对传入的类名作一个转化，统一成小写。

**opcode ==》** 2.与self关键字一样， 还有parent， static两个关键字的判断在同一个地方。 当这个函数执行完后，我们会得到类声明生成的中间代码为：**ZEND_DECLARE_CLASS** 。 当然，如果我们是声明内部类的话，则生成的中间代码为： **ZEND_DECLARE_INHERITED_CLASS**。

**handler==》 3.<u>ZEND_DECLARE_CLASS_SPEC_HANDLER</u>**。 这个函数通过调用 **do_bind_class** 函数将此类加入到 EG(class_table) 。 在添加到列表的同时，也判断该类是否存在，如果存在，则添加失败，报我们之前提到的类重复声明错误，只是这个判断在编译开启时是不会生效的。

类相关的各个结构均保存在**struct _zend_class_entry** 结构体中。这些具体的类别在语法分析过程中进行区分。 识别出类的类别，类的类名等，并将识别出来的结果存放到类的结构中。



#### 3.初始化的时候，在zend_do_begin_class_declaration中，还有其他操作

在声明类的时候初始化了类的成员变量所在的HashTable，之后如果有新的成员变量声明时，在编译时**zend_do_declare_property**。函数首先检查成员变量不允许的一些情况：

- 接口中不允许使用成员变量
- 成员变量不能拥有抽象属性
- 不能声明成员变量为final
- 不能重复声明属性

```C
if (access_type & ZEND_ACC_FINAL) {
    zend_error(E_COMPILE_ERROR, "Cannot declare property %s::$%s final, the final modifier is allowed only for methods and classes",
               CG(active_class_entry)->name, var_name->u.constant.value.str.val);
}
```

没报错就初始化变量

```C
ALLOC_ZVAL(property);   //  分配内存
 
if (value) {    //  成员变量有初始化数据
    *property = value->u.constant;
} else {
    INIT_PZVAL(property);
    Z_TYPE_P(property) = IS_NULL;
}
```

在初始化过程中，程序会先分配内存，如果这个成员变量有初始化的数据，则将数据直接赋值给该属性， 否则初始化ZVAL，并将其类型设置为IS_NULL。在初始化过程完成后，程序通过调用 **zend_declare_property_ex** 函数**将此成员变量添加到指定的类结构中。**

以上为成员变量的初始化和注册成员变量的过程，常规的成员变量最后都会注册到类的 **default_properties** 字段。 在我们平时的工作中，可能会用不到上面所说的这些过程，但是我们可能会使用get_class_vars()函数来查看类的成员变量。 此函数返回由类的默认属性组成的关联数组，这个数组的元素以 varname => value 的形式存在。



**get_class_vars()函数**核心代码如下：

```C
if (zend_lookup_class(class_name, class_name_len, &pce TSRMLS_CC) == FAILURE) {
    RETURN_FALSE;
} else {
    array_init(return_value);
    zend_update_class_constants(*pce TSRMLS_CC);
    add_class_vars(*pce, &(*pce)->default_properties, return_value TSRMLS_CC);
    add_class_vars(*pce, CE_STATIC_MEMBERS(*pce), return_value TSRMLS_CC);
}
```
zend_lookup_class函数查找名为class_name的类，并将赋值给pce变量。 这个查找的过程最核心是一个HashTable的查找函数zend_hash_quick_find，它会查找EG(class_table)。 判断类是否存在，如果存在则直接返回。如果不存在，则需要判断是否可以自动加载，如果可以自动加载，则会加载类后再返回。 如果不能找到类，则返回FALSE。**如果找到了类，则初始化返回的数组，更新类的静态成员变量，添加类的成员变量到返回的数组**。 



这里针对类的静态成员变量有一个更新的过程

#### 类的静态成员变量是所有实例共用的，它归属于这个类，因此它也叫做类变量。 在PHP的类结构中，类本身的静态变量存放在类结构的 **default_static_members** 字段中

```php
class Tipi {
    public static $var = 10;
}
 
Tipi::$var;
```

我们要调用一个类的静态变量，当然要先找到这个类，然后再获取这个类的变量。 从PHP源码来看，这是由于在编译时其调用了zend_do_fetch_static_member函数， 而在此函数中又调用了zend_do_fetch_class函数， 从而会生成ZEND_FETCH_CLASS中间代码。它所对应的执行函数为 **ZEND_FETCH_CLASS_SPEC_CONST_HANDLER**。 此函数会调用zend_fetch_class函数（Zend/zend_execute_API.c）。 而zend_fetch_class函数最终也会调用 **zend_lookup_class_ex** 函数查找类，这与前面的查找方式一样



找到了类，接着应该就是查找类的静态成员变量，其最终调用的函数为：zend_std_get_static_property。 这里由于第二个参数的类型为 ZEND_FETCH_STATIC_MEMBER。这个函数最后是从 **static_members** 字段中查找对应的值返回。 而在查找前会和前面一样，**执行zend_update_class_constants函数，从而更新此类的所有静态成员变量**。



![](http://www.php-internals.com/images/book/chapt05/05-02-01-class-static-vars.jpg)

