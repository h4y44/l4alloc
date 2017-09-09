#include <unistd.h>
#include <stdio.h>

int main() {
	void *p1 = sbrk(0);
	void *p2 = sbrk(10);
	printf("%p\n%p\n", p1, p2);
	return 0;
}
