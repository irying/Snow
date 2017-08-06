PHP使用主要虚拟机（Zend虚拟机，*译注：HHVM也是一种执行PHP代码的虚拟机，但很显然Zend虚拟机还是目前的主流*）可以分为两大部分，它们是紧密相连的：

- 编译栈（compile stack）：识别PHP语言指令，把它们转换为中间形式
- 执行栈（execution stack）：获取中间形式的代码指令并在引擎上执行，引擎是用C或者汇编编写成的



从一句代码开始

```C
ZEND_API void (*zend_execute)(zend_op_array *op_array TSRMLS_DC);
```

这是一个全局的函数指针，它的作用就是执行**“PHP代码文件解析完的转成的”**zend_op_array。 和zend_execute相同的还有一个zedn_execute_internal函数，它用来执行内部函数。 **在PHP内核启动时(zend_startup)时，这个全局函数指针将会指向execute函数。 注意函数指针前面的修饰符ZEND_API，这是ZendAPI的一部分。 在zend_execute函数指针赋值时，还有PHP的中间代码编译函数zend_compile_file（文件形式）和zend_compile_string(字符串形式)。**



```C
zend_compile_file = compile_file;
zend_compile_string = compile_string;
zend_execute = execute;
zend_execute_internal = NULL;
zend_throw_exception_hook = NULL;
```

这几个全局的函数指针均只调用了系统默认实现的几个函数，比如compile_file和compile_string函数， 他们都是以全局函数指针存在，这种实现方式在PHP内核中比比皆是，其优势在于更低的耦合度，甚至可以定制这些函数。 比如在APC等opcode优化扩展中就是通过替换系统默认的zend_compile_file函数指针为自己的函数指针my_compile_file， 并且在my_compile_file中增加缓存等功能。

到这里我们找到了中间代码执行的最终函数：

```C
if ((ret = EX(opline)->handler(execute_data TSRMLS_CC)) > 0) {
}
```

execute(Zend/zend_vm_execute.h)。 在这个函数中所有的中间代码的执行最终都会调用handler。**这个handler是什么呢？**



### 这个handler是什么呢

在Zend/zend_language_scanner.c文件中有compile_string函数的实现。 在此函数中，**当解析完中间代码后，一般情况下，它会执行pass_two**(Zend/zend_opcode.c)函数。 pass_two这个函数，从其命名上真有点看不出其意义是什么。 但是我们关注的是在函数内部，**它遍历整个中间代码集合， 调用ZEND_VM_SET_OPCODE_HANDLER(opline);为每个中间代码设置处理函数**。 **<u>ZEND_VM_SET_OPCODE_HANDLER是zend_vm_set_opcode_handler函数的接口宏</u>**



```C
static opcode_handler_t zend_vm_get_opcode_handler(zend_uchar opcode, zend_op* op)
{
        static const int zend_vm_decode[] = {
            _UNUSED_CODE, /* 0              */
            _CONST_CODE,  /* 1 = IS_CONST   */
            _TMP_CODE,    /* 2 = IS_TMP_VAR */
            _UNUSED_CODE, /* 3              */
            _VAR_CODE,    /* 4 = IS_VAR     */
            _UNUSED_CODE, /* 5              */
            _UNUSED_CODE, /* 6              */
            _UNUSED_CODE, /* 7              */
            _UNUSED_CODE, /* 8 = IS_UNUSED  */
            _UNUSED_CODE, /* 9              */
            _UNUSED_CODE, /* 10             */
            _UNUSED_CODE, /* 11             */
            _UNUSED_CODE, /* 12             */
            _UNUSED_CODE, /* 13             */
            _UNUSED_CODE, /* 14             */
            _UNUSED_CODE, /* 15             */
            _CV_CODE      /* 16 = IS_CV     */
        };
        return zend_opcode_handlers[opcode * 25 
                + zend_vm_decode[op->op1.op_type] * 5 
                + zend_vm_decode[op->op2.op_type]];
}
 
ZEND_API void zend_vm_set_opcode_handler(zend_op* op)
{
    op->handler = zend_vm_get_opcode_handler(zend_user_opcodes[op->opcode], op);
}
```



