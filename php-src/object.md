在PHP代码中，对象的创建是通过关键字 **new** 进行的。 一个new操作最终会产生三个opcode，这三个opcode实际为创建对象的三个步骤：

1. ZEND_FETCH_CLASS 根据类名获取存储类的变量，其实现为一个hashtalbe EG(class_table) 的查找操作；
2. NEW 初始化对象，并将EX(call)->fbc指向构造函数指针，初始化的操作我们在后面详细说明，其最后执行的函数为 ZEND_NEW_SPEC_HANDLER
3. DO_FCALL_BY_NAME 调用构造函数，其调用和其它的函数调用是一样，都是调用zend_do_fcall_common_helper_SPEC

第一步和第三步比较简单，我们详细介绍一下第二步：初始化对象。初始化对象调用ZEND_NEW_SPEC_HANDLER， 它首先会判断对象所对应的类是否为可实例化的类， 即判断类的**ce_flags是否与ZEND_ACC_INTERFACE、ZEND_ACC_IMPLICIT_ABSTRACT_CLASS或ZEND_ACC_EXPLICIT_ABSTRACT_CLASS有交集**， 即判断类是否为接口或抽象类。



在类的类型判断完成后，**如果一切正常，程序会给需要创建的对象存放的ZVAL容器分配内存。 然后调用object_init_ex方法初始化类，其调用顺序为： [object_init_ex()] --> [_object_init_ex()] --> [_object_and_properties_init()]**

在_object_and_properties_init函数中，程序会执行前面提到的类的类型的判断，然后更新类的静态变量等信息（在这前面的章节有说明）， 更新完成后，程序会设置zval的类型为IS_OBJECT。



在设置了类型之后，程序会执行zend_object类型的对象的初始化工作，此时调用的函数是zend_objects_new。



```c
ZEND_API zend_object_value zend_objects_new(zend_object **object, zend_class_entry *class_type TSRMLS_DC)
{
    zend_object_value retval;
 
    *object = emalloc(sizeof(zend_object));
    (*object)->ce = class_type;
    retval.handle = zend_objects_store_put(*object, (zend_objects_store_dtor_t) zend_objects_destroy_object, (zend_objects_free_object_storage_t) zend_objects_free_object_storage, NULL TSRMLS_CC);
    retval.handlers = &std_object_handlers;
    (*object)->guards = NULL;
    return retval;
}
```

zend_objects_new函数会初始化对象自身的相关信息，包括对象归属于的类，对象实体的存储索引，对象的相关处理函数。 在这里将对象放入对象池中的函数为zend_objects_store_put。

**<u>在将对象放入对象池，返回对象的存放索引后，程序设置对象的处理函数为标准对象处理函数：std_object_handlers。</u>** 其位于Zend/zend_object_handles.c文件中。