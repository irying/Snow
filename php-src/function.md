php函数

```php
function foo($var) {
    echo $var;
}
```

词法分析

```
<ST_IN_SCRIPTING>"function" {
    return T_FUNCTION;
}
```

#### 1.function将会生成T_FUNCTION标记。在获取这个标记后，我们开始语法分析。

```C
function:
    T_FUNCTION { $$.u.opline_num = CG(zend_lineno); }
;
 
is_reference:
        /* empty */ { $$.op_type = ZEND_RETURN_VAL; }
    |   '&'         { $$.op_type = ZEND_RETURN_REF; }
;
 
unticked_function_declaration_statement:
        function is_reference T_STRING {
zend_do_begin_function_declaration(&$1, &$3, 0, $2.op_type, NULL TSRMLS_CC); }
            '(' parameter_list ')' '{' inner_statement_list '}' {
                zend_do_end_function_declaration(&$1 TSRMLS_CC); }
;
```

 function is_reference T_STRING，表示function关键字，是否引用，函数名。

执行编译函数为**zend_do_begin_function_declaration**。在 Zend/zend_complie.c文件中找到其实现如下：

```C
void zend_do_begin_function_declaration(znode *function_token, znode *function_name,
 int is_method, int return_reference, znode *fn_flags_znode TSRMLS_DC) /* {{{ */
{
    ...//省略
    function_token->u.op_array = CG(active_op_array);
    lcname = zend_str_tolower_dup(name, name_len);
 
    orig_interactive = CG(interactive);
    CG(interactive) = 0;
    init_op_array(&op_array, ZEND_USER_FUNCTION, INITIAL_OP_ARRAY_SIZE TSRMLS_CC);
    CG(interactive) = orig_interactive;
 
     ...//省略
 
    if (is_method) {
        ...//省略 类方法 在后面的类章节介绍
    } else {
        zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
 
 
        opline->opcode = ZEND_DECLARE_FUNCTION;
        opline->op1.op_type = IS_CONST;
        build_runtime_defined_function_key(&opline->op1.u.constant, lcname,
            name_len TSRMLS_CC);
        opline->op2.op_type = IS_CONST;
        opline->op2.u.constant.type = IS_STRING;
        opline->op2.u.constant.value.str.val = lcname;
        opline->op2.u.constant.value.str.len = name_len;
        Z_SET_REFCOUNT(opline->op2.u.constant, 1);
        opline->extended_value = ZEND_DECLARE_FUNCTION;
        zend_hash_update(CG(function_table), opline->op1.u.constant.value.str.val,
            opline->op1.u.constant.value.str.len, &op_array, sizeof(zend_op_array),
             (void **) &CG(active_op_array));
    }
 
}
/* }}} */
```

可以看到opcode为**ZEND_DECLARE_FUNCTION**

根据这个opcode和操作数，我们去找它的handler为 **ZEND_DECLARE_FUNCTION_SPEC_HANDLER**。在Zend/zend_vm_execute.h 文件中。

此函数只调用了函数do_bind_function。其调用代码为：

```C
do_bind_function(EX(opline), EG(function_table), 0);
```



在这个函数中将EX(opline)所指向的函数添加到EG(function_table)中，并判断是否已经存在相同名字的函数，如果存在则报错。 **<u>EG(function_table)用来存放执行过程中全部的函数信息，相当于函数的注册表</u>**。 它的结构是一个HashTable，所以在do_bind_function函数中添加新的函数使用的是HashTable的操作函数**zend_hash_add**



上面是声明，就是词法分析--语法分析拿到执行编译的函数--生成中间码，就是生成opcode—根据opcode跟操作数找handler--handler处理就是调用相关函数—添加到函数的注册表。





### 执行过程的函数情况

在执行过程中，会将运行时信息存储于_zend_execute_data

```C
struct _zend_execute_data {
    //...省略部分代码
    zend_function_state function_state;
    zend_function *fbc; /* Function Being Called */
    //...省略部分代码
};
```

在程序初始化的过程中，function_state也会进行初始化，function_state由两个部分组成：

```c
typedef struct _zend_function_state {
    zend_function *function;
    void **arguments;
} zend_function_state;
```

***arguments是一个指向函数参数的指针，而函数体本身则存储于*function中， *function是一个zend_function结构体， 它最终存储了用户自定义函数的一切信息，它的具体结构是这样的：

