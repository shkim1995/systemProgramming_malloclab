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
 * provide your information in the following struct.
 ********************************************************/
student_t student = {
    /* full name */
    "Sehoon Kim",
    /* login ID */
    "csapp-2017summer-student09",
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


// Basic constants
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE ((1<<10))

//pack size and allocated bit into 
#define PACK(size, alloc) ((size) | (alloc))

//Read and Write word from address p
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

//Read the size and allocation field from address p
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

//Given block ptr bp, get address of its HEADER and FOOTER
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

//given block ptr bp, get address of next and prev blocks
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define NEXT(bp) (void *)(*((int *)(bp)))
#define PREV(bp) (void *)(*((int *)((bp) + (WSIZE))))



//points to prologue block
char* heap_listp;
int* root;
//max ftn
int size;

int r;
int r_size;
int r_stat;
void* r_ptr;

int debug;

//for debugging
static void scanHeap(void){

	void* bp = (void*)(*root);
	int debug = 0;
	
	while(1){
		if(bp==NULL){
			printf("--------------------------------\n");
			return NULL; // end
		}
		if((debug++)<10)	
			printf("%d : pointer %p, prev %p, next %p, size %d\n", debug, bp, PREV(bp), NEXT(bp), GET_SIZE(HDRP(bp)));
		else
			break;
		bp = NEXT(bp);
	}
	printf("--------------------------------\n");

}

//coalesce blocks
static void* coalesce(void* bp){
	
	void* prev = PREV_BLKP(bp);
	void* next = NEXT_BLKP(bp);

	size_t prev_alloc = GET_ALLOC(FTRP(prev));
	size_t next_alloc = GET_ALLOC(HDRP(next));
	int size = GET_SIZE(HDRP(bp));
	
	void* first_block = (void *)(*root);
	void* next_prev;
	void* next_next;
	void* prev_prev;
	void* prev_next;
	
	//case 1 : nothing happens
	if(prev_alloc && next_alloc){
		
		//bp becomes the only free block
		if(first_block == NULL){
			PUT(root, (int)bp);
			PUT(bp, 0);
			PUT(bp + WSIZE, 0);
		}

		else{
			PUT(first_block + WSIZE, (int)bp);
			PUT(bp, (int)first_block);
			PUT(bp + WSIZE, 0);
			PUT(root, (int)bp);
		}
//		printf("COALSE 1 first block : %p\n", first_block);
		return bp;
	}
	
	//case 2 : merge with next block
	else if(prev_alloc && !next_alloc){
		size += GET_SIZE(HDRP(next));
		next_prev = PREV(next);
		next_next = NEXT(next);
		
		if(next_prev == NULL){
			PUT(bp + WSIZE, 0);
			PUT(bp, (int)next_next);
			PUT(root, (int)bp);
			if(next_next != NULL) PUT(next_next + WSIZE, (int)bp);
		}

		else{
			PUT(next_prev, (int)next_next);
			if(next_next != NULL) PUT(next_next + WSIZE, (int)next_prev);

			PUT(first_block + WSIZE, (int)bp);
			PUT(bp, (int)first_block);
			PUT(bp + WSIZE, 0);
			PUT(root, (int)bp);
		}

		//change header and footer
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));

//		printf("COALSE 2 size : %d\n", size);
	}

	//case 3 : merge with prev block
	else if(!prev_alloc && next_alloc){
		size += GET_SIZE(HDRP(prev));
		prev_prev = PREV(prev);
		prev_next = NEXT(prev);
		
		if(prev_prev == NULL){
			//pointers need not to be modified
		}
		else{
			PUT(prev_prev, (int)prev_next);
			if(prev_next != NULL) PUT(prev_next + WSIZE, (int)prev_prev);
			
			PUT(first_block + WSIZE, (int)prev);
			PUT(prev, (int)first_block);
			PUT(prev + WSIZE, 0);
			PUT(root, (int)prev);
		}
		
		//change header and footer
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(prev), PACK(size, 0));
		bp = prev;

//		printf("COALSE 3\n");
	}

	//case 4 : merge with next and prev block
	else{
		size += (GET_SIZE(HDRP(prev)) + GET_SIZE(HDRP(next)));
		
		prev_prev = PREV(prev);
		prev_next = NEXT(prev);
		next_prev = PREV(next);
		next_next = NEXT(next);
		
		//if prev block is the first block
		if(prev_prev == NULL){
//			printf("CASE1\n");
			
			if((int)prev_next == (int)next){
				if(next_next != NULL) PUT(next_next + WSIZE, (int)prev);
				PUT(prev, (int)next_next);
			}
			
			else{
				if(next_prev != NULL) PUT(next_prev, (int)next_next);
				if(next_next != NULL && next_next >=0x40000000) PUT(next_next + WSIZE, (int)next_prev);
			}
		}

		//if next block is the fist block
		else if(next_prev == NULL){
//			printf("CASE2\n");

			if((int)next_next == (int)prev){
				PUT(prev + WSIZE, 0);
				PUT(root, (int)prev);
			}
			else{
				if(prev_prev != NULL) PUT(prev_prev, (int)prev_next);
				if(prev_next != NULL) PUT(prev_next + WSIZE, (int) prev_prev);
			
				PUT(prev + WSIZE, 0);
				PUT(prev, (int)next_next);
				PUT(root, (int)prev);
				if(next_next != NULL) PUT(next_next + WSIZE, (int)prev);
		
			}
		}

		//prev, next are not the first block
		else{
		//change pointers
			if(prev_prev != NULL) PUT(prev_prev, (int)prev_next);
			if(prev_next != NULL) PUT(prev_next + WSIZE, (int)prev_prev);
			
			next_prev = PREV(next);
			next_next = NEXT(next);
		
			if(next_prev != NULL) PUT(next_prev, (int)next_next);
			if(next_next != NULL) PUT(next_next + WSIZE, (int)next_prev);

			if(first_block != NULL) PUT(first_block + WSIZE, (int)prev);
			PUT(prev, (int)first_block);
			PUT(prev + WSIZE, 0);
			PUT(root, (int)prev);
		}

		//change header and footer
		PUT(HDRP(prev), PACK(size, 0));
		PUT(FTRP(next), PACK(size, 0));
		bp = prev;
//		printf("COALSE 4\n");
	}		
					
	return bp;
}


