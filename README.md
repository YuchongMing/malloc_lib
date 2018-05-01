# Malloc Library with Buddy System

The malloc library using buddy system to maintain a group of free lists to manage the memory heap. 
The buddy system supports seven APIs, including malloc, realloc, calloc, free, memalign, posix_memalign and reallocarray.
This malloc library is thread safty, and using per-core arena algorithm to maintain multi thread when allocating memory.

## Data Structure

### Schema

	   buddy_util.c
			|
			|
			|
		  buddy.c  ma_util.c
		  	|		  |						
		  	|		  |						
		  	|         |						
		  	|---------|					   	|----calloc.c
					  |		|----malloc.c---|----memalign.c
					  |		|				|----posixmemalign.c
					  |		|
					  |-----|----realloc.c---reallocarray.c
					     	|
					        |
					  		|----free.c
					  		

### Buddy System

The buddy system has two different kind of data structure. "f_header" is double linked list struct to maintain a couple of free lists, and "m_header" is the header of the allocated block, which contains a varible to store the block size. Both f_header and m_header have a pointer varible that point to themselves, which is used to check the validation of the header. The design idea of keeping two kinds of data structures is to reduce the header size.

The buddy system only handles the block whose size below half page size. If the requirement size bigger than half page size, the system calls mmap and munmap. 

### Per-Core Arena

The arena system has two different kind of data structure. Structure "arena" is used to store the infomation of an arena, including its id, used size and free size. Structure "pthread" is a linked list used to reference each thread to its own arena.

The number of arena is the same as the number of the processor that the computer have. If the number of the threads less than the number of the processor, each thread runs in its own arena. If the number of the threads greater than the number of the process, some threads will share the same arena. After a thread being assigned to an arena, that thread won't try to go to another arena.

## Getting Start

### Installing

Download from the website and unzip using the code
```
tar -xzf hw3.tar.gz
```

### Testing

To test the malloc library, please run
```
make check1
```

To clean the object file, please run
```
make clean
```

To debug the program, please run
```
make debug
```

## Author
* **Kevin Ming** *Feb, 18, 2018*
