/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*macro from book*/
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<9)
#define MAX(x, y) ((x) > (y)? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc))
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x3)
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/*
My Macro
PRED (predecessor)
SUCC (successor)
*/
#define PRED(bp) ((char *)(bp) )
#define SUCC(bp) ((char *)(bp)+WSIZE )

void *heap_start;

/*Segregated free list*/
void *seg_list;
#define NUMSEG 15

//score 86.8/100
#define MAXSIZE1  (1<<5)
#define MAXSIZE2  (1<<6)
#define MAXSIZE3  (1<<7)
#define MAXSIZE4  (1<<8)
#define MAXSIZE5  (1<<9)
#define MAXSIZE6  (1<<10)
#define MAXSIZE7  (1<<11)
#define MAXSIZE8  (1<<12)
#define MAXSIZE9  (1<<13)
#define MAXSIZE10 (1<<14)
#define MAXSIZE11 (1<<16)
#define MAXSIZE12 (1<<17)
#define MAXSIZE13 (1<<18)
#define MAXSIZE14 (1<<19)


/*functions*/
int mm_init(void);
void *extend_heap(size_t words);
void insert(void *bp, size_t size);
void delete(void *bp, size_t size);
void *coalesce(void *bp);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);
void *find_fit(size_t size);
void place(void *bp, size_t size);
unsigned int * findpos(unsigned int *pos, size_t size);
void *mm_realloc(void *ptr, size_t size);
void* del8byte(char *ptr);

