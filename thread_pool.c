#define _GNU_SOURCE

#include "thread_pool.h"

int block_queue_init(block_queue_t* queue){
    queue->head = NULL;
    queue->len = 0;
    if(queue->lock == NULL)
        queue->lock = malloc(sizeof(*queue->lock));
    if(queue->wait_job == NULL)
        queue->wait_job = malloc(sizeof(*queue->wait_job));
    pthread_mutex_init(queue->lock, NULL);
    pthread_cond_init(queue->wait_job, NULL);
    return 0;
}

int block_queue_clear(block_queue_t *queue){
    pthread_mutex_lock(queue->lock);
    while(queue->len != 0){
        printf("queue->len:%d, queue->head:%p\n", queue->len, queue->head);
        pthread_mutex_unlock(queue->lock);
        free(block_queue_remove_head(queue));
    }
    free(queue->wait_job);
    free(queue->lock);
    free(queue);
    return 0;
}

int block_queue_add_head(block_queue_t *tq, task_t *task){
    pthread_mutex_lock(tq->lock);
    task->next = tq->head;
    tq->head = task;
    tq->len++;
    pthread_cond_signal(tq->wait_job);
    pthread_mutex_unlock(tq->lock);
    return 0;
}

task_t *block_queue_remove_head(block_queue_t *tq){
    task_t *ret = NULL;

    pthread_mutex_lock(tq->lock);
    while(tq->len == 0)
        pthread_cond_wait(tq->wait_job, tq->lock);
    ret = tq->head;
    if (NULL == ret){
        pthread_mutex_unlock(tq->lock);
        return NULL;
    }
    tq->len -= 1;
    tq->head = tq->head->next;
    pthread_mutex_unlock(tq->lock);
    return ret;
}

thread_pool_t *thread_pool_init(int num_threads){
    thread_pool_t *threadpool = NULL;
    threadpool = malloc(sizeof(*threadpool));
    threadpool->num_threads = num_threads;
    threadpool->num_ready_threads = 0;
    threadpool->num_working_threads = 0;
    threadpool->keepactive = true;
    threadpool->threads = malloc(sizeof(thread_t *) * num_threads);
    threadpool->tp_cnt_lock = malloc(sizeof(*threadpool->tp_cnt_lock));

    threadpool->wait_all_idle = malloc(sizeof(*threadpool->wait_all_idle));

    pthread_mutex_init(threadpool->tp_cnt_lock, NULL);
    pthread_cond_init(threadpool->wait_all_idle, NULL);

    threadpool->taskqueue = malloc(sizeof(*threadpool->taskqueue));
    block_queue_init( threadpool->taskqueue);

    for(int i = 0; i < num_threads; i++){
        threadpool->threads[i] = malloc(sizeof(thread_t));
        thread_init(i+1, threadpool->threads[i], threadpool);
    }


    return threadpool;
}

int thread_pool_add_task(thread_pool_t *tp, task_t *task){
    return block_queue_add_head(tp->taskqueue, task);
}

void thread_pool_do_task( void *ptr){
    thread_t *thread = (thread_t *) ptr;

    char thread_name[128] = {0};

    sprintf(thread_name, "thread-pool-id:%d", thread->pid);
    pthread_setname_np(*thread->pthread, thread_name);

    thread_pool_t *tp = thread->tp;
    block_queue_t *taskqueue = tp->taskqueue;

    
    pthread_mutex_lock(tp->tp_cnt_lock);
    tp->num_ready_threads += 1;
    pthread_mutex_unlock(tp->tp_cnt_lock);

    task_t *job = NULL;
    while(tp->keepactive){
        job  = block_queue_remove_head(taskqueue);
        if(!tp->keepactive) break;
        if(NULL == job || NULL == job->f){
            free(job);
            continue;
        } 
        thread->status = WORKING;
        pthread_mutex_lock(tp->tp_cnt_lock);
        tp->num_working_threads += 1;
        pthread_mutex_unlock(tp->tp_cnt_lock);

        job->f(job->arg);
        free(job);

        thread->status = READY;
        pthread_mutex_lock(tp->tp_cnt_lock);
        tp->num_working_threads -= 1;
        if(tp->num_working_threads == 0){
            pthread_cond_signal(tp->wait_all_idle);
        }
        pthread_mutex_unlock(tp->tp_cnt_lock);
    }
    pthread_mutex_lock(tp->tp_cnt_lock);
    tp->num_ready_threads -= 1;
    pthread_mutex_unlock(tp->tp_cnt_lock);
}

int thread_pool_wait(thread_pool_t *tp){
    pthread_mutex_lock(tp->tp_cnt_lock);
    while(tp->num_working_threads){
        pthread_cond_wait(tp->wait_all_idle, tp->tp_cnt_lock);
    }
    pthread_mutex_unlock(tp->tp_cnt_lock);
    return 0;
}

int thread_pool_clear(thread_pool_t *tp){
    tp->keepactive = false;

    pthread_mutex_lock(tp->tp_cnt_lock);
    while(tp->num_ready_threads){
        printf("num ready threads:%d\n", tp->num_ready_threads);
        pthread_mutex_unlock(tp->tp_cnt_lock);
        /*add dummy task to let wait on threads to check keepactive and break out of loop*/
        task_t *dummy = malloc(sizeof(*dummy));
        block_queue_add_head(tp->taskqueue, dummy);
    }
    printf("block queue clear\n");
    block_queue_clear(tp->taskqueue);
    for(int i = 0; i < tp->num_threads; i++){
        printf("clear threads:%d\n", i);
        thread_clear(tp->threads[i]);
    }
    free(tp->threads);

    free(tp->tp_cnt_lock);
    free(tp->wait_all_idle);
    free(tp);
    return 0;
}


int thread_init(int id, thread_t *thread, thread_pool_t *tp){
    thread->pid = id;
    thread->tp = tp;
    thread->status = READY;
    thread->pthread = malloc(sizeof(pthread_t ));
    pthread_create(thread->pthread, NULL, (void *)thread_pool_do_task, (void *) thread);
    pthread_detach(*(thread->pthread));

    return 0;
}

int thread_clear(thread_t *thread){
    free(thread->pthread);
    free(thread);
}
