



1.得到一个zend_op

如下代码是在编译器遇到print语句的时候进行编译的函数:

```C
void zend_do_print(znode *result，const znode *arg TSRMLS_DC)
{
    zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
 
    opline->result.op_type = IS_TMP_VAR;
    opline->result.u.var = get_temporary_variable(CG(active_op_array));
    opline->opcode = ZEND_PRINT;
    opline->op1 = *arg;
    SET_UNUSED(opline->op2);
    *result = opline->result;
}
```



这个函数新创建一条zend_op，**将返回值的类型设置为临时变量(IS_TMP_VAR)，并为临时变量申请空间， 随后指定opcode为ZEND_PRINT，并将传递进来的参数赋值给这条opcode的第一个操作数。**这样在最终执行这条opcode的时候， Zend引擎能获取到足够的信息以便输出内容。

还有echo的

```C
void zend_do_echo(const znode *arg TSRMLS_DC)
{
    zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
 
    opline->opcode = ZEND_ECHO;
    opline->op1 = *arg;
    SET_UNUSED(opline->op2);
}
```

##### 可以看到echo处理除了指定opcode以外，还将echo的参数传递给op1，这里并没有设置opcode的result结果字段。 从这里我们也能看出print和echo的区别来，print有返回值，而echo没有，这里的没有和返回null是不同的， 如果尝试将echo的值赋值给某个变量或者传递给函数都会出现语法错误。



2.zend_op这个结构体

```C
struct _zend_op {
    opcode_handler_t handler;   /* The true C function to run */
    znode_op op1; /* operand 1 */
    znode_op op2; /* operand 2 */
    znode_op result; /* result */
    ulong extended_value; /* additionnal little piece of information */
    uint lineno;
    zend_uchar opcode; /* opcode number */
    zend_uchar op1_type; /* operand 1 type */
    zend_uchar op2_type; /* operand 2 type */
    zend_uchar result_type; /* result type */
};
```

你可以通过设想一个简单的计算器来理解OPCode（严肃点，我说真的）：这个计算器可以接收两个操作数（op1和op2），你请求它执行一个操作（handler），然后它返回一个结果（result）给你，**如果算术运算出现了溢出则会把溢出的部分舍弃掉（extended_value)**。PHP不像汇编那么底层， 在脚本实际执行的时候可能还需要其他更多的信息，extended_value字段就保存了这类信息， 其中的result域则是保存该指令执行完成后的结果。



3.Handler

Zend VM的每个OPCode的工作方式都完全相同：它们都有一个handler（*译注：在Zend VM中，handler是一个函数指针，它指向OPCode对应的处理函数的地址，这个处理函数就是用于实现OPCode具体操作的，为了简洁起见，这个单词不做翻译*），这是一个C函数，这个函数就包含了执行这个OPCode时会运行的代码（例如“add”，它就会执行一个基本的加法运算）。每个handler都可以使用0、1或者2个操作数：op1和op2，这个函数运行后，**它会后返回一个结果，有时也会返回一段信息（extended_value）。**

ZEND_ADD这个OPCode的handler

```c
// 头部定义了四个东西
//   1. 这个opcode的ID是1
//   2. 这个opcode的名称是ZEND_ADD
//   3. 这个opcode的第一个操作数可以接受CONST，TMP，VAR和CV四种类型
//   4. 这个opcode的第二个操作数也可以接受CONST，TMP，VAR和CV四种类型
ZEND_VM_HANDLER(1, ZEND_ADD, CONST|TMP|VAR|CV, CONST|TMP|VAR|CV)
{
    // USE_OPLINE表示我们想以`opline`的形式访问这个zend_op
    // 这对于所有要访问的操作数或者需要设定一个返回值的opcode来说是必须的
    USE_OPLINE
    // 对于每个会被访问的操作数，都必须定义一个free_op*变量
    // 它用于表示这个操作数是否需要释放
    zend_free_op free_op1, free_op2;
    // SAVE_OPLINE() 是实际用于把zend_op保存到`opline`的代码
    // USE_OPLINE 仅仅只是声明
    SAVE_OPLINE();
    // 调用快速加法函数（fast add function）
    fast_add_function(
        // 这行代码告诉这个函数把结果保存在临时结果变量中
        // EX_T 会以 opline->result.var表示的ID来访问临时变量
        &EX_T(opline->result.var).tmp_var,
        // 获取第一个操作数进行读操作 (BP_VAR_R中的R表示读操作)
        GET_OP1_ZVAL_PTR(BP_VAR_R),
        // 获取第二个操作数进行读操作
        GET_OP2_ZVAL_PTR(BP_VAR_R) TSRMLS_CC);
    // 释放这两个操作数 (如果有必要的话)
    FREE_OP1();
    FREE_OP2();
    // 检查异常。异常实际上会在任何地方发生，所以你基本上必须在所有opcode中检查它们。如果你不确定
    // 那就尽管加上这行检查的代码
    CHECK_EXCEPTION();
    // 跳转到下一个opcode的执行
    ZEND_VM_NEXT_OPCODE();
}
```

