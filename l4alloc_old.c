#include <unistd.h>
#include <string.h> /*mem set */
#include <errno.h>
#include <pthread.h>

#ifdef _DEBUG
#include <stdio.h> /* debug */

#define P_DEBUG(fmt, ...) \
	do { fprintf(stderr, "%s:%d:%s(): "fmt, __FILE__, \
			__LINE__, __func__, __VA_ARGS__);} while (0)
#endif
/*
 * head of the memory block linked list
 */
static void *global_top = NULL;
pthread_mutex_t global_mutex;
#define UPDATE_HEAP_TOP(pointer) \
	global_top = pointer;

/*
 * A `block` of memory consists of a header which keeps track of the whole block
 * and a real block that contains actual free memory, ready to be written
 */
struct head_t {
	unsigned int free;
	size_t size;
	struct head_t *next;
};

typedef struct head_t head_t;
void *get_free_block(size_t size);
void *mealloc(size_t size);
void *mecalloc(size_t value, size_t size);
void *merealloc(void *ptr, size_t size);
void *mefree(void *ptr);

void *mealloc(size_t size) {
	int head_size = sizeof(head_t);
	void *new = get_free_block(size);
#ifdef _DEBUG
	P_DEBUG("new block at %p\n", new-head_size);
#endif
	return new-head_size;
}

void *mecalloc(size_t value, size_t size) {
	void *new = get_free_block(size);
	memset(new - sizeof(head_t), value, size);
#ifdef _DEBUG
	P_DEBUG("new block at %p\n", new);
#endif
	return (new - sizeof(head_t));
}

void *merealloc(void *ptr, size_t size) {
	/*
	 * 2 cases
	 * if the new size is less than the current size, the difference is less than 
	 * or equal to sizeof(head_t), do nothing
	 * else create a new block, set free = 1
	 *
	 * if the new size is bigger than the current size, set the free status of current
	 * block = 1 and alloc new block on top of the heap
	 */
	head_t *head_curr = (head_t *)(ptr + sizeof(head_t));
#ifdef _DEBUG
	P_DEBUG("calling realloc on %p\n \
			seek to its top: %p\n", ptr, head_curr);
#endif
	if (size <= head_curr->size) {
#ifdef _DEBUG
		P_DEBUG("size of current block %ld\n", head_curr->size);
#endif
		/*
		 * check size and set up new block
		 * lazy, do nothing
		 */
#ifdef _DEBUG
		P_DEBUG("%ld is less than %ld, do nothing\n", size, head_curr->size);
#endif
		head_curr-> size = size;
		return ptr;
	}
	/*
	 * need more memory, alloc new block, from the top of heap
	 */
	head_curr->free = 1;
	void *head_new = get_free_block(size);
#ifdef _DEBUG
	P_DEBUG("new block at %p\n", head_new);
#endif
	memcpy(head_new - sizeof(head_t), ptr, head_curr->size);
	return (head_new - sizeof(head_t));
}

void free(void *ptr) {
	/*
	 * 2 cases
	 * if the block is on top of heap, just free it and set global_top to the next block
	 * if the block is somewhere else in the linked list, remove it from the linked list
	 * merge free blocks if there are 2 or more stand nearby
	 */
	head_t *head_curr = (head_t *)(ptr + sizeof(head_t));
	if (head_curr == global_top) {
		/*
		 * free heap top block, subtract program segment by size of that block
		 */
		pthread_mutex_lock(&global_mutex);
		sbrk(head_curr->size + sizeof(head_t));
		pthread_mutex_unlock(&global_mutex);
		UPDATE_HEAP_TOP(head_curr - head_curr->size - sizeof(head_t));
#ifdef _DEBUG
		P_DEBUG("free from heap top: %p\n", head_curr);
#endif
	}
	/*
	 * normal case, set free = 1
	 */
	head_curr->free = 1;
#ifdef _DEBUG
	printf("[free] from the middle of the heap\n");
#endif
}

/*
 * get a free block of memory with `size`, actually it creates a block
 * consists of a head and a block of free memory, and returns pointer to that
 * large block
 */
void *get_free_block(size_t size) {
	void *brk_curr = NULL;
	head_t *heap_ptr;
	/*
	 * check if the head is NULL, that means the heap hasn't allocated yet
	 */
	if (global_top == NULL) {
		//lock the mutex
		pthread_mutex_lock(&global_mutex);
		brk_curr = sbrk(0);
#ifdef _DEBUG
		P_DEBUG("heap starts at %p\n", brk_curr);
#endif
		/*
		 * create the first block
		 */
		int block_size = size + sizeof(head_t);
		//invoke the sbrk call
		void *old = sbrk(block_size);
		pthread_mutex_unlock(&global_mutex);

		if (old == (void *)-1) {
			return NULL;
		}
		/*
		 * create head block
		 */
		brk_curr += block_size;
		heap_ptr = (head_t *)brk_curr;
	
		heap_ptr->free = 0;
		heap_ptr->size = size;
		/*
		 * first block
		 */
		heap_ptr->next = NULL;
		UPDATE_HEAP_TOP(brk_curr);
#ifdef _DEBUG
		P_DEBUG("current heap top %p\n", global_top);
#endif
		return brk_curr;
	}
	/*
	 * first block exists, continue alloc from it
	 */
	brk_curr = global_top;
#ifdef _DEBUG
	P_DEBUG("old block at %p\n", brk_curr);
#endif

	pthread_mutex_lock(&global_mutex);
	int block_size = size + sizeof(head_t);
	// invoke sbrk syscall
	void *old = sbrk(block_size);
	pthread_mutex_unlock(&global_mutex);
	//on fail
	if (old == (void *)-1) {
		return NULL;
	}

	brk_curr += block_size;
	heap_ptr = (head_t *)brk_curr;
	/*
	 * set up new head
	 */
	heap_ptr->size = size;
	heap_ptr->free = 0;
	heap_ptr->next = old;

	UPDATE_HEAP_TOP(brk_curr);
#ifdef _DEBUG
	P_DEBUG("current heap top %p\n", global_top);
#endif

	/*
	 * this function always returns the raw pointer of a block
	 */
	return brk_curr;
}

/*
 * entrance point to test
 */
int main() {
#ifdef _DEBUG
	printf("size of head_t %ld\n", sizeof(head_t));
	puts("Alloc 20 bytes");
#endif
	char *s = mealloc(20);
	
	if (!s) {
		puts("malloc failed!");
		return 1;
	}
	printf("s: %p\n", s);
	char *ss = merealloc(s, 10);

	if (!ss) {
		puts("realloc failed!");
		return 1;
	}
	printf("%s\n", s);
	return 0;
}
