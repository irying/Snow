#### A.PHP不需要显式的对内存进行管理，这些工作都由Zend引擎进行管理了。

1**<u>.引用计数，函数表，符号表，常量表等</u>**。当我们明白这些信息都会占用内存的时候， 我们可以有意的避免不必要的浪费内存，比如我们在项目中通常会使用autoload来避免一次性把不一定会使用的类包含进来，而这些信息是会占用内存的，如果我们及时把不再使用的变量unset掉之后*可能*会释放掉它所占用的空间，

> 前面之所以会说把变量unset掉时候*可能*会把它释放掉的原因是： 在PHP中为了避免不必要的内存复制，采用了引用计数和写时复制的技术， **<u>所以这里unset只是将引用关系打破，如果还有其他变量指向该内存， 它所占用的内存还是不会被释放的。</u>** 
> 当然这还有一种情况：出现循环引用，这个就得靠gc来处理了， 内存不会当时就释放，只有在gc环节才会被释放。



2.所以普通应用程序是无法直接对内存进行访问的， 应用程序只能向操作系统申请内存，通常的应用也是这么做的，在需要的时候通过类似malloc之类的库函数向操作系统申请内存，**在一些对性能要求较高的应用场景下是需要频繁的使用和释放内存的， 比如Web服务器，编程语言等，由于向操作系统申请内存空间会引发[系统调用](http://zh.wikipedia.org/wiki/%E7%B3%BB%E7%BB%9F%E8%B0%83%E7%94%A8)**， 系统调用和普通的应用层函数调用性能差别非常大，<u>**因为系统调用会将CPU从用户态切换到内核， 因为涉及到物理内存的操作，只有操作系统才能进行，而这种切换的成本是非常大的， 如果频繁的在内核态和用户态之间切换会产生性能问题。**</u>

鉴于系统调用的开销，**一些对性能有要求的应用通常会自己在用户态进行内存管理**， 例如第一次申请稍大的内存留着备用，而使用完释放的内存并不是马上归还给操作系统， 可以将内存进行复用，这样可以避免多次的内存申请和释放所带来的性能消耗。



### B.接口层的宏

```C
/* Standard wrapper macros */
#define emalloc(size)                       _emalloc((size) ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC)
#define safe_emalloc(nmemb, size, offset)   _safe_emalloc((nmemb), (size), (offset) ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC)
#define efree(ptr)                          _efree((ptr) ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC)
#define ecalloc(nmemb, size)                _ecalloc((nmemb), (size) ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC)
#define erealloc(ptr, size)                 _erealloc((ptr), (size), 0 ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC)
#define safe_erealloc(ptr, nmemb, size, offset) _safe_erealloc((ptr), (nmemb), (size), (offset) ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC)
#define erealloc_recoverable(ptr, size)     _erealloc((ptr), (size), 1 ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC)
#define estrdup(s)                          _estrdup((s) ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC)
#define estrndup(s, length)                 _estrndup((s), (length) ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC)
#define zend_mem_block_size(ptr)            _zend_mem_block_size((ptr) TSRMLS_CC ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC)
```

PHP的内存管理可以被看作是分层（hierarchical）的。 它分为三层：**存储层（storage）、堆层（heap）和接口层（emalloc/efree）**。 存储层通过 malloc()、mmap() 等函数向系统真正的申请内存，并通过 free() 函数释放所申请的内存。

### C.存储层

#### 按A所示，本质就是一次申请大块内存，创建自己的内存管理结构，自己管理， 减少系统调用。

```C
/* Heaps with user defined storage */
typedef struct _zend_mm_storage zend_mm_storage;
 
typedef struct _zend_mm_segment {
    size_t    size;
    struct _zend_mm_segment *next_segment;
} zend_mm_segment;
 
typedef struct _zend_mm_mem_handlers {
    const char *name;
    zend_mm_storage* (*init)(void *params);    //    初始化函数
    void (*dtor)(zend_mm_storage *storage);    //    析构函数
    void (*compact)(zend_mm_storage *storage);
    zend_mm_segment* (*_alloc)(zend_mm_storage *storage, size_t size);    //    内存分配函数
    zend_mm_segment* (*_realloc)(zend_mm_storage *storage, zend_mm_segment *ptr, size_t size);    //    重新分配内存函数
    void (*_free)(zend_mm_storage *storage, zend_mm_segment *ptr);    //    释放内存函数
} zend_mm_mem_handlers;
 
struct _zend_mm_storage {
    const zend_mm_mem_handlers *handlers;    //    处理函数集
    void *data;
};
```

```c
/* 使用mmap内存映射函数分配内存 写入时拷贝的私有映射，并且匿名映射，映射区不与任何文件关联。*/
# define ZEND_MM_MEM_MMAP_ANON_DSC {"mmap_anon", zend_mm_mem_dummy_init, zend_mm_mem_dummy_dtor, zend_mm_mem_dummy_compact, zend_mm_mem_mmap_anon_alloc, zend_mm_mem_mmap_realloc, zend_mm_mem_mmap_free}
 
/* 使用mmap内存映射函数分配内存 写入时拷贝的私有映射，并且映射到/dev/zero。*/
# define ZEND_MM_MEM_MMAP_ZERO_DSC {"mmap_zero", zend_mm_mem_mmap_zero_init, zend_mm_mem_mmap_zero_dtor, zend_mm_mem_dummy_compact, zend_mm_mem_mmap_zero_alloc, zend_mm_mem_mmap_realloc, zend_mm_mem_mmap_free}
 
/* 使用HeapAlloc分配内存 windows版本 关于这点，注释中写的是VirtualAlloc() to allocate memory，实际在程序中使用的是HeapAlloc*/
# define ZEND_MM_MEM_WIN32_DSC {"win32", zend_mm_mem_win32_init, zend_mm_mem_win32_dtor, zend_mm_mem_win32_compact, zend_mm_mem_win32_alloc, zend_mm_mem_win32_realloc, zend_mm_mem_win32_free}
 
/* 使用malloc分配内存 默认为此种分配 如果有加ZEND_WIN32宏，则使用win32的分配方案*/
# define ZEND_MM_MEM_MALLOC_DSC {"malloc", zend_mm_mem_dummy_init, zend_mm_mem_dummy_dtor, zend_mm_mem_dummy_compact, zend_mm_mem_malloc_alloc, zend_mm_mem_malloc_realloc, zend_mm_mem_malloc_free}
 
static const zend_mm_mem_handlers mem_handlers[] = {
#ifdef HAVE_MEM_WIN32
    ZEND_MM_MEM_WIN32_DSC,
#endif
#ifdef HAVE_MEM_MALLOC
    ZEND_MM_MEM_MALLOC_DSC,
#endif
#ifdef HAVE_MEM_MMAP_ANON
    ZEND_MM_MEM_MMAP_ANON_DSC,
#endif
#ifdef HAVE_MEM_MMAP_ZERO
    ZEND_MM_MEM_MMAP_ZERO_DSC,
#endif
    {NULL, NULL, NULL, NULL, NULL, NULL}
};
```

##### 上面决定了内存分配方案和段大小，那就初始化吧，第1121~1138行 遍历整个mem_handlers数组，确认内存分配方案，如果没有设置ZEND_MM_MEM_TYPE变量，默认使用malloc方案，如果是windows(即ZEND_WIN32)，则默认使用win32方案，如果设置了ZEND_MM_MEM_TYPE变量，则采用设置的方案。

第1140~1152行 确认段分配大小，如果设置了ZEND_MM_SEG_SIZE变量，则使用设置的大小，此处会判断所设置的大小是否满足2的倍数，并且大于或等于ZEND_MM_ALIGNED_SEGMENT_SIZE + ZEND_MM_ALIGNED_HEADER_SIZE；如果没有设置没使用默认的ZEND_MM_SEG_SIZE

```c
ZEND_API zend_mm_heap *zend_mm_startup(void)
{
    int i;
    size_t seg_size;
    char *mem_type = getenv("ZEND_MM_MEM_TYPE");
    char *tmp;
    const zend_mm_mem_handlers *handlers;
    zend_mm_heap *heap;
 
    if (mem_type == NULL) {
   	 i = 0;
    } else {
   	 for (i = 0; mem_handlers[i].name; i++) {
   		 if (strcmp(mem_handlers[i].name, mem_type) == 0) {
   			 break;
   		 }
   	 }
   	 if (!mem_handlers[i].name) {
   		 fprintf(stderr, "Wrong or unsupported zend_mm storage type '%s'\n", mem_type);
   		 fprintf(stderr, "  supported types:\n");
   		 for (i = 0; mem_handlers[i].name; i++) {
   			 fprintf(stderr, "	'%s'\n", mem_handlers[i].name);
   		 }
   		 exit(255);
   	 }
    }
    handlers = &mem_handlers[i];
 
    tmp = getenv("ZEND_MM_SEG_SIZE");
    if (tmp) {
   	 seg_size = zend_atoi(tmp, 0);
   	 if (zend_mm_low_bit(seg_size) != zend_mm_high_bit(seg_size)) {
   		 fprintf(stderr, "ZEND_MM_SEG_SIZE must be a power of two\n");
   		 exit(255);
   	 } else if (seg_size < ZEND_MM_ALIGNED_SEGMENT_SIZE + ZEND_MM_ALIGNED_HEADER_SIZE) {
   		 fprintf(stderr, "ZEND_MM_SEG_SIZE is too small\n");
   		 exit(255);
   	 }
    } else {
   	 seg_size = ZEND_MM_SEG_SIZE;
    }
 
    //....代码省略
}
```