/* 
 * mm_init - initialize the malloc package.
 * __________________________________________________________________
 * segregated list |P header | P footer | Epilogue
 * __________________________________________________________________
*/
int mm_init(void)
{
    /*allocate space for segregated free list*/
    if ((seg_list = mem_sbrk(NUMSEG*WSIZE)) == (void *)-1)
        return -1;

    for(int i=0 ; i<NUMSEG ; i++){
        PUT(seg_list + (i*(WSIZE)), (unsigned int)NULL);
    }

    if ((heap_start = mem_sbrk(3*WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_start, PACK(DSIZE, 1));
    PUT(heap_start + 1*WSIZE, PACK(DSIZE, 1));
    PUT(heap_start + 2*WSIZE, PACK(0, 1));

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;

    return 0;   
}

void *extend_heap(size_t words){
    static char *bp;
    size_t size;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    PUT(HDRP(bp), PACK(size, 0));//prologue header
    PUT(FTRP(bp), PACK(size, 0));//prologue footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));//epilogue

    insert(bp,size);

    bp=coalesce(bp);

    return bp;
}

void insert(void *bp, size_t size){
    if((void *)NULL==bp) assert(0);

    static unsigned int *pos;   //position of list
    unsigned int prev;   //previous head

    pos=seg_list;
    pos=findpos(pos, size);
    prev=GET(pos);
    

    /*List of given size group is not empty*/
    if((unsigned int)NULL != prev){
        PUT(pos, (unsigned int)bp);
        PUT(PRED(bp), (unsigned int)NULL);
        PUT(SUCC(bp), (unsigned int)prev);
        PUT(PRED(prev), (unsigned int)bp);
    }
    /*List of give size group is empty*/
    else{
        PUT(pos, (unsigned int)bp);
        PUT(PRED(bp), (unsigned int)NULL);
        PUT(SUCC(bp), (unsigned int)NULL);
    }


}

/*LIFO order*/
/*bp is current node*/
void delete(void *bp, size_t size){
    if((void *)NULL==bp) assert(0);//assert function is wrong

    static unsigned int *pos;   //position of list
    unsigned int prev;
    /*next node's address is necessary to delete TOP node from list*/
    unsigned int next;
    prev=GET(PRED(bp));
    next=GET(SUCC(bp));
    pos=seg_list;
    pos=findpos(pos, size);

    /*case 1*/
    if(((unsigned int)NULL != prev) && ((unsigned int)NULL != next)){
        PUT(SUCC(prev),(unsigned int)next);
        PUT(PRED(next),(unsigned int)prev);

        PUT(SUCC(bp), (unsigned int)NULL );//delete the succ in current node
        PUT(PRED(bp), (unsigned int)NULL );//delete the pred in current node
    }
    /*case 2*/
    else if(((unsigned int)NULL != prev) && ((unsigned int)NULL == next)){
        PUT(SUCC(prev),(unsigned int)NULL);//delete the succ in previous node

        PUT(PRED(bp), (unsigned int)NULL );//delete the pred in current node

    }
    /*case 3*/
    else if(((unsigned int)NULL == prev) && ((unsigned int)NULL != next)){
        PUT(pos, (unsigned int)next); //change the element in heap

        PUT(SUCC(bp), (unsigned int)NULL );//delete the next in current node
        PUT(PRED(next), (unsigned)NULL );//delete the pred in next node
    }
    /*case 4*/
    else if(((unsigned int)NULL == prev) && ((unsigned int)NULL == next)){
        PUT(pos, (unsigned int)NULL);
    }

}

void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    size_t prev_size, next_size;

    /*
    if (prev_alloc && next_alloc){
        insert(bp,size);
        return bp;
    }*/

    /*current address next address should be removed from list
    and coalesced free memory data should be stored in the list
    */
    if(prev_alloc && next_alloc){
        return bp;
    }

    if (prev_alloc && !next_alloc){
        next_size=GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete(bp, size);
        delete(NEXT_BLKP(bp), next_size);

        size += next_size;
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));

    }

    else if (!prev_alloc && next_alloc){
        prev_size=GET_SIZE(HDRP(PREV_BLKP(bp)));

        delete(bp, size);
        delete(PREV_BLKP(bp),prev_size);

        size+=prev_size;

        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else if (!prev_alloc && !next_alloc){
        prev_size=GET_SIZE(HDRP(PREV_BLKP(bp)));
        next_size=GET_SIZE(FTRP(NEXT_BLKP(bp)));

        delete(PREV_BLKP(bp),prev_size);
        delete(NEXT_BLKP(bp),next_size);
        delete(bp,size);

        size+=prev_size;
        size+=next_size;

        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    insert(bp,size);
    return bp;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */


void *mm_malloc(size_t size){
    size_t extendsize; /* Amount to extend heap if no fit */
    static char *bp;

    if (size == 0)
        return NULL;

    if (size <= DSIZE){
        size = 2*DSIZE;
    }
    else{
        size = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    }
    
    if ((bp = find_fit(size)) != NULL){
        place(bp, size);
        return bp;
    }

    extendsize = MAX(size,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL){
        return NULL;
    }
    
    place(bp, size);
    return bp;
}

/*search for list*/
void *find_fit(size_t size){
    static unsigned int *pos;   //position of list
    static unsigned int *seg_list_end;
    static void* node;

    pos=seg_list;
    pos=findpos(pos, size);
    
    seg_list_end=(unsigned int *)seg_list;
    seg_list_end+=(NUMSEG-1);

    /*Search for seg_list to find the freed memory
    that is larger or equal than size*/
    node=NULL;
    for(; pos<=seg_list_end ; pos+=1){
        if(0 != GET(pos)){
            node=(void *)(GET(pos));
            break;
        }
    }

    /*Checking the size*/
    for(;0 != node;node=(void *)GET(SUCC(node))){
        if(GET_SIZE(HDRP(node))>=size){
            return node;
        }
    }
    
    /*No freed memory that is larger than size*/
    return NULL;
}


void place(void *bp, size_t size){
    size_t frag_size, remain_size;
    frag_size=GET_SIZE(HDRP(bp));
    remain_size=frag_size-size;

    /*delete from seglist*/
    delete(bp, frag_size);

    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));

    //save remain freed region
    if(remain_size>=16){
        PUT(HDRP(NEXT_BLKP(bp)), PACK(remain_size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(remain_size,0));
        insert(NEXT_BLKP(bp), remain_size);
    }
    //if remain_size is less than 16 it can't hold pred and succ info
    //in this case it doesn't freed during runtime
    //the case that remain_size is 4 doesn't occur
    //if remain_size==0 then all area is allocated
    else if(remain_size>=8){
        PUT(HDRP(NEXT_BLKP(bp)), PACK(remain_size,2));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(remain_size,2));
        del8byte((char *)bp);
    }  

    
}

