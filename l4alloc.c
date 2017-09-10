#include "l4alloc.h"
/*
 * constructor of the lib
 */
void l4constructor() {
	/*
	 * create the tail
	 */
	pthread_mutex_lock(&l4_lock);
	void *ptr = sbrk(sizeof(block_t));
	pthread_mutex_unlock(&l4_lock);

	ptr += sizeof(block_t);
	heap_top = (block_t *)ptr;
	
	heap_top->state = MEM_FIRST;
	heap_top->size = 0;
	heap_top->next = NULL;
#ifdef __DEBUG__
	P_DEBUG("calling constructor with ptr: %p\n", ptr);
#endif
}

/*
 * return a new block which is head of the linked list
 * loop through the whole linked list to find if there is
 * a free block which has size less than or equal to 
 * required size
 */
static block_t *get_free_block(size_t size) {
	block_t *ptr = heap_top;
	while (ptr->next != NULL) {
		if ((ptr->size <= size) && (ptr->state == MEM_FREE)) {
			/*
			 * if the freeblock is much larger than the required size
			 * then we have to create a new block and set it to MEM_FREE,
			 * then append it to the linked list
			 * (inner allocation, causes memory fragment)
			 */
			int gap = ptr->size - (size + sizeof(block_t));
			if (gap > GAP_VAL) {
				/*
				 * enough space, let's create a new block
				 */
				block_t *new = (block_t *)(ptr->mem + ptr->size);
				//and set it to MEM_FREE
				new->mem = (void *)new + sizeof(block_t);
				new->size = gap;
				new->state = MEM_FREE;
				new->next = ptr->next;

				//set up current block, point it to new block and return
				ptr->size = size;
				ptr->state = MEM_USED;
				ptr->next = new;

				return ptr;
			}
			/*
			 * the rest free space is less than or equal to GAP_VAL, 
			 * just leave it that way
			 */
			ptr->state = MEM_USED;
			return ptr;
		}
		ptr = ptr->next;
	}
	/*
	 * no free block in linked list, let's create new
	 */
	pthread_mutex_lock(&l4_lock);
	void *val = sbrk(sizeof(block_t) + size);
	pthread_mutex_unlock(&l4_lock);
	/*
	 * fail to alloc, do nothing but returning NULL
	 */
	if (val == (void*)-1) {
		return NULL;
	}
	/*
	 * these values are expected to be equal to each others
	 */
#ifdef __DEBUG__ 
	P_DEBUG("getting new block:\n\tval: %p\n\ttop: %p\n", val, heap_top);
#endif
	val += sizeof(block_t) + size;
	ptr = (block_t *)val;
	/*
	 * set up new block
	 */
	ptr->size = size;
	ptr->state = MEM_USED;
	ptr->next = heap_top;
	ptr->mem = val - sizeof(block_t);

	/*
	 * update heap top
	 */
	UPDATE_TOP(val);
	return ptr;
}

void *l4malloc(size_t size) {
	block_t *p = get_free_block(size);
	return p->mem;
}

/*
 * free
 */
void l4free(void *mem) {
	block_t *block = mem + sizeof(block_t);
	block_t *ptr = block;
	//the block is not the head
	if (block != heap_top) {
		while (ptr->state == MEM_FREE) {
			ptr = ptr->next;
		}
		/*
		 * merge free blocks in one (inner free, defragment)
		 */
#ifdef __DEBUG__ 
		P_DEBUG("inner free from: %p to %p\n", block, ptr);
		//raw size of the merged block
		P_DEBUG("merge into a block of: %ld bytes\n", (void *)block - (void *)ptr + ptr->size);
#endif
		block->size = block->mem - (ptr->mem + ptr->size);
		block->state = MEM_FREE;
		block->next = ptr->next;
	}
	else if (block == heap_top) {
		while (ptr->state == MEM_FREE) {
			ptr = ptr->next;
		}

		size_t free_size = (void *)block - (void *)ptr + ptr->size;
#ifdef __DEBUG__ 
		P_DEBUG("free from top to :%p\n", ptr);
		P_DEBUG("subtract heap_top by: %ld bytes\n", free_size);
#endif
		pthread_mutex_lock(&l4_lock);
		sbrk(-free_size);
		pthread_mutex_unlock(&l4_lock);
		/*
		 * this will become garbage data, but since it mays contain important
		 * data, should we clean this?
		 */
		UPDATE_TOP(ptr->next);
	}
	/*
	 * the pointer is not belong to the linked list, aka
	 * was not returned by l4alloc, l4realloc or l4calloc
	 * do nothing
	 */
}

/*
 * realloc 
 * - if size = 0, call l4free() and return NULL
 * - if mem = NULL, call l4malloc()
 * current scheme is to check for the current block size, if the required size
 * is larger than the current block size, create a new block, copy data from 
 * old block to it and set old block state to MEM_FREE
 *
 * if the required size is less than the current block size, we will 
 * decide to create a new free block base on the GAP_VAL
 */
void *l4realloc(void *mem, size_t size) {
	if (size == 0) {
		l4free(mem);
		return NULL;
	}
	if (mem == NULL) {
		block_t *p = get_free_block(size);
		return p->mem;
	}
	block_t *ptr = (block_t *)(mem + sizeof(block_t));

	if (size > ptr->size) {
		//set current block to MEM_FREE
		ptr->state = MEM_FREE;

		//find a suitable block, or create new block
		block_t *block = get_free_block(size);
		/*
		 * ERROR checking?
		 * use memcpy
		 */
		memcpy(block->mem, ptr->mem, ptr->size);
#ifdef __DEBUG__ 
		P_DEBUG("size > ptr->size\n\tblock: %p\n\tptr: %p\n", block, ptr);
#endif
		return block->mem;
	}
	else {
		size_t gap = ptr->size - (size + sizeof(block_t));
		if (gap > GAP_VAL) {
			//let's create a new block here
			block_t *block = (block_t *)(mem+size);

			block->mem = mem+size+sizeof(block_t);
			block->state = MEM_FREE;
			block->size = gap;
			block->next = ptr->next;

			//set up current block
			ptr->size = size;
			ptr->next = block;
#ifdef __DEBUG__ 
			P_DEBUG("size <= ptr->size\n\tblock: %p\n\tptr: %p\n", block, ptr);
#endif

			return ptr;
		}
		else {
			//no need to create a new one
#ifdef __DEBUG__ 
			P_DEBUG("size <= ptr->size and the gap: %lu\n", gap);
#endif
			return mem;
		}
	}
}

void *l4calloc(size_t size) {
	block_t *p = get_free_block(size);
	/*
	 * set all that memory to zero
	 */
#ifdef __DEBUG__ 
	P_DEBUG("calling memset at %p\n", p->mem);
#endif
	memset(p->mem, 0, size);
	return p->mem;
}

#ifdef __DEBUG__ 
void print_trace(void *mem) {
	//the raw block
	block_t *ptr = mem + sizeof(block_t);

	while (ptr->state != MEM_FIRST) {
		puts("----begin block----");
		printf("raw @: %p\n", ptr);
		printf("mem @: %p\n", ptr->mem);
		printf("size : %ld\n", ptr->size);
		printf("state: %d\n", ptr->state);
		puts("-----end block-----");
		puts("         |");
		puts("         V");
		
		//move to next block
		ptr = ptr->next;
	}
}
#endif
