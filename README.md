## Description
* My implementation of memory allocator on *linux*, it consists of 
4 basic functions: `malloc`, `free`, `calloc` and `realloc`

* The basic idea is using a linked list to manage allocated memory,
each node contains the info about the block, following by a *real* memory block,
then invoke `sbrk` syscall to increment or decrement program segment

## Building
To build this, you will need a working GCC compiler and GNU Make
* To build the shared library
```
$ make
```
* To build the shared library with debugging support
```
$ make debug
```
* To build the shared library with debugging support and calling conventions lib
```
$ make conv
```
## Known bugs
* When a block reachs to its end, data may overlap to the next block, causing
linked list corruption, and program behaviour is unpredictable!

## Todo
- [ ] Using `mmap` on large blocks
- [ ] Memory pools
- [ ] More test cases
- [ ] Being able to replace glibc's malloc in some cases

## License
MIT
