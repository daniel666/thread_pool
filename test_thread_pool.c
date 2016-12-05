#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include "thread_pool.h"
#include<pthread.h>

void task1(){
    printf("Thread #%u working on task1\n", (unsigned int) pthread_self());
}

void task2(){
    printf("Thread #%u working on task2\n", (unsigned int) pthread_self());
}



int main(){
	puts("Making threadpool with 4 threads");
	thread_pool_t *thpool = thread_pool_init(4);

        int n = 6;
	printf("Adding %d tasks to threadpool\n", 2*n);
	int i;
        task_t *tsk = NULL;
	for (i=1; i<=n; i++){
            tsk = malloc(sizeof(*tsk));
            tsk->id = -i;
            tsk->f = task1;
            tsk->arg = NULL;
            tsk->next = NULL;
            thread_pool_add_task(thpool, tsk);

            tsk = malloc(sizeof(*tsk));
            tsk->id = i;
            tsk->f = task2;
            tsk->arg = NULL;
            tsk->next = NULL;
            thread_pool_add_task(thpool, tsk);
	}

        sleep(1);
        puts("Killing threadpool");
        thread_pool_clear(thpool);
        puts("done Killing threadpool");
	
	return 0;
}
