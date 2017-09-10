#include <stdio.h>
#include "../l4alloc.h"

int main() {
	char *s = l4malloc(100);
	strcpy(s, "Hello heap!");
	char *s2 = l4malloc(100);
	puts(s);
	print_trace(s);
	return 0;
}
