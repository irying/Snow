//
// Created by kyrie on 2017/6/5.
//
//使用双向链表维护LRU特性
// 我们将在缓存中维护一个双向链表，该链表将缓存中的数据块按访问时间从新到旧排列起来：
// 当我们需要访问一块数据时，我们先调用接口LRUCacheGet尝试从缓存中获取数据
        //1.如果缓存中刚好缓存了该数据，那么我们先将该数据从缓存的双向链表中摘除，然后再重新放入到双向链表的表头
        //2.如果缓存中没有我们需要的数据，那么我们从外部取得数据，然后调用LRUCacheSet接口将该数据放入缓存中，此过程，会将新数据插入到双向链表的表头
        //3.使用哈希表保证缓存中数据的访问速度
// 如果我们仅将缓存中的数据维护在一个链表中，那么当我们需要从缓存中查找数据时，就意味着我们需要遍历链表。
// 这样的设计，其时间复杂度为O(n)，是一种比较低效率的做法。
// 因此，我们除了将数据维护在一个双向链表中，我们同时还将数据维护在一个哈希表当中。哈希表访问数据的时间复杂度为O(1)。


#ifndef KYRIE_C_LRUCACHEIMPL_H
#define KYRIE_C_LRUCACHEIMPL_H

typedef struct cacheEntry{
    char key;
    char data;
    struct cacheEntry *hashListPrev;
    struct cacheEntry *hashListNext;
    struct cacheEntry *lruListPrev;
    struct cacheEntry *lruListNext;
}cacheEntry;


typedef struct LRUCache{
    int capacity;
    cacheEntry **hashMap;
    cacheEntry *lruListHead;
    cacheEntry *lruListTail;
    int lruListSize;
}LRUCache;
#endif //KYRIE_C_LRUCACHEIMPL_H
