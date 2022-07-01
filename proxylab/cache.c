#include "cache.h"


int cache_init(Cache *cache, int n, int s)
{
    cache->length = 0;
    cache->capacity = n + 1;
    cache->blocks = (CacheBlock **)Calloc(n + 1, sizeof(CacheBlock *));
    cache->space = s;
    cache->visitcnt = 0;
    Sem_init(&cache->modify_mutex, 0, 1);
    Sem_init(&cache->mutex, 0, 1);
}


void cache_deinit(Cache * cache)
{
    for (int i = 1; i <= cache->length; i++)
    {
        CacheBlock * block = cache->blocks[i];
        block_remove(block);
        cache->blocks[i] = NULL;
    }

    Free(cache->blocks);
}

// 在读过程中不能添加或删除cache中的元素
int read_cache(Cache *cache, char *url, int connfd)
{
    int i;
    int done = 0;
    // Obtaining a modification lock prevents other threads from adding or deleting blocks to the cache
    P(&cache->mutex);
    cache->visitcnt++;
    if (cache->visitcnt == 1)
        P(&cache->modify_mutex);
    V(&cache->mutex);

    for (i = 1; i <= cache->length; i++)
    {
        CacheBlock * block = cache->blocks[i];
        if (!strcmp(block->url, url))
        {
            block_read(block, connfd);
            done = 1;
            break;
        }
    }

    P(&cache->mutex);
    cache->visitcnt--;
    if (cache->visitcnt == 0)
        V(&cache->modify_mutex);
    V(&cache->mutex);

    return done;
}

void write_cache(Cache *cache, char *url, char *data)
{
    P(&cache->modify_mutex);
    CacheBlock *block = (CacheBlock *)malloc(sizeof(CacheBlock));
    block_init(block, url, data);
    // 判断是否空间足够添加新块
    while (cache->space <= block->size)
    {
        // 删除最少使用的若干块
        extract_old(cache);
        heapify_cache(cache, 1);
    }
    insert_block(cache, block);
    V(&cache->modify_mutex);
}



/* Operations on cache blocks */
static void block_init(CacheBlock * block, char *url, char *data)
{
    memset(block, 0, sizeof(CacheBlock));
    block->readcnt = 0;
    block->size = strlen(data);
    block->url = (char *)malloc(strlen(url)+1);
    strcpy(block->url, url);
    block->data = (char *)malloc(block->size + 1);
    strcpy(block->data, data);
    Sem_init(&block->mutex, 0, 1);
}


static void block_remove(CacheBlock *block)
{
    Free(block->data);
    Free(block->url);
}


static void block_read(CacheBlock *block, int connfd)
{
    P(&block->mutex);
    block->readcnt++;
    V(&block->mutex);
    Rio_writen(connfd, block->data, block->size);
    P(&block->mutex);
    block->readcnt--;
    if (block->readcnt == 0)
    {
        // update the block`s recent access time
        block->last_access_time >>= 1;
        block->last_access_time |= (1<<(8 * sizeof(unsigned int) - 1));
    }
    V(&block->mutex);
    Close(connfd);
}


/* Private interfaces are used to maintain and access cache space */
static void heapify_cache(Cache *cache, int idx)
{
    CacheBlock ** array = cache->blocks;
    int l, r, oldest;

    while (idx <= cache->length / 2)
    {
        l = LEFT(idx);
        r = RIGHT(idx);
        oldest = idx;
        if (array[l]->last_access_time < array[idx]->last_access_time)
            oldest = l;
        if ((r <= cache->length && array[r]->last_access_time < array[oldest]->last_access_time))
            oldest = r;
        if (oldest != idx)
        {
            exchange(cache, idx, oldest);
            idx = oldest;
        }
    }
}


static void insert_block(Cache *cache, CacheBlock *block)
{
    int idx = ++cache->length;
    if (idx == cache->capacity)
    {
        cache->blocks = (CacheBlock **)realloc(cache->blocks, CACHE_INCREMENT + cache->capacity);
        cache->capacity += CACHE_INCREMENT;
    }
    cache->blocks[idx] = block;
    cache->space -= block->size;
    int pidx = PARENT(idx);
    if (pidx)
    {
        block->last_access_time = cache->blocks[pidx]->last_access_time;
        block->last_access_time >>= 1;
        block->last_access_time |= (1<<(sizeof(unsigned int) * 8 - 1));
    }
    else
        block->last_access_time = 0;
    heapify_cache(cache, idx);
}


static void extract_old(Cache *cache)
{
    cache->space += cache->blocks[1]->size;
    exchange(cache, 1, cache->length);
    cache->length--;
}


static void exchange(Cache *cache, int i, int j)
{
    CacheBlock ** array = cache->blocks;
    CacheBlock * ptr = array[i];
    array[i] = array[j];
    array[j] = ptr;
}