/*return the position of correct size group*/
/*Pointer is 4 bytes*/
unsigned int * findpos(unsigned int *pos, size_t size){
    if (size<=MAXSIZE1)         pos+=0;
    else if (size<=MAXSIZE2)    pos+=1;
    else if (size<=MAXSIZE3)    pos+=2;
    else if (size<=MAXSIZE4)    pos+=3;
    else if (size<=MAXSIZE5)    pos+=4;
    else if (size<=MAXSIZE6)    pos+=5;
    else if (size<=MAXSIZE7)    pos+=6;
    else if (size<=MAXSIZE8)    pos+=7;
    else if (size<=MAXSIZE9)    pos+=8;
    else if (size<=MAXSIZE10)   pos+=9;
    else if (size<=MAXSIZE11)   pos+=10;
    else if (size<=MAXSIZE12)   pos+=11;
    else if (size<=MAXSIZE13)   pos+=12;
    else if (size<=MAXSIZE14)   pos+=13;
    else pos+=14;
    return pos;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr){
    if(NULL==ptr) return;

    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    if (2==GET_ALLOC(FTRP(PREV_BLKP(ptr)))) ptr=del8byte(ptr);

    insert(ptr, size);

    ptr=coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */

void *mm_realloc(void *ptr, size_t size){
    if(NULL==ptr && 0==size) assert(0);
    if(NULL==ptr) return mm_malloc(size);
    if(0==size){
        mm_free(ptr);
        return NULL;
    }

    static void *newptr;
    // before realloc size
    int br_size, next_size, next_alloc;
    size=ALIGN(size)+DSIZE;
    br_size=GET_SIZE(HDRP(ptr));
    next_size=GET_SIZE(HDRP(NEXT_BLKP(ptr)));
    next_alloc=GET_ALLOC(HDRP(NEXT_BLKP(ptr)));

    if (br_size==size) return ptr;

    if (size<br_size){
        PUT(HDRP(ptr), PACK(size, 1));
        PUT(FTRP(ptr), PACK(size, 1));
        if(br_size-size>=16){
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(br_size-size, 0));
            PUT(FTRP(NEXT_BLKP(ptr)), PACK(br_size-size, 0));
            insert(NEXT_BLKP(ptr), br_size-size);
            coalesce(NEXT_BLKP(ptr));
        }
        else if(br_size-size>=8){
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(br_size-size, 2));
            PUT(FTRP(NEXT_BLKP(ptr)), PACK(br_size-size, 2));
            del8byte((char *)ptr);
        }
    }
    else if(size>br_size){
        if(!next_alloc){
            delete(NEXT_BLKP(ptr), next_size);
            PUT(HDRP(ptr), PACK(size, 1));
            PUT(FTRP(ptr), PACK(size, 1));
            if(next_size-(size-br_size)>=16){
                PUT(HDRP(NEXT_BLKP(ptr)), PACK(next_size-(size-br_size) ,0));
                PUT(FTRP(NEXT_BLKP(ptr)), PACK(next_size-(size-br_size) ,0));
                insert(NEXT_BLKP(ptr), next_size-(size-br_size));
            }
            else if(next_size-(size-br_size)>=8){
                PUT(HDRP(NEXT_BLKP(ptr)), PACK(next_size-(size-br_size) ,2));
                PUT(FTRP(NEXT_BLKP(ptr)), PACK(next_size-(size-br_size) ,2));
                del8byte((char *)ptr);
            }

        }
        else{
            newptr=mm_malloc(size-DSIZE);
            memcpy(newptr, ptr, br_size-DSIZE);
            mm_free(ptr);
            ptr=newptr;
        }
    }
    return ptr;
}

/*8byte freed memory can't hold PREC and SUCC so it can't be freed by normal way
this function free 8byte freed memory
*/
void* del8byte(char *ptr){
    return ptr;
    //8 byte freed memory can be merged
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t curr_alloc = GET_ALLOC(HDRP(ptr));
    size_t size;

    
    if(2==curr_alloc && 0==next_alloc){
        size=GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        delete(NEXT_BLKP(ptr), size);

        PUT(ptr, (unsigned int)NULL);
        PUT(ptr+WSIZE, (unsigned int)NULL);
        PUT(ptr-WSIZE, PACK(size+8, 0));
        PUT(FTRP(ptr), PACK(size+8, 0));

        insert(ptr, size+8);

    }
    if(2==prev_alloc && 0==curr_alloc){
        size=GET_SIZE(HDRP(ptr));
        PUT(ptr-WSIZE, (unsigned int)NULL);
        PUT(ptr-DSIZE, (unsigned int)NULL);
        PUT(ptr-(3*WSIZE), PACK(size+8, 0));
        ptr-=DSIZE;
        PUT(FTRP(ptr), PACK(size+8, 0));
    }

    return (void *)ptr;
}
