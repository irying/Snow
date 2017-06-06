#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LRUCache.h"
#include "LRUCacheImpl.h"

static cacheEntry* newCacheEntry(char key, char data)
{
    cacheEntry* entry = NULL;
    if (NULL == (entry = malloc(sizeof(*entry)))) {
        perror("malloc");
        return NULL;
    }
    memset(entry, 0, sizeof(*entry));
    entry->key = key;
    entry->data = data;
    return entry;
}


int LRUCacheCreate(int capacity, void **lruCache)
{
    // 1.初始化缓存结构 2.初始化缓存结构里面的hashMap
    LRUCache* cache = NULL;
    if (NULL == (cache=malloc(sizeof(*cache)))) {
        perror("malloc");
        return -1;
    }
    memset(cache, 0, sizeof(*cache));
    cache->capacity = capacity;
    cache->hashMap = malloc(sizeof(cacheEntry)*capacity);
    if (NULL == cache->hashMap) {
        free(cache);
        perror("malloc");
        return -1;
    }
    *lruCache = cache;
    return 0;
}

static void freeCacheEntry(cacheEntry* entry)
{
    if (NULL == entry) return;
    free(entry);
}

static void removeFromList(LRUCache *cache, cacheEntry *entry)
{
    if (cache->lruListSize <= 0)
        return;

    // 4种情况，这个列表只剩一个元素
    // 这个元素是头
    // 这个元素是尾
    // 这个元素在中间

    if (entry == cache->lruListHead && entry == cache->lruListTail) {
        cache->lruListHead = cache->lruListTail = NULL;
    } else if (entry == cache->lruListHead) {
        cache->lruListHead = entry->lruListNext;
        cache->lruListHead->lruListPrev = NULL;
    } else if(entry == cache->lruListTail) {
        cache->lruListTail = entry->lruListPrev;
        cache->lruListTail->lruListNext = NULL;
    } else {
        entry->lruListPrev->lruListNext = entry->lruListNext;
        entry->lruListNext->lruListPrev = entry->lruListPrev;
    }

    cache->lruListSize--;
}

static cacheEntry* insertToListHead(LRUCache *cache, cacheEntry *entry)
{
    cacheEntry *removeEntry = NULL;

    // 缓存满了，需要删除链表表尾节点，淘汰最久没有被访问的缓存单元
    // 链表为空
    // 链表非空
    if (++cache->lruListSize > cache->capacity) {
        removeEntry = cache->lruListTail;
        removeFromList(cache, removeEntry);
    }

    if (cache->lruListHead == NULL && cache->lruListTail == NULL) {
        cache->lruListHead = cache->lruListTail = entry;
    } else {
        entry->lruListPrev = NULL;
        entry->lruListNext = cache->lruListHead;
        cache->lruListHead->lruListPrev = entry;
        cache->lruListHead = entry;
    }

    return removeEntry;
}

static void freeList(LRUCache *cache)
{
    // 链表为空
    // 链表不为空
    if (0 == cache->lruListSize) return;

    cacheEntry *entry = cache->lruListHead;

    while(entry) {
        cacheEntry *temp = entry->lruListNext;
        freeCacheEntry(entry);
        entry = temp;
    }
    cache->lruListSize = 0;
}

static void updateLRUList(LRUCache *cache, cacheEntry *entry)
{
    removeFromList(cache, entry);
    insertToListHead(cache, entry);
}








