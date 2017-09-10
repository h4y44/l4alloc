#include "../l4alloc.h"
#include <stdio.h>

int main() {
	void *s = l4malloc(100);
	l4free(s);
	return 0;
}
