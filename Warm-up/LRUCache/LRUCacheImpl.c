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

static cacheEntry* insert(LRUCache *cache, cacheEntry *entry)
{
    cacheEntry *removeEntry = NULL;

    if (++cache->lruListSize > cache->ca)
}




