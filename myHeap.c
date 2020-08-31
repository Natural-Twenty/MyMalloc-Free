// Heap management system

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "myHeap.h"

/** minimum total space for heap */
#define MIN_HEAP 4096
/** minimum amount of space for a free Chunk (excludes Header) */
#define MIN_CHUNK 32


#define ALLOC 0x55555555
#define FREE  0xAAAAAAAA

/// Types:

typedef unsigned int  uint;
typedef unsigned char byte;

typedef uintptr_t     addr; // an address as a numeric type

/** The header for a chunk. */
typedef struct header {
	uint status;    /**< the chunk's status -- ALLOC or FREE */
	uint size;      /**< number of bytes, including header */
	byte data[];    /**< the chunk's data -- not interesting to us */
} header;

/** The heap's state */
struct heap {
	void  *heapMem;     /**< space allocated for Heap */
	uint   heapSize;    /**< number of bytes in heapMem */
	void **freeList;    /**< array of pointers to free chunks */
	uint   freeElems;   /**< number of elements in freeList[] */
	uint   nFree;       /**< number of free chunks */
};


/// Variables:

/** The heap proper. */
static struct heap Heap;

/// Functions:
static void deleteList(void **freelist, void *p);
static int insertList(void **freelist, void *p);
static void mergeFreeChunks(void **freelist, int index);
static addr heapMaxAddr (void);


/** Initialise the Heap. */
int initHeap (int size)
{
	Heap.nFree = 0;
	Heap.freeElems = 0;
//  Make sure size is at least (minimum) the MIN_HEAP
	if (size < MIN_HEAP) {
	    size = MIN_HEAP;
	}
//  Round up if needed.
    if ((size % 4) > 0) { 
        size = size + (4 - (size%4));
    }
/*  Set up Heap members */
//  Set up memory region
    uint n = size/sizeof(void *);
//  Set heapMem to first byte and zeroes it out
    Heap.heapMem = calloc(n, sizeof(void *)); 
//  Return -1 if unsuccessful
    if (Heap.heapMem == NULL) {
        return -1;
    }
//  Initialise size member
    Heap.heapSize = size; 
//  Initial single large free chunk is free (increment to 1)
    Heap.nFree++; 
//  Set up freeList array
    n = size/(MIN_CHUNK*sizeof(void *));
//  Allocate memory for freeList array and zero out region
    Heap.freeList = calloc(n, sizeof(void *)); 
//  Return -1 if usuccessful
    if (Heap.freeList == NULL) {
        return -1;
    }
//  Set up freeElems as capacity of freeList 
    Heap.freeElems = n;
//  Set first element of freeList to first byte of heap
    header *first = (header *) Heap.heapMem;
//  Initialise it as header and add it to freeList
    *first = (header){.status = FREE, .size = size};
    Heap.freeList[0] = (void *)first;
//  Return 0 if everything is successful
	return 0; 
}

/** Release resources associated with the heap. */
void freeHeap (void)
{
	free (Heap.heapMem);
	free (Heap.freeList);
}

/** Allocate a chunk of memory large enough to store `size' bytes. */
void *myMalloc (int size)
{
//  Return NULL if size is less than 1
    if (size < 1) {
        printf("Invalid size");
        return NULL;
    }
//  Round up size if necessary
    if ((size % 4) > 0) {
        size = size + (4 - (size % 4));
    }
//  Return NULL if size + HeaderSize is larger than heap size
    uint HeaderSize = sizeof(header);
    uint totalSize = size + HeaderSize;
    if (totalSize > Heap.heapSize) {
        printf("Size too large");
        return NULL;
    }
    int i = 0;
    int replaceIndex = -1;
    header *Header;
    addr headerP;
    addr dataP;
    void *smallestChunk;
    uint smallestSize = Heap.heapSize+1;
//  Search freeList for smallest free block of memory with sufficient size
    while (Heap.freeList[i] != NULL) {
        Header = (header *)Heap.freeList[i];
        if (Header->status == FREE && Header->size >= totalSize 
                && Header->size < smallestSize) {
            smallestSize = Header->size;
            smallestChunk = Heap.freeList[i];
            headerP = dataP = (addr)Heap.freeList[i];
            replaceIndex = i;
        }
        i++;
    }
//  If no suitable block of memory is available return NULL
    if (replaceIndex == -1) {
        printf("Insufficient memory");
        return NULL;
    }
//  Set chunk to ALLOC 
    header *smallest = (header *)smallestChunk;
    smallest->status = ALLOC;
//  Assign whole chunk if n + headersize + MIN_CHUNK > chunk size
    if (smallest->size < totalSize + MIN_CHUNK + HeaderSize) {
        deleteList(Heap.freeList, smallest);
//  Otherwise split chunk and update header size, no change to freeList index
    } else {
    //  Create new header at the split
        headerP = headerP + totalSize;
        header *new = (header *)headerP;     
        *new = (header){.status = FREE, .size = Header->size - totalSize};
        Header->size = totalSize;
        Heap.freeList[replaceIndex] = (void *)new;
    }
    dataP = dataP + HeaderSize;   
//  Return pointer to beginning of data block (after header)
	return (void *) dataP;
}

