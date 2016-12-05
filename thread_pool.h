#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H
#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include <pthread.h>

typedef enum{READY, WORKING, IDLE} status_t;

typedef struct _task_t{
    unsigned int id;
    void (*f)(void *);
    void *arg;
    struct _task_t *next;
} task_t;

typedef struct _block_task_queue{
    task_t *head;
    int len;
    pthread_cond_t *wait_job;
    pthread_mutex_t *lock;
} block_queue_t;


typedef struct _thread{
    pthread_t *pthread;
    status_t status;
    struct _thread_pool *tp;
    unsigned int pid;
} thread_t;

typedef struct _thread_pool{
    thread_t** threads;
    block_queue_t *taskqueue;
    int num_threads;
    int num_ready_threads;
    int num_working_threads;

    pthread_mutex_t *tp_cnt_lock;
    pthread_cond_t *wait_all_idle;

    bool volatile keepactive;

} thread_pool_t;


int block_queue_init(block_queue_t* queue);
int block_queue_clear(block_queue_t *queue);
int block_queue_add_head(block_queue_t *tq, task_t *task);
task_t *block_queue_remove_head(block_queue_t *tq);

int thread_pool_add_task(thread_pool_t *tp, task_t *task);

void thread_pool_do_task( void *ptr);

int thread_pool_wait(thread_pool_t *tp);

int thread_pool_clear(thread_pool_t *tp);
thread_pool_t *thread_pool_init(int num_threads);

int thread_init(int id, thread_t *thread, thread_pool_t *tp);

int thread_clear(thread_t *thread);
#endif
