#include "csapp.h"
#define CACHE_INCREMENT 20

#define PARENT(i) ((i)/2)
#define LEFT(i) (2*(i))
#define RIGHT(i)    LEFT(i) + 1

struct CacheBlock
{
    char *url;
    char *data;
    int readcnt; /* the number of readers */
    unsigned int last_access_time; /* the recent access time */
    int size; /* size of cache data */
    sem_t mutex; /* lock for readcnt */
};

typedef struct CacheBlock CacheBlock;

struct Cache
{
    CacheBlock ** blocks;
    int length; /* size of array */
    int capacity; /* capacity of array */
    int space; /* the size of used space */
    int visitcnt; /* Number of threads searching the cache */
    sem_t mutex; /* lock for write(modify last_access_time) or remove */
    sem_t modify_mutex; /* when append or remove a block, should lock, prevent the search operate */
};

typedef struct Cache Cache;

/* External interface, used to initialize and destroy cache, add cache records and read cache records */
int cache_init(Cache *list, int n, int s);
void cache_deinit(Cache * list);
int read_cache(Cache *list, char *url, int connfd);
void write_cache(Cache *list, char *url, char *data);

/* Operations on cache blocks */
static void block_init(CacheBlock * block, char *url, char *data);
static void block_remove(CacheBlock *block);
static void block_read(CacheBlock *block, int connfd);

/* Private interfaces are used to maintain and access cache space */
static void heapify_cache(Cache *cache, int idx);
static void insert_block(Cache *cache, CacheBlock *block);
static void extract_old(Cache *cache);
static void exchange(Cache *cache, int i, int j);
