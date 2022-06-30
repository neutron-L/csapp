/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Good Luck",
    /* First member's full name */
    "Rob Font",
    /* First member's email address */
    "2055017970@qq.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ //line:vm:mm:beginconst
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */  //line:vm:mm:endconst 
#define CLASS_SIZE 10

#define MAX(x, y) ((x) > (y)? (x) : (y))  
#define MIN(x, y) ((x) < (y)? (x) : (y))  


/* Pack a size and allocated bit into a word */
// #define PACK(size, alloc)  ((size) | (alloc)) //line:vm:mm:pack
#define PACK(size, prev_alloc, alloc) ((size) | (alloc) | (prev_alloc << 1))
/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))            //line:vm:mm:get
#define PUT(p, val)  (*(unsigned int *)(p) = (val))    //line:vm:mm:put
#define GET_ADDRESS(p) (*(void **)(p))


/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   //line:vm:mm:getsize
#define GET_ALLOC(p) (GET(p) & 0x1)                    //line:vm:mm:getalloc
#define GET_PREV_ALLOC(p) ((GET(p) >> 1) & 0x1)


#define PRED(bp) ((char *)(bp))       //祖先节点
#define SUCC(bp) ((char *)(bp)+WSIZE)   //后继结点


/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      //line:vm:mm:hdrp
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //line:vm:mm:ftrp

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //line:vm:mm:nextblkp
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //line:vm:mm:prevblkp

#define GET_PAYLOAD(bp) (GET_SIZE(HDRP(bp))-WSIZE) 
/* $end mallocmacros */

/* Global variables  */
/* {1,31},{32,63},{64,127},{128,255},{256,511},{512,1023},{1024,2047},
{2048,4095},{4096,8191},{8192,inf} */
static void* linkhead[CLASS_SIZE]={NULL};      

static char * heap_listp = 0;
// static void * pEnd = NULL;
static char * rover;


/* static function */
static void *merge_block(void *bp, size_t extend_size);
static void *extend_heap(size_t words);
static void *coalesce(void * bp); 
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void insert_block(void *bp);
static void remove_block(void *bp);
static int find_link(size_t size); // Determine the linked list index corresponding to the block size
static size_t get_block_size(size_t size);


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    mem_init();
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1, 1)); /* Prologue header */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1, 1)); /* Prologue footer */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1, 1));
    heap_listp += (2*WSIZE);
    rover = heap_listp;

    for (int i = 0; i < CLASS_SIZE; ++i)
        linkhead[i] = NULL;

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extend_size;
    char * bp;

    if (size == 0)
        return NULL;

    asize = get_block_size(size);
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }
    
    // extend_size = MAX(asize, CHUNKSIZE);
    extend_size = asize + 2*DSIZE;
    if ((bp = extend_heap(extend_size / WSIZE)) == NULL)
        return NULL;

    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, GET_PREV_ALLOC(HDRP(ptr)), 0));
    PUT(FTRP(ptr), PACK(size, GET_PREV_ALLOC(HDRP(ptr)), 0));

    if (GET_ALLOC(HDRP(NEXT_BLKP(ptr))))
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(GET_SIZE(HDRP(NEXT_BLKP(ptr))), 0, 1));
    else
    {
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(GET_SIZE(HDRP(NEXT_BLKP(ptr))), 0, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(GET_SIZE(HDRP(NEXT_BLKP(ptr))), 0, 0));
    }

    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return mm_malloc(size);
    else if (size == 0)
    {
        mm_free(ptr);
        return ptr;
    }
    else
    {
        size_t payload_size = GET_PAYLOAD(ptr);
        size_t asize = get_block_size(size);
        
        if (size <= payload_size)
        {
            if (GET_SIZE(HDRP(ptr)) - asize >= 2*DSIZE)
            {
                size_t next_size = GET_SIZE(HDRP(ptr)) - asize;
                PUT(HDRP(ptr), PACK(asize, GET_PREV_ALLOC(HDRP(ptr)), 1));
                void * next = NEXT_BLKP(ptr);
                PUT(HDRP(next), PACK(next_size, 1, 0));
                PUT(FTRP(next), PACK(next_size, 1, 0));
                if (GET_ALLOC(HDRP(NEXT_BLKP(next))))
                    PUT(HDRP(NEXT_BLKP(next)), PACK(GET_SIZE(HDRP(NEXT_BLKP(next))), 0, 1));
                else
                {
                    PUT(HDRP(NEXT_BLKP(next)), PACK(GET_SIZE(HDRP(NEXT_BLKP(next))), 0, 0));
                    PUT(FTRP(NEXT_BLKP(next)), PACK(GET_SIZE(HDRP(NEXT_BLKP(next))), 0, 0));
                }
                coalesce(next);
            }
            return ptr;
        }
        else
        {
            size_t extend_size = asize - GET_SIZE(HDRP(ptr));
            void *newptr = merge_block(ptr, extend_size);
            // void *newptr = NULL;
            if (newptr == NULL)
            {
                newptr = mm_malloc(size);
                if (newptr != NULL)
                {
                    memcpy(newptr, ptr, payload_size);
                    mm_free(ptr);
                }
            }
            return newptr;    
        }
    }
}