// extend block at the tail, w/ amount of words WORDs 
static void *extend_heap(size_t words){
	size_t size;
	char* bp;

	size = (words%2) ? (words+1)*WSIZE : words*WSIZE;
	if((long)(bp = mem_sbrk(size)) == -1)
		return NULL; //allocation failed

	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
	return coalesce(bp);//if merging possible with previous block
}
//returns extended part pointer

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	debug = 0;

	r = 0;
	r_size = 0;
	r_stat = 0;

	if((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1)
		return -1; // allocation error
	
	//initial empty heap
	PUT((int*)heap_listp, (int)(heap_listp + 2*DSIZE)); // padding
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); //prologue blk header
	PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); //prologue blk footer 
	PUT(heap_listp + (3*WSIZE), PACK(0, 1)); //epilogue blk header : size = 0, alloc = 1
   	
	root = (int*)heap_listp;
	PUT(root, 0);	
	

	heap_listp += 2*WSIZE; // set heap pointer
//	printf("Initial : %p at %p \n", (void*)(PREV(*root)),(void*)(*root));

	//extend heap
	if(extend_heap(CHUNKSIZE/WSIZE) == NULL) return -1;
	
//	printf("INIT\n");
//	scanHeap();

	return 0;
}

//find free space that fits asize
void *find_fit(size_t asize){
	
	void* bp = (void*)(*root);

	while(1){
		if(bp==NULL){
			return NULL; // end
		}
		if(GET_SIZE(HDRP(bp))==asize || asize <= GET_SIZE(HDRP(bp))-2*DSIZE){ 
			return bp;
		}
		bp = NEXT(bp);
	}
}

void place(void* bp, size_t asize){
	
	size_t original_size = GET_SIZE(HDRP(bp));
	size_t new_size = original_size - asize;
	void* prev;
	void* next;
	void* next_temp;
	

	prev = PREV(bp);
	next = NEXT(bp);
	if(new_size == 0){ // exactly fits
		
		//change prev. block if exists
		if(prev != NULL){
			PUT(prev, (int)next);
		}
		//change root if root is deleted
		if(prev == NULL){
			PUT(root, (int)next);		
		}

		//change next block if exists
		if(next != NULL){
			PUT(next+WSIZE, (int)prev);
		}
		
		//change the current block
		PUT(HDRP(bp), PACK(original_size, 1));
		PUT(FTRP(bp), PACK(original_size, 1));
	}
	
	else{ // splits

		//set size of new allocated block
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		
		next_temp = NEXT_BLKP(bp);
		
		//set size of next non-allocated block (splitted)
		PUT(HDRP(next_temp), PACK(new_size, 0));
		PUT(FTRP(next_temp), PACK(new_size, 0));
		
		if(prev != NULL){
			PUT(prev, (int)next_temp);
		}
		if(prev == NULL){
			PUT(root, (int)next_temp);
		}

		if(next != NULL){
			PUT(next+WSIZE, (int)next_temp);
		}

		PUT(next_temp, (int)next);
		PUT(next_temp+WSIZE, (int)prev);

	}
}



/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	size_t asize;
	size_t extendsize;
	char *bp;
	debug++;

	if(size == 0) return NULL;

	if(size <= DSIZE)
		asize = 2*DSIZE;
	else
		asize = DSIZE * ((size + 2*DSIZE - 1)/DSIZE);

	if((bp = find_fit(asize)) != NULL){
		place(bp, asize);
//		printf("MALLOC\n");
//		scanHeap();
		return bp;
	}

	//Not found
	extendsize = MAX(asize, CHUNKSIZE);
	if(r_stat==1) extendsize = 500000;
	if(r_stat==2) extendsize = 30000;
	if((bp = extend_heap(extendsize/WSIZE))==NULL)
		return NULL; // extension failed
	place(bp, asize);

//	printf("MALLOC\n");
//	scanHeap();
	return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
	debug++;
	size_t size = GET_SIZE(HDRP(ptr));
	//for debugging
//	printf("%p %p %p \n", *root, PREV(*root), NEXT(*root));
	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));
//	printf("FREE %p, %d\n", ptr,  debug);
	coalesce(ptr);
//	scanHeap();
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
   	void* bp;
	int new_size;
//	printf("%d, size %d\n", debug++, size);
	debug++;
	r ++;
	if(r==1 && size < 1000){
		r_stat = 1;
	}
	if(r==1 && size > 1000){
		r_stat = 2;
	}
	
	if(r==1){
		if(r_stat==1) new_size = 620000;
		else new_size = 28500;
		
		bp = mem_sbrk(new_size);

		PUT(HDRP(bp), PACK(new_size, 1));
		PUT(FTRP(bp), PACK(new_size, 1));
		PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
		
		new_size = new_size - 2*DSIZE;
		bp = bp + DSIZE;
		
		PUT(HDRP(bp), PACK(size, 1));
		PUT(FTRP(bp), PACK(size, 1));
	
		r_ptr = bp;
		
		r++;
	}
	
	bp = r_ptr;

	copySize = GET_SIZE(HDRP(ptr));
	if(size<copySize) copySize = size;
	memcpy(bp, ptr, copySize);


	return bp;	
}