ZEND_ADD这个OPCode的handler中会调用fast_add_function()函数（一个存放在其他地方的C函数），这段代码中给这个函数传入了三个参数：result（结果）、op1和op2。从这段代码我们可以看出真正执行加法运算的代码是在fast_add_function()这个函数中。



4.单说ZEND_VM_NEXT_OPCODE()

```C
#define ZEND_VM_NEXT_OPCODE() /  
    CHECK_SYMBOL_TABLES() /  
    EX(opline)++; /  
    ZEND_VM_CONTINUE()  
   
#define ZEND_VM_SET_OPCODE(new_op) /  
    CHECK_SYMBOL_TABLES() /  
    EX(opline) = new_op  
   
#define ZEND_VM_JMP(new_op) /  
    CHECK_SYMBOL_TABLES() /  
    if (EXPECTED(!EG(exception))) { /  
        EX(opline) = new_op; /  
    } /  
    ZEND_VM_CONTINUE()  
   
#define ZEND_VM_INC_OPCODE() /  
    EX(opline)++
```



ZEND_VM_NEXT_OPCODE()：移动到下一条op，返回0，不进入switch，循环继续（这个是最常用到的）
ZEND_VM_SET_OPCODE(new_op)：当前opline设置成new_op
ZEND_VM_JMP(new_op) ：当前opline设置成new_op，返回0，不进入switch，循环继续
ZEND_VM_INC_OPCODE()：仅仅移动到下一条op

#### 编译器的工作就是当在编译PHP脚本时，脚本中的PHP语法会被转换为多个OPCode，一个接着一个。

这意味着PHP编译器会做一件事情：**把PHP脚本转换为一个“OP数组（OP array）”，它是一个包含多个OPCode的数组。每个OPCode的handler都会以调用ZEND_VM_NEXT_OPCODE()结束，它会告诉executor提取（fetch）紧接着的下一个OPCode，然后执行它，这个过程会不断进行。**

所有这些都发生在一个循环里，它的代码如下（经过简化后的代码）

```C
ZEND_API void execute_ex(zend_execute_data *execute_data TSRMLS_DC)
{
    zend_bool original_in_execution;
    original_in_execution = EG(in_execution);
    EG(in_execution) = 1;
zend_vm_enter:
    execute_data = i_create_execute_data_from_op_array(EG(active_op_array), 1 TSRMLS_CC);
    while (1) {  /* infinite dispatch loop */
        int ret;
        if ((ret = execute_data->opline->handler(execute_data TSRMLS_CC)) > 0) { /* do the job */
            switch (ret) {
                case 1:
                    EG(in_execution) = original_in_execution;
                    return; /* exit from the infinite loop */
                case 2:
                    goto zend_vm_enter;
                    break;
                case 3:
                    execute_data = EG(current_execute_data);
                    break;
                default:
                    break;
            }
        }
    } /* end of infinite dispatch loop */
    zend_error_noreturn(E_ERROR, "Arrived at end of main loop which shouldn't happen");
}
```

```c
#define ZEND_VM_NEXT_OPCODE() \
CHECK_SYMBOL_TABLES() \
ZEND_VM_INC_OPCODE(); \
ZEND_VM_CONTINUE()
#define ZEND_VM_INC_OPCODE() \
OPLINE++
#define OPLINE execute_data->opline
#define ZEND_VM_CONTINUE()         return 0
#define ZEND_VM_RETURN()           return 1
#define ZEND_VM_ENTER()            return 2
#define ZEND_VM_LEAVE()            return 3
```

