前段时间，出现了一次服务器cpu 占用100的问题。以下为追查原因的过程。仅当抛砖引玉，欢迎拍砖。
**查看占用cpu高的进程**
想找出占用cpu高的进程，用top命令就可以搞定。

```
$top
.....此处省略n多行...
10434 root     20   0  509m 174m 1528 R 99.7  0.5   8:42.43 php                                                                                                        
 5638 root     20   0  509m 174m 1528 R 99.1  0.5   9:12.35 php                                                                                                        
16390 root     20   0  541m 182m 5244 R 98.4  0.6   8:40.92 php        

```

此时，轻轻按下C键。就会看到如下信息。

**找出进程占用cpu高的原因**
进程占用cpu高，一般是由于进程长时间占用cpu，又没有主动释放占用。如果想主动释放cpu，可以调用sleep。在写程序的时候，尤其要注意while 等循环的地方。

**找出php进程在执行那段代码**

```C
$sudo gdb -p 10434
(gdb) print (char *)executor_globals.active_op_array->filename
$13 = 0x2924118 "/home/gearman_manager/pecl-manager.php"
(gdb) print executor_globals->current_execute_data->opline->lineno
$14 = 55
(gdb) c
Continuing.
^C
Program received signal SIGINT, Interrupt.
0x00000031d32306d0 in sigprocmask () from /lib64/libc.so.6
(gdb) print executor_globals->current_execute_data->opline->lineno
$15 = 71
(gdb) c
Continuing.
^C
Program received signal SIGINT, Interrupt.
0x00000000006250e1 in zend_hash_find ()
(gdb) print executor_globals->current_execute_data->opline->lineno
$16 = 53
```

