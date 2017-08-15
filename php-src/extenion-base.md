```c
int zend_parse_parameters(int num_args TSRMLS_DC, char *type_spec, ...);
```

zend_parse_parameters()接口用于获取从脚本调用函数时传递过来的参数。

下面来说说type_spec这个参数，type_spec参数是一个字符串，其中每个字符代表要获取参数的类型，例如："ls"表示要获取一个整数类型和一个字符串类型的参数。下面列出常用的参数类型所使用的字符：

- *l* - long
- *d* - double
- *s* - string (with possible null bytes) and its length
- *b* - boolean
- *r* - resource, stored in *zval\**
- *a* - array, stored in *zval\**
- *o* - object (of any class), stored in *zval\**
- *O* - object (of class specified by class entry), stored in *zval\**
- *z* - the actual *zval\**



#### 数组赋值API

要把变量设置为数组类型，可以使用array_init(zval *arr)接口。例如下面代码就可以生成一个数组类型的变量

**add_assoc_long(zval \*array, char *key, long n);()**

**add_assoc_bool(zval \*array, char *key, int b);()**

...



#### 对象赋值API

```c
zval *obj;
MAKE_STD_ZVAL(obj);
object_init(obj);
add_property_string(obj, "name", "666", 1);
```



#### PHP_INI

PHP_INI_BEGIN()是一个配置块的开始，PHP_INI_END()是配置块的结束。

```C
PHP_INI_BEGIN();
PHP_INI_ENTRY("setting", "value", PHP_INI_ALL, onchange);
PHP_INI_END();
```

onchange是个回调函数

定义一个回调函数可以用下面方法

Zend引擎会提供配置改变后的值（new_value）和值的字符串长度（new_value_length）作为参数。

```c
PHP_INI_MH(callback) {
    php_printf("Setting value is %s", new_value);
}
```



#### 资源类型

因为PHP的变量不能保存C语言中的复杂结构类型，如：FILE结构。

**1.要使用一个资源类型，首先需要注册资源类型的析构函数。**

```C
ZEND_API int zend_register_list_destructors_ex(rsrc_dtor_func_t ld, rsrc_dtor_func_t pld, char *type_name, int module_number);
```

参数说明：
ld：普通资源的析构函数。
pld：持久化资源的析构函数。
type_name：资源的名字。
module_number：模块编号（由Zend引擎提供，可以忽略）

**资源的析构函数是当资源引用为零时，被回调来释放资源。**

```C
typedef struct _zend_rsrc_list_entry {
     
    void *ptr;
    int type;
    int refcount;

} zend_rsrc_list_entry;

void list_destructor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
    list_t *ls = (list_t *)rsrc->ptr;
    list_free(ls);
}
```

```c
PHP_MINIT_FUNCTION(queue)
{
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/
    /* Register destructor function */
    le_queue = zend_register_list_destructors_ex(
        &list_destructor, NULL, RESOURCE_TYPE_NAME, module_number);
	return SUCCESS;
}
```

**2.注册完资源的析构函数后，还需要注册资源实例（创建资源变量）**

```c
int ZEND_REGISTER_RESOURCE(zval *rsrc_result, void *rsrc_pointer, int rsrc_type);
```

参数说明：
rsrc_result：返回的资源变量。
rsrc_pointer：保存的资源实体。
rsrc_type：由zend_register_list_destructors_ex()函数返回的资源类型句柄。

我们可以通过zend_register_resource()这个接口来创建一个资源变量，rsrc_result是用来保存创建后的资源变量（zval结构体），<u>rsrc_pointer是我们要保存的资源，而rsrc_type是zend_register_list_destructors_ex()的返回值。</u>



zend_register_resource()返回一个整形的值，称为：资源实体在全局资源列表中的唯一整数标识符。我们可以通过RETURN_RESOURCE(rsrc_id)宏来把这个标识符返回给用户。然后用户就可以通过这个资源ID来获取真实的资源实体。

```
PHP_FUNCTION(queue_new)
{
    zval *res;
    list_t *ls;
    int rsid;

    ls = list_create(); /* Create list resource */
    if (!ls) {
        RETURN_FALSE;
    }

    rsid = ZEND_REGISTER_RESOURCE(res, ls, le_queue);

    RETURN_RESOURCE(rsid);
}
```

**3.通过资源变量获取资源实体，可以使用以下的接口**

ZEND_FETCH_RESOURCE

```c
PHP_FUNCTION(queue_push)
{
    zval *res, *data;
    list_t *ls;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz", &res, &data)
        == FAILURE)
    {
        RETURN_FALSE;
    }

    ZEND_FETCH_RESOURCE(ls, list_t *, &res, -1, RESOURCE_TYPE_NAME, le_queue);

    if (list_push(ls, (void *)data) == 0) {
        zval_add_ref(&data); /* refcount++ */
        RETURN_TRUE;
    }

    RETURN_FALSE;
}
```

参数说明：
rsrc：用于保存资源实体的指针。
rsrc_type：资源实体的类型（如FILE）。
rsrc_id：资源变量指针（zval **）。
default_rsrc_id：设置为-1即可。
resource_type_name：资源类型名称，用于出错时提示。
resource_type：由zend_register_list_destructors_ex()函数返回的资源类型句柄