*实际上并非所有的handler最后都会调用ZEND_VM_NEXT_OPCODE()这个宏，例如ZEND_GENERATOR_RETURN这个OPCode的handler最后调用的就是ZEND_VM_RETURN()这个宏，它会返回1，这种情况下会退出循环，我们通过上面的宏的定义也可以看出，**所有调用ZEND_VM_NEXT_OPCODE()这个宏的handler都会ZEND_VM_CONTINUE()这个宏返回0，这种情况下会进入下一次循环，也就是执行下一个OPCode**）*

**每个脚本都会以一个RETURN结束，如果不是这样的话：整个循环会无限执行下去，这显然是不合理的。**

****所以PHP编译器被设计为不管编译什么代码都会在编译出的OP数组的最后加一个RETURN的OPCode。这意味着如果编译一个空的PHP脚本（不包含任何代码），所产生的OPArray中也会包含一个唯一的OPCode：ZEND_RETURN。当它被加载到VM的执行分发循环时，它会执行这个OPCode的handler的代码，那就是让VM返回：空PHP脚本不会做其他任何事情。

----------------------------------------------------------------------------------------------------------------



看看编译产生的zend_op_array，zend_op_array就是包含上面zend_op的数组



```c
struct _zend_op_array {  
    /* Common elements */ 
    zend_uchar type;  
    char *function_name;          
    zend_class_entry *scope;  
    zend_uint fn_flags;  
    union _zend_function *prototype;  
    zend_uint num_args;  
    zend_uint required_num_args;  
    zend_arg_info *arg_info;  
    zend_bool pass_rest_by_reference;  
    unsigned char return_reference;  
    /* END of common elements */ 
   
    zend_bool done_pass_two;  
   
    zend_uint *refcount;  
   
    zend_op *opcodes;  
    zend_uint last, size;  
   
    zend_compiled_variable *vars;  
    int last_var, size_var;  
   
    zend_uint T;  
   
    zend_brk_cont_element *brk_cont_array;  
    int last_brk_cont;  
    int current_brk_cont;  
   
    zend_try_catch_element *try_catch_array;  
    int last_try_catch;  
   
    /* static variables support */ 
    HashTable *static_variables;  
   
    zend_op *start_op;  
    int backpatch_count;  
   
    zend_uint this_var;  
   
    char *filename;  
    zend_uint line_start;  
    zend_uint line_end;  
    char *doc_comment;  
    zend_uint doc_comment_len;  
    zend_uint early_binding; /* the linked list of delayed declarations */ 
   
    void *reserved[ZEND_MAX_RESERVED_RESOURCES];  
};  
   
typedef struct _zend_op_array zend_op_array;  
```

