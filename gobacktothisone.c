/* * mm-naive.c - The fastest, least memory-efficient malloc package.
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
    "ILOVECSOGIVEM",
    /* First member's full name */
    "Savannah Lim",
    /* First member's email address */
    "ssl399@nyu.edu",
    /* Second member's full name (leave blank if none) */
    "Nicholas Hyland",
    /* Second member's email address (leave blank if none) */
    "nsh263@nyu.edu"
};

// basic constants and macros
#define WSIZE	4 // word and header/footer byte size
#define DSIZE	8 // double word byte size
#define CHUNKSIZE (1 << 7) // extend heap by this byte size amount, 2^7
#define MIN	32 // because of 16 alignment

#define MAX(x, y) ((x) > (y)? (x) : (y))

// pack a size and allocated bit into a word
#define PACK(size, alloc) ((size) | (alloc))

// read and write a word at address p
#define GET(p)		(*(unsigned int *)(p))
#define PUT(p, val)	(*(unsigned int *)(p) = (val))

// read the size and allocated fields from address p
// ~0x7 = 1111110
#define GET_SIZE(p)	(GET(p) & ~0x7)
#define GET_ALLOC(p)	(GET(p) & 0x1)

// given block ptr bp, compute address of its header and footer
#define HDRP(bp)	((char *)(bp) - WSIZE)
#define FTRP(bp)	((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// given block ptr bp, compute address of next and previous blocks
#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define NEXT_FREE_BLKP(bp)	(*(void **)(bp + DSIZE))
#define PREV_FREE_BLKP(bp)	(*(void **)bp)

// global variables
char * heap_head = 0; // pointer to first block
char * free_head = 0;

static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void add_free(void *bp);
static void delete(void *bp);

static void delete(void *bp) {
	if (PREV_FREE_BLKP(bp) == NULL) {
		free_head = NEXT_FREE_BLKP(bp);
	}
	else if (PREV_FREE_BLKP(bp) != NULL) {
		NEXT_FREE_BLKP(PREV_FREE_BLKP(bp)) = NEXT_FREE_BLKP(bp);
	}
	else if ((NEXT_FREE_BLKP(bp)) != NULL) {
		free_head = NEXT_FREE_BLKP(bp);
	}

/*	if(bp == free_head) printf("\n\nHEllo\n\n");
        if(NEXT_FREE_BLKP(bp) == free_head) printf("\n\nbyro\n\n");
        if(PREV_FREE_BLKP(bp) == free_head) printf("\n\nHplo\n\n");

	printf("%p", bp);

        if(bp == heap_head) printf("\n\nHEll1111o\n\n");
        if(NEXT_FREE_BLKP(bp) == heap_head) printf("\n\n111byro\n\n");
        if(PREV_FREE_BLKP(bp) == heap_head) printf("\n\nHpl1111o\n\n");

        void *bp2 = heap_head;
        for (; GET_SIZE(HDRP(bp2)) > 0; bp2 = NEXT_BLKP(bp2)) {
                printf("-%d,%d-", GET_SIZE(HDRP(bp2)), GET_ALLOC(HDRP(bp2)));
	}

	if(1) printf("\n%d!\n", GET_SIZE((HDRP(*(void **)free_head))));

	bp2 = free_head;
        for (; GET_SIZE(HDRP(bp2)) > 0; bp = NEXT_FREE_BLKP(bp2)) {
                printf("|%d,%d|", GET_SIZE(HDRP(bp2)), GET_ALLOC(HDRP(bp2)));
        }
	
//	bp2 = heap_head;
//      for (; GET_SIZE(HDRP(bp2)) > 0; bp2 = NEXT_BLKP(bp2)) {
//            	printf("-%d,%d-", GET_SIZE(HDRP(bp2)), GET_ALLOC(HDRP(bp2)));     //      }
*/	

	NEXT_FREE_BLKP(PREV_FREE_BLKP(bp)) = NEXT_FREE_BLKP(bp);
}

static void add_free(void *bp) {
	//NEXT_FREE_BLKP(bp) = free_head;
	//PREV_FREE_BLKP(free_head) = bp;
	//PREV_FREE_BLKP(bp)= NULL;
	//free_head = bp;
	void *curr_top = free_head;
        NEXT_FREE_BLKP(bp) = curr_top;
        PREV_FREE_BLKP(curr_top) = bp;
        PREV_FREE_BLKP(bp) = NULL;
        free_head = bp;	
}

static void *coalesce(void *bp) {
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	
	// case 1
//	if (prev_alloc && next_alloc) {
//		return bp;
//	}
	
	// case 2
	if (prev_alloc && !next_alloc) {
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		delete(bp);
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size,0));
    	}

	// case 3
	else if (!prev_alloc && next_alloc) {
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		bp = PREV_BLKP(bp);
		delete(bp);
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    	}

	// case 4

	else {                         
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
		GET_SIZE(FTRP(NEXT_BLKP(bp)));
		delete(NEXT_BLKP(bp));
		delete(PREV_BLKP(bp));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
    	}
	add_free(bp);
	return bp;
}

