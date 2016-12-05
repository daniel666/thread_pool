CC=gcc

CFLAGS=-lpthread

all: thread_pool.c test_thread_pool.c thread_pool.h
	$(CC) -o test_thread_pool -g -std=c99 thread_pool.c test_thread_pool.c $(CFLAGS)