#### zend_vm_set_opcode_handler函数定义在Zend/zend_vm_execute.h文件。 handler所指向的方法基本都存在于Zend/zend_vm_execute.h文件文件。



接着讲操作数，就是op，虽然知道handler在哪个文件，但编译的时候要根据操作数设置不同handler。



我们知道每个OPCode的handler最多可以使用两个操作数：op1和op2。每个操作数都表示一个OPCode的“参数（parameter）”。例如，ZEND_ASSIGN这个OPCode的第一个参数是你要赋值的PHP变量，第二个操作数是你要给第一个操作数赋的值。这个OPCode的结果不会用到。（译注：这一点很有意思，**赋值语句会返回一个值**，这就是我们可以使用$a=$b=1这个语言结构的原因，基本很多语言都是这么做的，<u>**但是理论上语句（statement）是不返回值的，例如if语句或者for语句都是不会返回值的，这也是不能把它们赋值给某个变量的原因，在程序设计语言中，能够返回值的都是表达式（expression），有些人吐槽这种设计不合理，因为这违反了一致性原则）**</u>



```
IS_CV ：编译变量（Compiled Variable）：这个操作数类型表示一个PHP变量：以$something形式在PHP脚本中出现的变量
IS_VAR ： 供VM内部使用的变量，它可以被其他的OPCode重用，跟$php_variable很像，只是只能供VM内部使用
IS_TMP_VAR ： VM内部使用的变量，但是不能被其他的OPCode重用
IS_CONST ： 表示一个常量，它们都是只读的，它们的值不可改变
IS_UNUSED ：这个表示操作数没有值：这个操作数没有包含任何有意义的东西，可以忽略
```

ZEND VM的这些类型规范很重要，它们会直接影响整个executor的性能，以及在executor的内存管理中扮演重要的角色。

#### 当某个OPCode的handler想读取（fetch/read）保存在某个操作数中的信息时，executor不会执行同样的代码来读取这些信息：而是会针对不同的操作数类型调用不同的（读取）代码。



如果不考虑性能问题，那么ZEND_ADD的handler代码将如下所示（经过简化的伪代码）：

```C
int ZEND_ADD(zend_op *op1, zend_op *op2)
{
    void *op1_value;
    void *op2_value;
    switch (op1->type) {
        case IS_CV:
            op1_value = read_op_as_a_cv(op1);
        break;
        case IS_VAR:
            op1_value = read_op_as_a_var(op1);
        break;
        case IS_CONST:
            op1_value = read_op_as_a_const(op1);
        break;
        case IS_TMP_VAR:
            op1_value = read_op_as_a_tmp(op1);
        break;
        case IS_UNUSED:
            op1_value = NULL;
        break;
    }
    /* ... 对op2做同样的事情 .../
    /* 对op1_value和op2_value做一些事情 (执行一个算术加法运算?) */
}
```



这段代码看起来没问题，但是有性能问题，怎么说？

我们是在设计某个OPCode的handler，这个handler可能会在执行PHP脚本的时候被调用很多次。**如果每次调用这个handler时都不得不先获取它的操作数的类型，然后根据不同的类型执行不同的读取（fetch/read）代码，这显然不利于程序的性能**。



我们假设某个OPCode的handler要读取的操作数（op1或者op2）的类型是IS_CV，这表示这个操作数是某个在PHP代码中定义过的$variable，这个handler会首先会查找符号表（symbol table），**符号表里存放了每个代码中已声明的变量。查找符号表的工作一旦结束，在此我们假设查找是成功的——找到了一个编译变量（Compiled Variable）**，那么跟当前执行的OPCode处在同一个OPArray中的OPCode（位于当前OPCode后面）非常非常有可能会再次用到这个操作数的信息。所以当第一次读取成功后，executor会把读取的信息缓存在OPArray中，这样之后要再次读取这个操作数的信息时就快很多。

上面是对IS_CV这个类型的解释说明，这也同样适用于其他的类型：我们可以对任意OPCode的handler的操作数访问进行优化，只要我们知道它们的类型信息（这个操作数可共享么？它需要被释放么？之后还可能重用它么？等等）。



