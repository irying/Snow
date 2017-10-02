#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LRUCache.h"
#include "LRUCacheImpl.h"

/*创建一个缓存单元*/
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


/*创建一个LRU缓存*/
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

/*释放一个缓存单元*/
static void freeCacheEntry(cacheEntry* entry)
{
    if (NULL == entry) return;
    free(entry);
}


/*从双向链表中删除指定节点*/
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

/* 将节点插入到链表表头*/
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

/*释放整个链表*/
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

/*哈希函数*/
static int hashKey(LRUCache *cache, char key)
{
    return (int)key%cache->capacity;
}


/*向哈希表插入缓存单元*/
static void insertEntryToHaspMap(LRUCache *cache, cacheEntry *entry)
{
    cacheEntry *slot = cache->hashMap[hashKey(cache, entry->key)];
    /*如果槽内已有其他数据项，将槽内数据项与当前欲加入数据项链成链表，当前欲加入数据项为表头*/
    if (slot != NULL) {
        entry->hashListNext = slot;
        slot->hashListPrev = entry;
    }
    /*将数据项放入加入哈希槽内*/
    cache->hashMap[hashKey(cache,entry->key)] = entry;
}

/*从哈希表获取缓存单元*/
static cacheEntry *getValueFromHashMap(LRUCache *cache, int key)
{
    cacheEntry *entry = cache->hashMap[hashKey(cache, key)];

    while (entry) {
        if (entry->key == key)
            break;
        entry = entry->hashListNext;
    }

    return entry;
}

/*从哈希表删除缓存单元*/
static void removeEntryFromHashMap(LRUCache *cache, cacheEntry *entry)
{
    if (NULL == entry || NULL == cache || NULL == cache->hashMap) return;
    cacheEntry *slot = cache->hashMap[hashKey(cache,entry->key)];
    while (slot) {
        if (slot->key == entry->key) {
            if (slot->hashListPrev) {
                slot->hashListPrev->hashListNext = slot->hashListNext;
            } else {
                cache->hashMap[hashKey(cache, entry->key)] = slot->hashListNext;
            }
            if (slot->hashListNext)
                slot->hashListNext->hashListPrev = slot->hashListPrev;
            return;
        }

        slot = slot->hashListNext;
    }
}


/* 下面是缓存存取接口 */
int LRUCacheSet(void *lruCache, char key, char data)
{
    LRUCache *cache = (LRUCachey *)lruCache;
    cacheEntry *entry = getValueFromHashMap(cache, key);
    if (entry != NULL) {
        entry->data = data;
        updateLRUList(cache, entry);
    } else {
        entry = newCacheEntry(key, data);
        cacheEntry *removeEntry = insertToListHead(cache, entry);
        if (NULL != removeEntry) {
            removeEntryFromHashMap(cache, removeEntry);
            freeCacheEntry(removeEntry);
        }
        insertEntryToHaspMap(cache, entry);
    }

    return  0;

}


char LRUCacheGet(void *lruCache, char key)
{
    LRUCache *cache = (LRUCache *)lruCache;
    cacheEntry *entry = getValueFromHashMap(cache, key);
    if (NULL != entry) {
        updateLRUList(cache, entry);
        return entry->data;
    } else {
        return '\0';
    }
}

/*遍历缓存链表，打印缓存中的数据，按访问时间从新到旧的顺序输出*/
void LRUCachePrint(void *lruCache)
{
    LRUCacheS *cache = (LRUCacheS *)lruCache;
    if (NULL==cache||0 == cache->lruListSize) return ;
    fprintf(stdout, "\n>>>>>>>>>>\n");
    fprintf(stdout, "cache  (key  data):\n");
    cacheEntryS *entry = cache->lruListHead;
    while(entry) {
        fprintf(stdout, "(%c, %c) ", entry->key, entry->data);
        entry = entry->lruListNext;
    }
    fprintf(stdout, "\n<<<<<<<<<<\n\n");
}







