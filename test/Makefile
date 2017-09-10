SOURCE = $(wildcard *.c)
TARGET = $(SOURCE:.c=)

LIB = ../libl4alloc.so
CFLAGS = -Wall -c

%: %.c

%: %.o
	$(CC) $(LIB) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $^

all: $(TARGET)
	@echo "Done!"

clean:
	$(RM) -f *.o $(TARGET)

#.PHONY: all clean