static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words & 0x1) ? (words + 1) * WSIZE : words * WSIZE;

    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    PUT(HDRP(bp), PACK(size, GET_PREV_ALLOC(HDRP(bp)), 0));
    PUT(FTRP(bp), PACK(size, GET_PREV_ALLOC(HDRP(bp)), 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 0, 1));

    return coalesce(bp);
}

static void *merge_block(void *bp, size_t extend_size)
{
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    void * prev = PREV_BLKP(bp);
    void * next = NEXT_BLKP(bp);
    size_t prev_size = GET_SIZE(HDRP(prev));
    size_t next_size = GET_SIZE(HDRP(next));
    size_t aval_size = 0;
    
    if (!prev_alloc)
        aval_size += prev_size;

    if (!next_alloc)
        aval_size += next_size;

    if (aval_size < extend_size)
        return NULL;
    else
    {
        if (!next_alloc && next_size >= extend_size)
        {
            remove_block(next);
            if (next_size - extend_size >= 2*DSIZE)
            {
                PUT(HDRP(bp), PACK(size + extend_size, prev_alloc, 1));
                next = NEXT_BLKP(bp);
                PUT(HDRP(next), PACK(next_size - extend_size, 1, 0));
                PUT(FTRP(next), PACK(next_size - extend_size, 1, 0));
                insert_block(next);
            }
            else
            {
                PUT(HDRP(bp), PACK(size + next_size, prev_alloc, 1));
                next = NEXT_BLKP(bp);
                size_t alloc = GET_ALLOC(HDRP(next));
                if (alloc)
                    PUT(HDRP(next), PACK(GET_SIZE(HDRP(next)), 1, 1));
                else
                {
                    PUT(HDRP(next), PACK(GET_SIZE(HDRP(next)), 1, 0));
                    PUT(FTRP(next), PACK(GET_SIZE(HDRP(next)), 1, 0));
                }
            }
        } 
        else
        {
            remove_block(prev);
            if (!next_alloc)
                remove_block(next);
            if (aval_size - extend_size >= 2*DSIZE)
            {
                size += extend_size;
                PUT(HDRP(prev), PACK(size, GET_PREV_ALLOC(HDRP(prev)), 1));
                memcpy(prev, bp, GET_PAYLOAD(bp));
                bp = prev;
                aval_size -= extend_size;
                next = NEXT_BLKP(bp);
                PUT(HDRP(next), PACK(aval_size, 1, 0));
                PUT(FTRP(next), PACK(aval_size, 1, 0));
                void * ptr = NEXT_BLKP(next);
                size_t alloc = GET_ALLOC(HDRP(ptr));
                if (alloc)
                    PUT(HDRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0, 1));
                else
                {
                    PUT(HDRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0, 0));
                    PUT(FTRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0, 0));
                }                    
                coalesce(next);
            }
            else
            {
                size += aval_size;
                PUT(HDRP(prev), PACK(size, GET_PREV_ALLOC(HDRP(prev)), 1));
                memcpy(prev, bp, GET_PAYLOAD(bp));
                bp = prev;
                next = NEXT_BLKP(bp);
                size_t alloc = GET_ALLOC(HDRP(next));
                if (alloc)
                    PUT(HDRP(next), PACK(GET_SIZE(HDRP(next)), 1, 1));
                else
                {
                    PUT(HDRP(next), PACK(GET_SIZE(HDRP(next)), 1, 0));
                    PUT(FTRP(next), PACK(GET_SIZE(HDRP(next)), 1, 0));
                }                    
            }
        }
    }

    return bp;
}

static void *coalesce(void * bp)
{
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
        ;
    
    else if (prev_alloc && !next_alloc)
    {
        remove_block(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 1, 0));
        PUT(FTRP(bp), PACK(size, 1, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        remove_block(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 1, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 1, 0));
        bp = PREV_BLKP(bp);
    }
    else
    {
        remove_block(NEXT_BLKP(bp));
        remove_block(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 1, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 1, 0));
        bp = PREV_BLKP(bp);
    }


    insert_block(bp);
    if ((rover > (char *)bp) && (rover < NEXT_BLKP(bp)))
        rover = bp;

    return bp;
}


static void * find_fit(size_t asize)
{
    void *bp;
    for (int i = find_link(asize); i < CLASS_SIZE; ++i)
    {
        bp = linkhead[i];
        
        while (bp != NULL && GET_SIZE(HDRP(bp)) < asize)
            bp = GET_ADDRESS(SUCC(bp));
        if (bp != NULL)
            return bp;
    }
    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    remove_block(bp);

    if ((csize - asize) >= (2*DSIZE)) 
    {
        PUT(HDRP(bp), PACK(asize, 1, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 1, 0));
        PUT(FTRP(bp), PACK(csize-asize, 1, 0));
        insert_block(bp);
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(GET_SIZE(HDRP(NEXT_BLKP(bp))), 1, 1));
    }
}

