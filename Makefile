SOURCE = l4alloc.c 
OBJECT = l4alloc.o
TARGET = libl4alloc.so

CC = gcc 
LD = gcc 
RM = rm 

CFLAGS = -Wall -std=gnu99 -fPIC
LDFLAGS = -shared
DEBUG_CFLAGS = $(CFLAGS) -D__DEBUG__ -g

all:
	$(CC) $(CFLAGS) -c $(SOURCE) -o $(OBJECT)
	$(LD) $(LDFLAGS) $(OBJECT) -o $(TARGET)
	@echo "Build done!"

debug:
	$(CC) $(DEBUG_CFLAGS) -c $(SOURCE) -o $(OBJECT)
	$(LD) $(LDFLAGS) $(OBJECT) -o $(TARGET)
	@echo "Build done!"

conv: debug 
	$(CC) $(CFLAGS) -c l4alloc_conv.c -o l4alloc_conv.o
	$(CC) $(LDFLAGS) $(OBJECT) l4alloc_conv.o -o libl4alloc_conv.so

clean:
	$(RM) -f $(OBJECT)
	@echo "Clean done!"

sweep:
	$(RM) -f *.so *.o
	@echo "Sweep done!"