static void * extend_heap(size_t words) {
	void *bp;
	size_t size;

	// allocate an even number of words to maintain alignment
//	size = ALIGN(words * WSIZE);
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;
	
	// initialize free block header/footer and the epilogue header
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

	// coalesce if the previous block was free
	return coalesce(bp);
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	// create initial empty heap
	// mem_sbrk, from memlib.c expands the heap by x bytes, 
	// and returns a generic pointer
	// to the first byte of the newly allocated heap area
//	heap_head = mem_sbrk(4 * WSIZE);
	
	// initialize array of pointers

	// error
	if ((heap_head = mem_sbrk(4 * WSIZE)) == (void *) - 1)
		return -1;
	// #define PUT(p, val)     (*(unsigned int *)(p) = (val))
	// does this keep moving heap_head?
	PUT(heap_head, 0); // alignment padding
	// heap_head = 0
	PUT(heap_head + (1 * WSIZE), PACK(DSIZE, 1)); // prologue header
	// PUT(5, (8 | 1)) << is this right?
	PUT(heap_head + (2 * WSIZE), PACK(DSIZE, 1)); // prologue footer
	PUT(heap_head + (3 * WSIZE), PACK(0, 1)); // epilogue header
	heap_head = heap_head + (2 * WSIZE);

//	free_head = heap_head;
//	free_head = NULL;
	free_head = heap_head + DSIZE;

	// extend empty heap with a free block of CHUNKSIZE bytes	
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;
	return 0; 
}

static void *find_fit(size_t asize) {
	// first-fit search
	void *bp = free_head;
	for (; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_FREE_BLKP(bp)) {
		if (asize <= (size_t) GET_SIZE(HDRP(bp))) {
			return bp;
		}
	}
	return NULL; // no fit	
}

static void place(void *bp, size_t asize) {
	size_t csize = GET_SIZE(HDRP(bp));
	if ((csize - asize) >= (2 * DSIZE)) {
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		delete(bp);
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize - asize, 0));
		PUT(FTRP(bp), PACK(csize - asize, 0));
	}
	else {
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
		delete(bp);
	}
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{	// keep this	
//	int newsize = ALIGN(size + SIZE_T_SIZE);
//	int newsize = ALIGN(size + DSIZE);
//	void *p = mem_sbrk(newsize);
//	if (p == (void *)-1)
//		return NULL;
//	else {
//        	*(size_t *)p = size;
//        	return (void *)((char *)p + SIZE_T_SIZE);
//	}
	// keep this
	size_t asize; // adjusted block size
	size_t extendsize; // amount to extend heap if no fit
	char *bp;

	// ignore spurious requests
	if (size == 0) 
		return NULL;

	// adjust block size to include overhead and alignment reqs
	//asize = (MAX(ALIGN(size), 32));
//	if (size < 32) {
//		asize = ALIGN(MIN);
//	}
//	else {
//		asize = ALIGN(size);
//	}
//	if (size <= (3 * DSIZE)) {
//		newsize = 2 * DSIZE;
//		asize = 4 * DSIZE;
//	}
//	else {
//              asize = newsize;
//		asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
//	}
//	asize = ALIGN(asize);

	asize = ALIGN(MAX(size + DSIZE + DSIZE, 24));

	// search the free list for a fit

//      if ((bp = find_fit(newsize)) != NULL) {
//              place(bp, newsize);
//              return bp;
//      }

	if ((bp = find_fit(asize)) != NULL) {
		place(bp, asize);
		return bp;
	}

	// no fit found, get more memory and place the block
//	extendsize = MAX(newsize, CHUNKSIZE);
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
		return NULL;
//	place(bp, newsize);
	place(bp, asize);
	return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
	if (bp == NULL) {
		return;
	}
	size_t size = GET_SIZE(HDRP(bp));
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
//    void *oldptr = ptr;
//    void *newptr;
//    size_t copySize;
    
//    newptr = mm_malloc(size);
//    if (newptr == NULL)
//      return NULL
//    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
//    if (size < copySize)
//      copySize = size;
//    memcpy(newptr, oldptr, copySize);
//    mm_free(oldptr);
//    return newptr;

    size_t oldsize;
    void *newptr; 

    // If size == 0 then this is just free, and we return NULL. KEEP
    if(size == 0) {
	mm_free(ptr);
	return 0;
    }

    // If oldptr is NULL, then this is just malloc. KEEP
    if(ptr == NULL) {
	return mm_malloc(size);
    }

    newptr = mm_malloc(size);

    // If realloc() fails the original block is left untouched 
    if(!newptr) {
	return 0;
    }

    // Copy the old data.
    oldsize = GET_SIZE(HDRP(ptr));
    if(size < oldsize) oldsize = size;
//  memcpy(newptr, ptr, oldsize);

    // Free the old block. 
    mm_free(ptr);

    return newptr;

}


void mm_checkheap(int verbose) 
{
	return;
}

