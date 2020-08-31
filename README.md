# Project: My basic implementation of malloc() and free()

# Summary
We want to manage a large region of memory as a Heap. Chunks of space in memory are allocated and freed.

The myMalloc() function will convert a free chunk to allocated.

The myFree() function will convert an allocated chunk to free.

We also want to maintain a list of free chunks, and merge adjacent free chunks.



All chunks should have allocate extra memory to fit in a header. The header contains some information on the chunk it belongs to such as
the size of the chunk.

Consecutive free chunks should be merged to form a single larger chunk.

When finding a spot in the heap to allocate a chunk, look for a chunk that fits the to-be allocated chunk best
(as small as possible that still has enough memory for the to-be allocated chunk).
