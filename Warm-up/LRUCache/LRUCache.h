//
// Created by kyrie on 2017/6/5.
//

#ifndef KYRIE_C_LRUCACHE_H
#define KYRIE_C_LRUCACHE_H

// lruCache新建缓存句柄
int LRUCacheCreate(int capacity, void **lruCache);

int LRUCacheDestroy(void *lruCache);

int LRUCacheSet(void *lruCache, char key, char data);

char LRUCacheGet(void *lruCache, char key);

// 打印缓存中的数据，按访问时间从新到旧顺序输出
void LRUCachePrint(void *lruCache);

#endif //KYRIE_C_LRUCACHE_H
