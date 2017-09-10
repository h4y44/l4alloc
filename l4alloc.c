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
	P_DEBUG("calling constructor with p: %p\n", p);
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
	P_DEBUG("getting new block:\nval: %p\ntop: %p\n", val, heap_top);
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
		P_DEBUG("merge into a block of: %d bytes\n", \
				(void *)block - (void *)ptr + ptr->size);
#endif
		block->size = block->mem - (ptr->mem + ptr->size);
		block->state = MEM_FREE;
		block->next = ptr->next;
	}
	else if (block == heap_top) {
		while (ptr->state == MEM_FREE) {
			ptr = ptr->next;
		}
#ifdef __DEBUG__ 
		P_DEBUG("free from top to :%p\n", ptr);
		P_DEBUG("subtract heap_top by: %d bytes\n", \
				(void *)block - (void *)ptr + ptr->size);
#endif
		/*
		 * this will become garbage data, but since it mays contain important
		 * data, should we clean this?
		 */
		UPDATE_TOP(ptr->next);
	}
}

void *l4realloc(void *mem, size_t size) {
	block_t *block = mem + sizeof(block_t);
	block = get_free_block(size);
	return block->mem;
}
