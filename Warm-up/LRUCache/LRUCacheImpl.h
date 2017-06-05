//
// Created by kyrie on 2017/6/5.
//

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
