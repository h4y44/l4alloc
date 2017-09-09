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
	$(LD) $(OBJECT) -o $(TARGET)
	@echo "Build done!"

clean:
	$(RM) -f $(OBJECT)
	@echo "Clean done!"

sweep:
	$(RM) -f $(OBJECT) $(TARGET)
	@echo "Sweep done!"