```C
typedef union _zend_function {
    zend_uchar type;    /* 如用户自定义则为 #define ZEND_USER_FUNCTION 2
                            MUST be the first element of this struct! */
 
    struct {
        zend_uchar type;  /* never used */
        char *function_name;    //函数名称
        zend_class_entry *scope; //函数所在的类作用域
        zend_uint fn_flags;     // 作为方法时的访问类型等，如ZEND_ACC_STATIC等  
        union _zend_function *prototype; //函数原型
        zend_uint num_args;     //参数数目
        zend_uint required_num_args; //需要的参数数目
        zend_arg_info *arg_info;  //参数信息指针
        zend_bool pass_rest_by_reference;
        unsigned char return_reference;  //返回值 
    } common;
 
    zend_op_array op_array;   //函数中的操作
    zend_internal_function internal_function;  
} zend_function;
```



#### 是个联合体

 要理解这个结构，首先你要理解他的设计目标： zend_internal_function, zend_function,zend_op_array可以安全的互相转换(The are not identical structs, but all the elements that are in “common” they hold in common, thus the can safely be casted to each other);

具体来说，**当在op code中通过ZEND_DO_FCALL调用一个函数的时候，ZE会在函数表中，根据名字（其实是lowercase的函数名字，这也就是为什么PHP的函数名是大小写不敏感的)查找函数， 如果找到，返回一个zend_function结构的指针**(仔细看这个上面的zend_function结构), 然后判断type,如果是ZEND_INTERNAL_FUNCTION， 那么ZE就调用zend_execute_internal,通过zend_internal_function.handler来执行这个函数， 如果不是，就调用zend_execute来执行这个函数包含的zend_op_array.



#### 2.接着说下ZEND_INTERNAL_FUNCTION

跟上面用户定义的函数不同，用户定义的函数是编译时候声明并加到函数的符号表；

而内部函数是在模块初始化时，**ZE会遍历每个载入的扩展模块，然后将模块中function_entry中指明的每一个函数(module->functions)， 创建一个zend_internal_function结构， 并将其type设置为ZEND_INTERNAL_FUNCTION，将这个结构填入全局的函数表(HashTable结构）**;

内部函数的结构与用户自定义的函数结构基本类似，有一些不同，

- 调用方法，handler字段. 如果是ZEND_INTERNAL_FUNCTION， **<u>那么ZE就调用zend_execute_internal，通过zend_internal_function.handler来执行这个函数。</u>** 而用户自定义的函数需要生成中间代码，然后通过中间代码映射到相对就把方法调用。
- 内置函数在结构中**<u>多了一个module字段，表示属于哪个模块</u>**。不同的扩展其模块不同。
- type字段，在用户自定义的函数中，type字段几乎无用，而内置函数中的type字段作为几种内部函数的区分。



最常见的ZEND_FUNCTION

```c
#define PHP_FUNCTION                ZEND_FUNCTION
 
#define ZEND_FUNCTION(name)         ZEND_NAMED_FUNCTION(ZEND_FN(name))
#define ZEND_NAMED_FUNCTION(name)   void name(INTERNAL_FUNCTION_PARAMETERS)
#define ZEND_FN(name)               zif_##name
```

其中，zif是zend internal function的意思，zif_前缀是可供PHP语言调用的函数在C语言中的函数名称前缀。

```C
ZEND_FUNCTION(walu_hello)
{
    php_printf("Hello World!\n");
}
```

```c
void zif_walu_hello(INTERNAL_FUNCTION_PARAMETERS)
{
    php_printf("Hello World!\n");
}
```



#### 3.转换

在函数调用的执行代码中我们会看到这样一些强制转换，zend_internal_function，zend_function，zend_op_array这三种结构在一定程序上存在公共的元素， 于是这些元素以联合体的形式共享内存，并且在执行过程中对于一个函数，这三种结构对应的字段在值上都是一样的， 于是可以在一些结构间发生完美的强制类型转换。 可以转换的列表如下：

- zend_function可以与zend_op_array互换
- zend_function可以与zend_internal_function互换

```C
EX(function_state).function = (zend_function *) op_array;
 
或者：
 
EG(active_op_array) = (zend_op_array *) EX(function_state).function;
```

注：函数设置及注册过程见 Zend/zend_API.c文件中的 **zend_register_functions**函数。这个函数除了处理函数，也处理类的方法，包括那些魔术方法。