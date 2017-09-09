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