- 当前脚本的文件名（const char *filename），以及会被编译为OPArray的PHP**<u>脚本在文件中的开始行数（zend_uint line_start）和结束行数（zend_uint line_end）</u>**
- 文档注释的信息（const char *doc_comment）：PHP脚本中使用“/**”注释的信息
- 引用计数（zend_uint *refcount），OPArray本身也可能会在其他地方共享，所以需要记录它的引用情况
- 编译变量列表（zend_compiled_variable *vars）。编译变量（compiled variable）是所有PHP脚本中使用的变量（以$something的形式出现）
- 临时变量列表：临时变量用于保存运算的临时结果，这些结果不会显示地在PHP脚本中使用（不是使用$something的形式在PHP脚本中出现，但是是会被真正用到的中间数据）（*译注：从上面的结构体中我看不出哪个是保存临时变量的字段*）
- try-catch-finally的信息（zend_try_catch_element *try_catch_array），executor需要这些信息来实现正确的跳转
- break-continue的信息（zend_brk_cont_element *brk_cont_array），executor需要这些信息来实现正确的跳转
- 静态变量的列表（HashTable *static_variables）。静态变量会被特殊处理，因为它们的信息需要维持到PHP生命周期的最后时刻（大体上是这样）
- 字面量（zend_literal *literals）。字面量是指任何在编译期就知道它的值的东西（*译注：实际上就是常量*），例如我们在代码中使用的字符串'foo'，或者是整型42
- 运行时缓存槽（cache slot）：这个地方用于缓存引擎执行过程中还用会用到的东西

参考图片

![](http://gywbd.github.io/images/oparray-compiled.png)

从这些图片中你可以看到这个OPArray现在包含所有需要传给executor的东西。记住一个原则：**在编译期（生成OPArray的时候）进行的计算越多，那么executor所要进行的计算就越少（*译注：这就是运行期*），这样executor就可以专注于它“真正”的工作：执行编译后的PHP代码**。我们可以看到所有的字面量都会编译进了literals数组（你可能会注意到literals数组中的整型1，它是来自ZEND_RETURN这个OPCode，它会返回1）。

其他的zend_op_array字段基本都是空的（值为0，*译注：指针字段的值会为NULL，NULL也是一种0值*），这是因为我们所编译的脚本非常小：里面没有任何函数调用，也没有任何try-catch结构，或者是break-continue语句。这就是编译一段PHP脚本后得到的结果，而不是编译一个PHP函数的结果。在其他情况下会生成不同的OPArray，有些情况下生成的OPArray中的很多字段都会被填充。



1.type

**op_array的类型**，首先需要说明的是，一段PHP代码被编译之后，虽然返回的是一个zend_op_array指针，但是实际上生成的zend_op_array结构可能不止一个，通过这个结构中的一些字段,例如function_name ,num_args等你也许会发现这个zend_op_array结构似乎能和函数产生一定的联系，确实如此，**用户自定义的函数，以及用户定义的类的方法，都是一个zend_op_array结构，这些zend_op_array结构在编译过程中被保存在某些地方**，例如用户自定义的函数被保存进了GLOBAL_FUNCTION_TABLE,这个是全局函数符号表，通过函数名可以在此表中检索到函数体。那么编译后返回的那个zend_op_array指针是什么呢，其实编译后返回的zend_op_array是执行的一个入口,也可以认为它是最外层，即不在任何函数体内的全局代码组成的op_array。**然而全局代码，用户自定义函数，用户自定义的方法都拥有相同的type值**(2)

type可取值的宏定义为:

```C
#define ZEND_INTERNAL_FUNCTION              1  
#define ZEND_USER_FUNCTION                  2  
#define ZEND_OVERLOADED_FUNCTION            3  
#define ZEND_EVAL_CODE                      4  
#define ZEND_OVERLOADED_FUNCTION_TEMPORARY  5 
```

可以看到全局代码，用户函数，用户方法都对应的是ZEND_USER_FUNCTION,这个也是最常见的type了，其中ZEND_EVAL_CODE对应的是eval函数中的PHP代码，所以我们可以想到，eval函数参数中的PHP代码也会被编译成单独的zend_op_array。



2.function_name
如果op_array是由用户定义的函数或则方法编译而生成，**那么此字段对应函数的名字**，如果是全局代码或则是eval部分的代码，那么此字段为控制。

#### 下面看看执行过程：

```c
ZEND_API void execute(zend_op_array *op_array TSRMLS_DC)  
{  
    zend_execute_data *execute_data;  
    zend_bool nested = 0;  
    zend_bool original_in_execution = EG(in_execution);  
   
   
    if (EG(exception)) {  
        return;  
    }  
   
    EG(in_execution) = 1;  
   
zend_vm_enter:  
    /* Initialize execute_data */ 
    execute_data = (zend_execute_data *)zend_vm_stack_alloc(  
        ZEND_MM_ALIGNED_SIZE(sizeof(zend_execute_data)) +  
        ZEND_MM_ALIGNED_SIZE(sizeof(zval**) * op_array->last_var * (EG(active_symbol_table) ? 1 : 2)) +  
        ZEND_MM_ALIGNED_SIZE(sizeof(temp_variable)) * op_array->T TSRMLS_CC);  
   
    EX(CVs) = (zval***)((char*)execute_data + ZEND_MM_ALIGNED_SIZE(sizeof(zend_execute_data)));  
    memset(EX(CVs), 0, sizeof(zval**) * op_array->last_var);  
    EX(Ts) = (temp_variable *)(((char*)EX(CVs)) + ZEND_MM_ALIGNED_SIZE(sizeof(zval**) * op_array->last_var * (EG(active_symbol_table) ? 1 : 2)));  
    EX(fbc) = NULL;  
    EX(called_scope) = NULL;  
    EX(object) = NULL;  
    EX(old_error_reporting) = NULL;  
    EX(op_array) = op_array;  
    EX(symbol_table) = EG(active_symbol_table);  
    EX(prev_execute_data) = EG(current_execute_data);  
    EG(current_execute_data) = execute_data;  
    EX(nested) = nested;  
    nested = 1;  
   
    if (op_array->start_op) {  
        ZEND_VM_SET_OPCODE(op_array->start_op);  
    } else {  
        ZEND_VM_SET_OPCODE(op_array->opcodes);  
    }  
   
    if (op_array->this_var != -1 && EG(This)) {  
        Z_ADDREF_P(EG(This)); /* For $this pointer */ 
        if (!EG(active_symbol_table)) {  
            EX(CVs)[op_array->this_var] = (zval**)EX(CVs) + (op_array->last_var + op_array->this_var);  
            *EX(CVs)[op_array->this_var] = EG(This);  
        } else {  
            if (zend_hash_add(EG(active_symbol_table), "this", sizeof("this"), &EG(This), sizeof(zval *), (void**)&EX(CVs)[op_array->this_var])==FAILURE) {  
                Z_DELREF_P(EG(This));  
            }  
        }  
    }  
   
    EG(opline_ptr) = &EX(opline);  
   
    EX(function_state).function = (zend_function *) op_array;  
    EX(function_state).arguments = NULL;  
       
    while (1) {  
        int ret;  
#ifdef ZEND_WIN32  
        if (EG(timed_out)) {  
            zend_timeout(0);  
        }  
#endif  
   
        if ((ret = EX(opline)->handler(execute_data TSRMLS_CC)) > 0) {  
            switch (ret) {  
                case 1:  
                    EG(in_execution) = original_in_execution;  
                    return;  
                case 2:  
                    op_array = EG(active_op_array);  
                    goto zend_vm_enter;  
                case 3:  
                    execute_data = EG(current_execute_data);  
                default:  
                    break;  
            }  
        }  
   
    }  
    zend_error_noreturn(E_ERROR, "Arrived at end of main loop which shouldn't happen");  
}
```



zend_vm_enter 这**个跳转标签是作为虚拟机执行的入口**，当op中涉及到函数调用的时候，就有可能会跳转到这里来执行函数体。



为execute_data分配空间

主要是对execute_data进行一些初始化，以及保存现场工作，要保存现场是因为在进入函数调用的时候，需要保存当前一些运行期间的数据，在函数调用结束之后再进行还原，可以想象为操作系统中进程调度，当进程在调出的时候需要保存寄存器等上下文环境，而当进程被调入的时候再取出来继续执行。

在当前动态符号表中加入$this变量，这个是在调用对象的方法时才有必要进行。



#### 执行环境的切换

**用户自定义函数，类方法，eval的代码都会编译成单独的op_array**,那么当进行函数调用等操作时，必然涉及到调用前的op_array执行环境和新的函数的op_array执行环境的切换，这一段我们将以调用用户自定义函数来介绍整个切换过程如何进行。

1.执行期的全局变量就是**executor_globals,其类型为zend_executor_globals, zend_executor_globals的定义在{PHPSRC}/Zend/zend_globals.h，结构比较庞大，这里包含了整个执行期需要用到的各种变量，无论是哪个op_array在执行，都共用这一个全局变量，在执行过程中，此结构中的一些成员可能会改变，**比如当前执行的op_array字段active_op_array，动态符号表字段active_symbol_table可能会根据不同的op_array而改变，This指针会根据在不同的对象环境而改变。
另外还定义了一个EG宏来取此变量中的字段值，此宏是针对线程安全和非线程安全模式的一个封装。



2.针对每一个op_array,都会有自己执行期的一些数据，在函数execute开始的时候我们能看到zend_vm_enter跳转标签下面就会初始一个局部变量execute_data，所以我们每次切换到新的op_array的时候，都会为新的op_array建立一个execute_data变量，此变量的类型为**zend_execute_data的**指针，相关定义在{PHPSRC}/Zend/zend_compile.h：

```c
struct _zend_execute_data {  
    struct _zend_op *opline;  
    zend_function_state function_state;  
    zend_function *fbc; /* Function Being Called */ 
    zend_class_entry *called_scope;  
    zend_op_array *op_array;  
    zval *object;  
    union _temp_variable *Ts;  
    zval ***CVs;  
    HashTable *symbol_table;  
    struct _zend_execute_data *prev_execute_data;  
    zval *old_error_reporting;  
    zend_bool nested;  
    zval **original_return_value;  
    zend_class_entry *current_scope;  
    zend_class_entry *current_called_scope;  
    zval *current_this;  
    zval *current_object;  
    struct _zend_op *call_opline;  
};  
```



opline: 当前正在执行的op。
prev_execute_data: op_array环境切换的时候，这个字段用来保存切换前的op_array,此字段非常重要，**他能将每个op_array的execute_data按照调用的先后顺序连接成一个<u>单链表</u>**，每当一个op_array执行结束要还原到调用前op_array的时候，**就通过当前的execute_data中的prev_execute_data字段来得到调用前的执行器数据。**
在executor_globals中的字段current_execute_data就是指向当前正在执行的op_array的execute_data。



```php
<?php  
$a = 123;  
   