如果对上面的命令有疑问，可以查看 [[当cpu飙升时，找出php中可能有问题的代码行](http://www.bo56.com/%E5%BD%93cpu%E9%A3%99%E5%8D%87%E6%97%B6%EF%BC%8C%E6%89%BE%E5%87%BAphp%E4%B8%AD%E5%8F%AF%E8%83%BD%E6%9C%89%E9%97%AE%E9%A2%98%E7%9A%84%E4%BB%A3%E7%A0%81%E8%A1%8C/)]
根据上面的信息，我们可以知道，cpu高时，正在执行/home/gearman_manager/pecl-manager.php文件。并且正在执行53和71行附近的代码。

**分析相关代码**
php的相关代码如下：

```php
while(!$this->stop_work){ //此处是第51行
    if(@$thisWorker->work() ||
        $thisWorker->returnCode() == GEARMAN_IO_WAIT ||
        $thisWorker->returnCode() == GEARMAN_NOT_CONNECTED ||
        $thisWorker->returnCode() == GEARMAN_NO_JOBS) {
         if ($thisWorker->returnCode() == GEARMAN_SUCCESS) continue;
         if (!@$thisWorker->wait()){
             if ($thisWorker->returnCode() == GEARMAN_NO_ACTIVE_FDS){
                 sleep(5);
             }
         }
    }
 
    /**
     * Check the running time of the current child. If it has
     * been too long, stop working.
     */
     //此处是第71行
     if($this->max_run_time > 0 && time() - $start > $this->max_run_time) {
         $this->log("Been running too long, exiting", GearmanManager::LOG_LEVEL_WORKER_INFO);
         $this->stop_work = true;
      }
 
      if(!empty($this->config["max_runs_per_worker"]) && $this->job_execution_count >= $this->config["max_runs_per_worker"]) {
          $this->log("Ran $this->job_execution_count jobs which is over the maximum({$this->config['max_runs_per_worker']}), exiting", GearmanManager::LOG_LEVEL_WORKER_INFO);
           $this->stop_work = true;
       }
}
```

看来作者已经考虑到某些情况可能导致cpu 100，因此在代码中使用了sleep方法。现在php进程占用cpu 100，看来是没执行sleep(5)。如果$thisWorker->work() 和 $thisWorker->returnCode() 方法中没有IO等操作，能释放cpu的占用，又没有调用sleep的情况下，很容易导致进程占用cpu 100。
根据代码我们可以得出如下几个结论：
\- 肯定没执行slepp(5)
\- $thisWorker->work() 和 $thisWorker->returnCode() 方法中没有IO等操作，能释放cpu的占用。

**进一步跟踪代码中可疑部分**
分析 $thisWorker->returnCode() 的返回值。如果很容易复现cpu 100的话，你可以在程序中echo $thisWorker->returnCode() 来获得返回值。但是现在cpu 100的问题，复现比较麻烦。所以，还是使用gdb 来搞吧。

```C
$gdb -p 10434
(gdb)b zif_gearman_worker_return_code 
Breakpoint 1, zif_gearman_worker_return_code (ht=0, return_value=0x2a1b648, return_value_ptr=0x0, this_ptr=0x2a14090, return_value_used=1)
    at /home/php_src/pecl/gearman-1.1.1/php_gearman.c:3186
3186    in /home/php_src/pecl/gearman-1.1.1/php_gearman.c
(gdb) p return_value
$3 = (zval *) 0x2a1b648
(gdb) p *$3
$4 = {value = {lval = 0, dval = 0, str = {val = 0x0, len = 0}, ht = 0x0, obj = {handle = 0, handlers = 0x0}}, refcount__gc = 1, type = 0 '\0', is_ref__gc = 0 '\0'}
(gdb) s
3189    in /home/php_src/pecl/gearman-1.1.1/php_gearman.c
(gdb) p *$3
$5 = {value = {lval = 0, dval = 0, str = {val = 0x0, len = 0}, ht = 0x0, obj = {handle = 0, handlers = 0x0}}, refcount__gc = 1, type = 0 '\0', is_ref__gc = 0 '\0'}
(gdb) s
3186    in /home/php_src/pecl/gearman-1.1.1/php_gearman.c
(gdb) p *$3
$6 = {value = {lval = 0, dval = 0, str = {val = 0x0, len = 0}, ht = 0x0, obj = {handle = 0, handlers = 0x0}}, refcount__gc = 1, type = 0 '\0', is_ref__gc = 0 '\0'}
(gdb) s
3189    in /home/php_src/pecl/gearman-1.1.1/php_gearman.c
(gdb) p *$3
$7 = {value = {lval = 0, dval = 0, str = {val = 0x0, len = 0}, ht = 0x0, obj = {handle = 0, handlers = 0x0}}, refcount__gc = 1, type = 0 '\0', is_ref__gc = 0 '\0'}
(gdb) s
3190    in /home/php_src/pecl/gearman-1.1.1/php_gearman.c
(gdb) p *$3
$8 = {value = {lval = 0, dval = 0, str = {val = 0x0, len = 0}, ht = 0x0, obj = {handle = 0, handlers = 0x0}}, refcount__gc = 1, type = 0 '\0', is_ref__gc = 0 '\0'}
(gdb) s
3191    in /home/php_src/pecl/gearman-1.1.1/php_gearman.c
(gdb) p *$3
$9 = {value = {lval = 25, dval = 1.2351641146031164e-322, str = {val = 0x19 <Address 0x19 out of bounds>, len = 0}, ht = 0x19, obj = {handle = 25, handlers = 0x0}}, 
  refcount__gc = 1, type = 1 '\001', is_ref__gc = 0 '\0'}

```

从最后的lval = 25 和 type =1 我们可以看出 returnCode()方法的最后返回值是 25 。
根据文档，返回值25 对应的是 GEARMAN_NOT_CONNECTED。

## 解决问题

既然出问题时， returnCode()方法的最后返回值是 25，而现在程序中对这种情况又没进行出来，导致了cpu 100。那我们只要在出现这种情况时，sleep几秒就ok了。



### 背景知识

在解释执行过程中，有一个全局变量包含了执行过程中用到的各种数据。它就是executor_globals。在源码的Zend/zend_globals.h 文件中可以找到他的类型定义。

```C
struct _zend_executor_globals {
    zval **return_value_ptr_ptr;
 
    zval uninitialized_zval;
    zval *uninitialized_zval_ptr;
 
    zval error_zval;
    zval *error_zval_ptr;
 
    zend_ptr_stack arg_types_stack;
 
    /* symbol table cache */
    HashTable *symtable_cache[SYMTABLE_CACHE_SIZE];
    HashTable **symtable_cache_limit;
    HashTable **symtable_cache_ptr;
 
    zend_op **opline_ptr;
 
    HashTable *active_symbol_table;
    HashTable symbol_table;     /* main symbol table */
 
    HashTable included_files;   /* files already included */
 
    JMP_BUF *bailout;
 
    int error_reporting;
    int orig_error_reporting;
    int exit_status;
 
    zend_op_array *active_op_array;
 
    HashTable *function_table;  /* function symbol table */
    HashTable *class_table;     /* class table */
    HashTable *zend_constants;  /* constants table */
 
    zend_class_entry *scope;
    zend_class_entry *called_scope; /* Scope of the calling class */
 
    zval *This;
 
    long precision;
 
    int ticks_count;
 
    zend_bool in_execution;
    HashTable *in_autoload;
    zend_function *autoload_func;
    zend_bool full_tables_cleanup;
 
    /* for extended information support */
    zend_bool no_extensions;
 
#ifdef ZEND_WIN32
    zend_bool timed_out;
    OSVERSIONINFOEX windows_version_info;
#endif
 
    HashTable regular_list;
    HashTable persistent_list;
 
    zend_vm_stack argument_stack;
 
    int user_error_handler_error_reporting;
    zval *user_error_handler;
    zval *user_exception_handler;
    zend_stack user_error_handlers_error_reporting;
    zend_ptr_stack user_error_handlers;
    zend_ptr_stack user_exception_handlers;
 
    zend_error_handling_t  error_handling;
    zend_class_entry      *exception_class;
 
    /* timeout support */
    int timeout_seconds;
 
    int lambda_count;
 
    HashTable *ini_directives;
    HashTable *modified_ini_directives;
 
    zend_objects_store objects_store;
    zval *exception, *prev_exception;
    zend_op *opline_before_exception;
    zend_op exception_op[3];
 
    struct _zend_execute_data *current_execute_data;
 
    struct _zend_module_entry *current_module;
 
    zend_property_info std_property_info;
 
    zend_bool active; 
 
    void *saved_fpu_cw;
 
    void *reserved[ZEND_MAX_RESERVED_RESOURCES];
};
```

这里我们只说两个对我们比较重要的变量，active_op_array 和 current_execute_data。
**active_op_array**变量中保存了引擎正在执行的op_array（[想了解什么是op_array请点击查看](http://www.bo56.com/php%E5%86%85%E6%A0%B8%E6%8E%A2%E7%B4%A2%E4%B9%8Bzend_execute%E7%9A%84%E5%85%B7%E4%BD%93%E6%89%A7%E8%A1%8C%E8%BF%87%E7%A8%8B/#op_array)）。在Zend/zend_compile.h中有关于op_array的数据类型的定义。



```
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
```

定义中，filename和 function_name分别保存了正在执行的文件名和方法名。
**current_execute_data**保存了正在执行的op_array的execute_data。

#### execute_data保存了每个op_array执行过程中的一些数据。其定义在，Zend/zend_compile.h：

```
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

#### 定义中的opline就是正在执行的opcode。opcode的结构定义如下：

```
struct _zend_op {
    opcode_handler_t handler;
    znode result;
    znode op1;
    znode op2;
    ulong extended_value;
    uint lineno;
    zend_uchar opcode;
};
```

### 你可以使用.gdbinit文件。这个文件在php源码的根目录下。使用方法如下：

```
$sudo gdb -p 14973
(gdb) source /home/xinhailong/.gdbinit 
(gdb) zbacktrace
[0xa453f34] sleep(1) /home/xinhailong/test/php/test.php:4 
[0xa453ed0] test1() /home/xinhailong/test/php/test.php:7 
(gdb) 
```