```C
$a + $b; // IS_CV + IS_CV
1 + $a;  // IS_CONST + IS_CV
foo() + 3 // IS_VAR + IS_CONST
!$a + 3;  // IS_TMP + IS_CONST (此处会产生两个OPCode，但只显示了一个)
```



PHP源码中对ZEND_ADD这个OPCode的handler的定义

```C
ZEND_VM_HANDLER(1, ZEND_ADD, CONST|TMP|VAR|CV, CONST|TMP|VAR|CV)
{
    USE_OPLINE
    zend_free_op free_op1, free_op2;
    SAVE_OPLINE();
    fast_add_function(&EX_T(opline->result.var).tmp_var,
        GET_OP1_ZVAL_PTR(BP_VAR_R),
        GET_OP2_ZVAL_PTR(BP_VAR_R) TSRMLS_CC);
    FREE_OP1();
    FREE_OP2();
    CHECK_EXCEPTION();
    ZEND_VM_NEXT_OPCODE();
}
```

看看这个奇怪的函数的签名，它甚至都不符合有效C语法（因此它不可能通过C编译器的编译）。这行代码表示ZEND_ADD的handler的第一个操作数op1可以接受CONST、TMP、VAR或者CV类型，op2也是如此。



#### 那这段代码有什么用？

包含这段代码的源文件是[zend_vm_def.h](http://lxr.php.net/xref/PHP_5_6/Zend/zend_vm_def.h)，这只是一个模板文件，**它会被传给一个处理工具（processor），这个工具会生成每个handler的代码（符合C语言语法的程序），它会对所有操作数类型进行排列组合，然后生成专属于组合中的每一项的handler函数。**



我们来算个数，**op1可接受5种不同的类型，op2也可以接受5种不同的类型，那个op1和op2的类型组合就有25种情况**：上面的工具会为ZEND_ADD生成25个不同的专用于处理特定类型组合的handler函数，这些函数会被写入一个文件中，这个文件会作为PHP源码的一部分被编译。



最终生成的文件名为[zend_vm_execute.h](http://lxr.php.net/xref/PHP_5_6/Zend/zend_vm_execute.h)

PHP5.6支持167个OPCode，假设这167个OPCode每个都有可以接受5种操作数类型的op1和op2，那么最终生成的文件会包含4175个C函数。

实际上并非每个OPCode都支持5种不同的操作数类型，所以最终生成的函数个数会小于上面的数字。例如：

```C
ZEND_VM_HANDLER(84, ZEND_FETCH_DIM_W, VAR|CV, CONST|TMP|VAR|UNUSED|CV)
```



#### 结论：

- zend_vm_def.h并非有效的C文件，它描述了每个OPCode的handler的特点（使用一种与C接近的自定义语法），每个handler的特点依赖于它的op1和op2的类型，每个操作数最多支持5种类型
- zend_vm_def.h会被传递给一个名为[zend_vm_gen.php](http://lxr.php.net/xref/PHP_5_6/Zend/zend_vm_gen.php)的PHP脚本，这个脚本位于PHP源码中，它会分析zend_vm_def.h中的特殊语法，会用到很多正则表达式匹配，最终会生成出zend_vm_execute.h这个文件
- zend_vm_def.h在编译PHP源码时不会被处理（这是很显然的）
- zend_vm_execute.h是解析zend_vm_def.h后的输出文件，它包含了符合C语法的代码，它是VM executor的心脏：每个OPCode的专有handler函数都存放在这个文件里，很显然这是一个非常重要的文件
- 当你从源码编译PHP时，PHP源码会提供一个默认的zend_vm_execute.h文件，不过如果你想修改（hack）PHP源码，例如你想添加一个新的OPCode，或者是**修改一个已存在的OPCode的行为，你必须先修改（hack）zend_vm_def.h，然后再重新生成zend_vm_execute.h文件**



继续回到上面的handler文件：

```C
ZEND_VM_HANDLER(1, ZEND_ADD, CONST|TMP|VAR|CV, CONST|TMP|VAR|CV)
```

```c
static int ZEND_FASTCALL  ZEND_ADD_SPEC_CONST_CONST_HANDLER(ZEND_OPCODE_HANDLER_ARGS) { /* handler code */ }
static int ZEND_FASTCALL  ZEND_ADD_SPEC_CONST_TMP_HANDLER(ZEND_OPCODE_HANDLER_ARGS) { /* handler code */ }
static int ZEND_FASTCALL  ZEND_ADD_SPEC_CONST_VAR_HANDLER(ZEND_OPCODE_HANDLER_ARGS) { /* handler code */ }
static int ZEND_FASTCALL  ZEND_ADD_SPEC_CONST_CV_HANDLER(ZEND_OPCODE_HANDLER_ARGS) { /* handler code */ }
static int ZEND_FASTCALL  ZEND_ADD_SPEC_TMP_CONST_HANDLER(ZEND_OPCODE_HANDLER_ARGS) { /* handler code */ }
static int ZEND_FASTCALL  ZEND_ADD_SPEC_TMP_TMP_HANDLER(ZEND_OPCODE_HANDLER_ARGS)  { /* handler code */ }
```

这个函数名是动态生成的，它的生成模式是：ZEND_{OPCODE-NAME}_SPEC_{OP1-TYPE}_{OP2-TYPE}_HANDLER()。



必须说明，这是编译的时候设置的。

当PHP编译器在把PHP语言写的程序编译成OPCode时，它知道每个OPCode所接受的op1和op2的类型（因为它是编译器所以它必须知道，这是它的职责）。所以PHP编译器会直接生成一个使用正确的专有handler的OPArray：**在执行过程中不会存在其他的选择，不需要使用switch()：在运行期直接执行OPCode的专有handler显然会更高效一些。不过如果你修改你的PHP程序，那你必须得重新编译生成一个新的OPArray，这就是OPCode缓存要解决的问题。**



```C
static int ZEND_FASTCALL  ZEND_ADD_SPEC_CONST_CONST_HANDLER(ZEND_OPCODE_HANDLER_ARGS) /* CONST_CONST */
{
    USE_OPLINE
    SAVE_OPLINE();
    fast_add_function(&EX_T(opline->result.var).tmp_var,
        opline->op1.zv, /* fetch op1 value */
        opline->op2.zv TSRMLS_CC); /* fetch op2 value */
    CHECK_EXCEPTION();
    ZEND_VM_NEXT_OPCODE();
}
static int ZEND_FASTCALL  ZEND_ADD_SPEC_CV_CV_HANDLER(ZEND_OPCODE_HANDLER_ARGS) /* CV_CV */
{
    USE_OPLINE
    SAVE_OPLINE();
    fast_add_function(&EX_T(opline->result.var).tmp_var,
        _get_zval_ptr_cv_BP_VAR_R(execute_data, opline->op1.var TSRMLS_CC), /* fetch op1 value */
        _get_zval_ptr_cv_BP_VAR_R(execute_data, opline->op2.var TSRMLS_CC) TSRMLS_CC); /* fetch op2 value */
    CHECK_EXCEPTION();
    ZEND_VM_NEXT_OPCODE();
}
```

在CONST_CONST的handler中（op1和op2都是CONST），我们直接获取这个操作数的zval值。此时不需要做任何其他事情，例如增加或者减少一个引用计数的值，或者是释放操作数的值：这个值是不可变的，只需要读取它，这样就可以收工了。

不过对于CV_CV的handler（op1和op2都是CV，编译变量），我们必须访问它们的值，增加它们的引用计数（refcount）（这是由于我们现在要用到了它们），并且为了方便以后使用而把它们的值缓存起来：**_get_zval_ptr_cv_BP_VAR_R()就是做这些事情的。我们从这个函数的命名可以看出这是一个“R”读取操作：这表示只读取操作数的值，如果这个变量不存在，这个函数会产生一个notice：未定义变量（undefined variable）。对于”W“访问情况则会有些不同，如果这个变量不存在，我们只需要创建它，而不会产生任何警告或者notice，PHP不就是这么工作的么**？;-)