static void insert_block(void *bp)
{
    // find the link list
    size_t size = GET_SIZE(HDRP(bp));
    int idx = find_link(size);
    void * head = linkhead[idx];

    if (head == NULL)
    {
        linkhead[idx] = bp;
        GET_ADDRESS(SUCC(bp)) = NULL;
    }
    else
    {
        void *pre = NULL;
        while (head != NULL)
        {
            if (GET_SIZE(HDRP(head)) >= size)
                break;
            pre = head;
            head = GET_ADDRESS(SUCC(head));
        }

        if (head == NULL)
        {
            GET_ADDRESS(SUCC(pre)) = bp;
            GET_ADDRESS(PRED(bp)) = pre;
            GET_ADDRESS(SUCC(bp)) = NULL;
        }
        else
        {
            if (head == linkhead[idx])
            {   
                GET_ADDRESS(SUCC(bp)) = head;
                GET_ADDRESS(PRED(head)) = bp;
                linkhead[idx] = bp;
            }
            else
            {
                GET_ADDRESS(SUCC(pre)) = bp;
                GET_ADDRESS(PRED(bp)) = pre;
                GET_ADDRESS(SUCC(bp)) = head;
                GET_ADDRESS(PRED(head)) = bp;
            }
        }
    }
}


static void remove_block(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    int idx = find_link(size);

    if (linkhead[idx] == bp)
    {
        linkhead[idx] = GET_ADDRESS(SUCC(bp));
        return;
    }
    void * prev = GET_ADDRESS(PRED(bp));
    void * next = GET_ADDRESS(SUCC(bp));

    // SET_NEXT(prev, next);
    GET_ADDRESS(SUCC(prev)) = next;
    if (next != NULL)
        GET_ADDRESS(PRED(next)) = prev;
}


static int find_link(size_t size)
{
    int idx = 0;
    while (idx < CLASS_SIZE)
    {
        if (size < (1<<(idx+5)))
            break;
        idx++;
    }

    return MIN(idx, CLASS_SIZE - 1);
}

static size_t get_block_size(size_t size)
{
    if (size <= WSIZE)
        return 2*DSIZE;
    else
        return DSIZE * ((size + (WSIZE) + (DSIZE - 1)) / DSIZE);   
}

// int check(void * ptr)
// {
//     size_t size = GET_SIZE(ptr);
//     char *lo = ptr;
//     char *hi = lo + size - 1;
//     if ((lo < (char *)mem_heap_lo()) || (lo > (char *)mem_heap_hi()) || 
// 	(hi < (char *)mem_heap_lo()) || (hi > (char *)mem_heap_hi())) \
//     {
//         printf("lies over\n");
//         return 1;
//     }
//     return 0;
// }

// void analytical_parameters(void *ptrs[], char * buf)
// {
//     char func = buf[0];
//     buf += 2;

//     if (func != 'f')
//     {
//         char *delim = strchr(buf, ' ');
//         *delim = '\0';
//         int arg1 = atoi(buf);
//         buf = delim + 1;
//         int arg2 = atoi(buf);
//         // printf("%c %d %d\n", func, arg1, arg2);
//         if (func == 'a')
//         ptrs[arg1] = mm_malloc(arg2);
//         else
//             ptrs[arg1] = mm_realloc(ptrs[arg1], arg2);
//     // if (check(ptrs[arg1]))
//         //  printf("%c %d %d\n", func, arg1, arg2);

//     }
//     else 
//     {
//         int arg1 = atoi(buf);
//         // printf("%c %d\n", func, arg1);
//         mm_free(ptrs[arg1]);
//     //  if (check(ptrs[arg1]))
//     //    printf("%c %d\n", func, arg1);
//     }
// }

// int main()
// {
//     mm_init();
//     FILE * file = fopen("test.rep", "r");
//     char buf[64];
// // get numbers
// fgets(buf, sizeof buf, file);
// int num = atoi(buf);
// // printf("%d\n", num);
// void * ptrs[num];
// int idx = 1;
//     while (fgets(buf, sizeof buf, file))
//     {
//         // Analytical parameters
//         printf("%d\n", idx++);
//         analytical_parameters(ptrs, buf);
//     }
//     fclose(file);
//     // mm_init();
//     // void *a;
//     // void *b;
//     // void *c;
//     // void *d;
//     // void *e;
//     // void *f;
//     // a = mm_malloc(4092);
//     // b = mm_malloc(16);
//     // a = mm_realloc(a, 4097);
//     // c = mm_malloc(16);
//     // mm_free(b);
//     // a = mm_realloc(a, 4102);
//     // d = mm_malloc(16);
//     // mm_free(c);
//     // a = mm_realloc(a, 4017);
//     // e = mm_malloc(16);
//     // mm_free(d);
//     // // d = mm_malloc(4072);
//     // mm_free(a);
//     // // mm_free(c);
//     // // f = mm_malloc(4072);
//     // mm_free(e);

//     return 0;
// }