test();  
   
function test()  
{  
    return 1;  
}  
?>  
```

此代码编译之后会生成两个op_array,一个是全局代码的op_array,另外一个是test函数的op_array,其中全局代码中会通过函数调用进入到test函数的执行环境，执行结束之后，会返回到全局代码，然后代码结束。



比如test()函数，会先在全局函数符号表中根据test来搜索相关的函数体，如果搜索不到则会报错函数没有定义，找到test的函数体之后，取得test函数的op_array，然后跳转到execute中的goto标签:zend_vm_enter,于是就进入到了test函数的执行环境。



下面我们分几个阶段来介绍这段代码的过程，然后从中可以知道执行环境切换的方法。

1. 进入execute函数，开始执行op_array ,这个op_array就是全局代码的op_array，我们暂时称其为op_array1

首先在**<u>execute中为op_array1建立了一个execute_data数据,我们暂时命名为execute_data1,然后进行相关的初始化操作</u>**，其中比较重要的是：

2. 在op_array1执行到test函数调用的的时候，**首先从全局函数符号表中找到test的函数体，将函数体保存在execute_data1的function_state字段**，然后从函数体中取到test的op_array,我们这里用op_array2来表示，**并将op_array2赋值给EG(active_op_array)**：

于是执行期全局变量的动态op_array字段指向了函数test的op_array，然后用调用ZEND_VM_ENTER();这个时候会先回到execute函数中的switch结构，并且满足以下case

EG(active_op_array)之前已经被我们设置为test函数的op_array2，于是在函数execute中，op_array变量就指向了test的op_array2,然后跳转到zend_vm_enter。

3. 跳转到zend_vm_enter之后其实又回到了类似1中的步骤，此时为test的op_array2建立了它的执行数据execute_data,我们这里用execute_data2来表示。**跟1中有些不同的是EX(prev_execute_data) = EG(current_execute_data);这个时候current_execute_data = execute_data1**,也就是全局代码的执行执行期数据，然后**EG(current_execute_data) = execute_data;这样current_execute_data就等于test的执行期数据execute_data2了**，同时全局代码的execute_data1被保存在execute_data2的prev_execute_data字段。这个时候进行环境的切换已经完成，于是开始执行test函数。


4. test函数执行完之后就要返回到调用前的执行环境了，也就是全局代码执行环境，此阶段最重要的一个操作就是EG(current_execute_data) = EX(prev_execute_data); 在3中EX(prev_execute_data)已经设置成了全局代码的execute_data1,所以这样当前执行数据就变成了全局代码的执行数据，这样就成功的从函数test执行环境返回到了全局代码执行环境

这样，执行环境的切换过程就完成了，对于深层次的函数调用，原理一样，执行数据execute_data组成的单链表会更长。