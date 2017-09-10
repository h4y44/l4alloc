/*
 * convention calls to test in another programs
 */

#include "l4alloc_conv.h"

void *malloc(size_t size) {
	return l4malloc(size);
}

void *realloc(void *mem, size_t size) {
	return l4realloc(mem, size);
}

void *calloc(size_t size) {
	return l4calloc(size);
}

void free(void *mem) {
	l4free(mem);
}
