/*
 * This scheme is an experiment, and for learning, don't use it
 * in your software, really, because it's exploitable
 *
 * when a block is out of bound, data are overlapped and will be
 * written in the next block, overflow to the next pointer, so 
 * the program behavior is unpredictable and may be extremely 
 * dangerous to use
 */

#ifndef __L4ALLOC_H__ 
#define __L4ALLOC_H__ 

#ifdef __DEBUG__ 
#include <stdio.h>
#endif

#include <stddef.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

/*
 * gap value, increase this to decrease memory fragmentation, that 
 * means the more GAP_VAL is, the less small block you have in the 
 * linked list
 */
#define GAP_VAL 1000 // ~1KB for each block

#ifdef __DEBUG__ 
#define P_DEBUG(fmt, ...) \
	do { fprintf(stderr, "[%s:%d:%s] "fmt, __FILE__, \
			__LINE__, __func__, __VA_ARGS__); } while(0)
#endif

#define UPDATE_TOP(top_new) \
	do { heap_top = top_new; } while(0)

enum state {
	MEM_FREE,
	MEM_USED,
	MEM_FIRST
};

/*
 * a block of memory is a node in the linked list
 * the tail of linked list is heap bottom, which is an empty block
 * with state = MEM_FIRST
 */
typedef struct block_t {
	void *mem; //point to the real memory block
	size_t size;
	int state;

	struct block_t *next; //point to next block
} block_t;

//save the head of the linked list
static block_t *heap_top;
/*
 * mutex to advoid call simultaniously between threads
 * may be i will implement concurrent malloc someday
 */
pthread_mutex_t l4_lock;

/*
 * function prototypes
 */
void __attribute__((constructor)) l4constructor();

static block_t *get_free_block(size_t);

void *l4malloc(size_t);
void *l4calloc(size_t);
void *l4realloc(void *, size_t);
void l4free(void *);

#endif