/** Deallocate a chunk of memory. */
void myFree (void *obj)
{
/*
    obj does not represent an unallocated (free) chunk if:
    - obj points to NULL
    - obj points to already free chunk
	Obj pointer points to end of header
*/
//	Print message and exit if obj does not point to anything
	if (obj == NULL) {
	    printf("Attempt to free unallocated chunk\n");
	    exit(1);
	}
//  Move obj pointer to header
	addr headerP = (addr)obj - sizeof(header);
	header *free = (header *)headerP;
//  If chunk is allocated, update status to FREE
	if (free->status == ALLOC) {
	    free->status = FREE;
//  Prints message and exits if obj points to an already free chunk
    } else {
        printf("Attempt to free unallocated chunk\n");
        exit(1);
    }
	Heap.nFree++;
//  If freeList is empty, let the first element be the freed chunk
	if (Heap.nFree == 0) {
	    Heap.freeList[0] = free;
//  Otherwise, insert freed chunk to freeList and merge adjacent free chunks
	} else {	    
        int index = insertList(Heap.freeList, (void *)headerP);
        mergeFreeChunks(Heap.freeList, index);
	}
}

/** Return the first address beyond the range of the heap. */
static addr heapMaxAddr (void)
{
	return (addr) Heap.heapMem + Heap.heapSize;
}

/** Convert a pointer to an offset in the heap. */
int heapOffset (void *obj)
{
	addr objAddr = (addr) obj;
	addr heapMin = (addr) Heap.heapMem;
	addr heapMax =        heapMaxAddr ();
	if (obj == NULL || !(heapMin <= objAddr && objAddr < heapMax))
		return -1;
	else
		return (int) (objAddr - heapMin);
}

/** Dump the contents of the heap (for testing/debugging). */
void dumpHeap (void)
{
	int onRow = 0;

	// We iterate over the heap, chunk by chunk; we assume that the
	// first chunk is at the first location in the heap, and move along
	// by the size the chunk claims to be.
	addr curr = (addr) Heap.heapMem;
	while (curr < heapMaxAddr ()) {
	    
		header *chunk = (header *) curr;

		char stat;
		switch (chunk->status) {
		case FREE:  stat = 'F'; break;
		case ALLOC: stat = 'A'; break;
		default:
			fprintf (
				stderr,
				"myHeap: corrupted heap: chunk status %08x\n",
				chunk->status
			);
			exit (1);
		}

		printf (
			"+%05d (%c,%5d)%c",
			heapOffset ((void *) curr),
			stat, chunk->size,
			(++onRow % 5 == 0) ? '\n' : ' '
		);

		curr += chunk->size;
		
	}
	if (onRow % 5 > 0) {
		printf("\n");
	}
}
/*
Delete element from freeList
p is a pointer to the chunk's header which is now set as ALLOC
*/
static void deleteList(void **freelist, void *p) {
    int i = 0;   
    while ((addr)freelist[i] != (addr)p) {
        i++;
    }
    while (i < Heap.nFree) {
        freelist[i] = freelist[i+1];
        i++;
    }
    Heap.nFree--;
}
/* 
Insert element in freeList in order, returns index where it was placed
*/
static int insertList(void **freelist, void *obj)
{
    void *prev;
    void *curr;
    int i = 0;
    
    // Find index where obj was inserted
    while ((addr)obj > (addr)freelist[i]) {
        i++;
    }
    int index = i;
    // Place the chunk in freeList at i
    prev = freelist[i];
    freelist[i] = obj;
    i++;
    Heap.nFree++;
    for (; i < Heap.nFree; i++) {
        curr = freelist[i];
        freelist[i] = prev;
        prev = curr;
    }
    return index;
}
/*
Merges free chunks in freeList from left to right. By checking and merging
free chunks from left to right, order and pointer of the chunk is conserved.
It takes an argument, index, at which the last element was inserted and 
merges adjacent free chunks at the index.
*/
static void mergeFreeChunks(void **freelist, int index) {
    addr currP;
    addr nextP;
//  Case when obj was inserted as first element, check next element
    if (index == 0) {
        header *curr = (header *)freelist[index];
        header *next = (header *)freelist[index+1];
        currP = (addr)freelist[index];
        nextP = (addr)freelist[index+1];
    //  Check if the next element of freelist is free (connected), merge if so
        if (nextP == currP + curr->size) {
            curr->size = curr->size + next->size;
            deleteList(freelist, (void *)nextP);
        //  Zero out/reset region of memory
            memset((void *)nextP, 0, next->size);
        }
//  Case when obj is inserted as last element, check previous element
    } else if (index == Heap.nFree - 1) {
        currP = (addr)freelist[index - 2];
        nextP = (addr)freelist[index - 1];
        header *curr = (header *)freelist[index - 2];
        header *next = (header *)freelist[index - 1];
    //  Check if previous element is free (connected), merge if so
        if (nextP == currP + curr->size) {
            curr->size = curr->size + next->size;
            deleteList(freelist, (void *)nextP);
            memset((void *)nextP, 0, next->size);
        }
//  Case index between first and last elements, check prev and next elements
    } else {
        addr prevP;
        prevP = (addr)freelist[index - 1];
        currP = (addr)freelist[index];
        nextP = (addr)freelist[index + 1];
        header *prev = (header *)prevP;
        header *curr = (header *)currP;
        header *next = (header *)nextP;
    //  Merge with next element in freeList if they are connected
        if (nextP ==  currP + curr->size) {
            curr->size = curr->size + next->size;
            deleteList(freelist, (void *)nextP);
            memset((void *)nextP, 0, next->size);
        }
    //  Merge with prev element in freeList if they are connected
        if (currP == prevP + prev->size) {
            prev->size = prev->size + curr->size;
            deleteList(freelist, (void *)currP);
            memset((void *)currP, 0, curr->size);
        }
    }   
}



