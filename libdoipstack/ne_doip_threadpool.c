/**
 * Copyright @ 2019 iAuto (Shanghai) Co., Ltd.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are NOT permitted except as agreed by
 * iAuto (Shanghai) Co., Ltd.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "ne_doip_threadpool.h"
#include "ne_doip_util.h"
#include "ne_doip_thread.h"

#define NE_DOIP_MAX_IDLE_TIME   15 * 1000;
#define NE_DOIP_IDLIST_SIZE     256
#define NE_DOIP_MAX_QUEUE_CNT   1000

/// define one task of list
typedef struct
{
    ne_doip_list_t base;    ///< basic list info
    void *data;             ///< task info
    uint8_t id;             ///< task id
}ne_doip_async_t;

/// define task queue
typedef struct
{
    pthread_mutex_t lock;                   ///< mutex for multithread
    pthread_cond_t cond;                    ///< cond for multithread
    pthread_condattr_t condattr;            ///< cond attribute
    ne_doip_list_t queue;                   ///< task list
    int waiting_thread;                     ///< number of waiting_thread
    int idlistrun[NE_DOIP_IDLIST_SIZE];     ///< array for group id of running task
    int taskcount;                          ///< task count in queue
}ne_doip_async_queue_t;

/// real thread pool info
typedef struct
{
    ne_doip_threadpool_t pool;        ///< basic pool info
    ne_doip_async_queue_t *queue;     ///< task queue
    pthread_cond_t cond;              ///< cond for multithread
    int max_threads;                  ///< max number of threads in pool
    int num_threads;                  ///< number of running threads in pool
    int running;                      ///< whether thread pool is running
    int immediate;                    ///< close thread pool imediately or not
    int waiting;                      ///< wait for all threads in pool over or not
    int load_balance;                 ///< use load balance to close idle thread
    int idle_time;                    ///< idle time of thread
}ne_doip_threadpool_r_t;

ne_doip_async_queue_t *ne_doip_queue_new();
void ne_doip_queue_destroy(ne_doip_async_queue_t *queue);
void ne_doip_queue_lock(ne_doip_async_queue_t *queue);
void ne_doip_queue_unlock(ne_doip_async_queue_t *queue);
int ne_doip_queue_length(ne_doip_async_queue_t *queue);
int ne_doip_queue_length_unlock(ne_doip_async_queue_t *queue);
int ne_doip_queue_gr_length(ne_doip_async_queue_t *queue);
int ne_doip_queue_gr_length_unlock(ne_doip_async_queue_t *queue);
int ne_doip_queue_id_in_pool(ne_doip_async_queue_t *queue, int id);
void ne_doip_queue_push(ne_doip_async_queue_t *queue, ne_doip_task_t *data);
void ne_doip_queue_push_unlocked(ne_doip_async_queue_t *queue, ne_doip_task_t *data);
ne_doip_task_t *ne_doip_queue_pop(ne_doip_async_queue_t *queue);
ne_doip_task_t *ne_doip_queue_pop_unlock(ne_doip_async_queue_t *queue);
ne_doip_task_t *ne_doip_queue_timeout_pop(ne_doip_async_queue_t *queue, int timeout);
ne_doip_task_t *ne_doip_queue_timeout_pop_unlock(ne_doip_async_queue_t *queue, int timeout);
ne_doip_async_t *ne_doip_queue_valid_task(ne_doip_async_queue_t *queue);


static ne_doip_task_t *
ne_doip_threadpool_wait_for_task(ne_doip_threadpool_r_t *pool)
{
    ne_doip_task_t *task = NULL;

    if (pool->running <= 0 && (pool->immediate > 0
        || ne_doip_queue_gr_length_unlock(pool->queue) <= 0)) {
        NE_DOIP_PRINT("thread:[%ld] in pool:[%p],stop running!\n", pthread_self(), pool);
        return task;
    }

    if (pool->num_threads > pool->max_threads) {
        NE_DOIP_PRINT("thread:[%ld] in pool:[%p],superfluous!\n", pthread_self(), pool);
        return task;
    }

    if (pool->load_balance > 0) {
        NE_DOIP_PRINT ("load_balance thread:[%ld], pool:[%p], running:[%d], unprocessed:[%d]\n",
                      pthread_self(), pool, pool->num_threads, ne_doip_queue_gr_length_unlock(pool->queue));
        task = ne_doip_queue_timeout_pop_unlock(pool->queue, pool->idle_time);
    }
    else {
        NE_DOIP_PRINT ("fixed thread:[%ld], pool:[%p], running:[%d], unprocessed:[%d]\n",
                      pthread_self(), pool, pool->num_threads, ne_doip_queue_gr_length_unlock(pool->queue));
        task = ne_doip_queue_pop_unlock(pool->queue);
    }

    return task;
}

static void
ne_doip_threadpool_in_free(ne_doip_threadpool_r_t *pool)
{
    if (pool == NULL) {
        NE_DOIP_PRINT("ne_doip_threadpool_in_free pool == NULL\n");
        return;
    }

    if (pool->running > 0) {
        NE_DOIP_PRINT("ne_doip_threadpool_in_free running>0\n");
        return;
    }

    if (pool->num_threads > 0) {
        NE_DOIP_PRINT("ne_doip_threadpool_in_free num_threads>0\n");
        return;
    }

    ne_doip_queue_destroy(pool->queue);
    pthread_cond_destroy(&pool->cond);

    free(pool);
}

static void
ne_doip_threadpool_wakeup_stop(ne_doip_threadpool_r_t *pool)
{
    NE_DOIP_PRINT("ne_doip_threadpool_wakeup_stop enter, num_threads:[%d]\n", pool->num_threads);

    pool->immediate = 1;
    int i;
    for (i = 0; i < pool->num_threads; i++) {
        ne_doip_task_t *task = malloc(sizeof *task);
        memset(task, 0, sizeof *task);
        task->id = 0;
        task->data = (void *)1;
        ne_doip_queue_push_unlocked(pool->queue, task);
    }
}

static void *
ne_doip_thread_run(ne_doip_thread_t *thread, void *arg)
{
    if (arg == NULL) {
        return NULL;
    }

    ne_doip_threadpool_r_t *pool = arg;
//    NE_DOIP_PRINT("thread:[%d] run for pool:[%p]\n", pthread_self(), pool);

    ne_doip_queue_lock(pool->queue);

    while (1) {
        ne_doip_task_t *task = ne_doip_threadpool_wait_for_task(pool);
        if (task) {

            if (pool->running || pool->immediate <= 0) {
                if (task->id) {
                    pool->queue->idlistrun[task->id] = 1;
                }

                ne_doip_queue_unlock(pool->queue);
                NE_DOIP_PRINT("thread:[%ld] run for pool:[%p] call fun!\n", pthread_self(), pool);
                pool->pool.func(task, pool->pool.user_data);

                ne_doip_queue_lock(pool->queue);
                pool->queue->idlistrun[task->id] = 0;
                if (task->id && pool->running <= 0) {
                    pthread_cond_signal(&pool->queue->cond);
                }
            }
            pool->queue->taskcount--;
            free(task);
            continue;
        }

        NE_DOIP_PRINT("thread:[%ld] run for pool:[%p] exit enter!\n", pthread_self(), pool);

        int free_pool = 0;
        pool->num_threads--;

        if (pool->running <= 0) {
            if (pool->waiting <= 0) {
                if (pool->num_threads == 0) {
                    free_pool = 1;
                }
                else {
                    if (ne_doip_queue_gr_length_unlock(pool->queue) == -pool->num_threads) {
                        ne_doip_threadpool_wakeup_stop(pool);
                    }
                }
            }
            else if (pool->immediate > 0 || ne_doip_queue_gr_length_unlock(pool->queue) <= 0) {
                pthread_cond_broadcast(&pool->cond);
            }
        }

        ne_doip_queue_unlock(pool->queue);

        if (free_pool > 0) {
            NE_DOIP_PRINT("thread:[%ld] run for pool:[%p] free pool\n", pthread_self(), pool);
            ne_doip_threadpool_in_free(pool);
        }

        break;
    }

    NE_DOIP_PRINT("thread:[%ld] run for pool:[%p] exit end\n", pthread_self(), pool);
    ne_doip_thread_release(thread);

    return NULL;
}

static int
ne_doip_threadpool_start_thread(ne_doip_threadpool_r_t *pool)
{
    if (pool == NULL) {
        return -1;
    }

    if (pool->num_threads >= pool->max_threads) {
        return 0;
    }

    ne_doip_thread_t *thread = ne_doip_thread_create(ne_doip_thread_run, pool, "pool");
    if (thread == NULL) {
        return -1;
    }

    pool->num_threads++;

    return 0;
}

ne_doip_threadpool_t *
ne_doip_threadpool_new(ne_doip_func func, void *user_data, int max_threads, int balance, int time)
{
    ne_doip_threadpool_r_t *pool_r = NULL;

    if (func == NULL) {
        return NULL;
    }

    if (max_threads <= 0) {
        return NULL;
    }

    pool_r = malloc(sizeof *pool_r);
    pool_r->pool.func = func;
    pool_r->pool.user_data = user_data;
    pool_r->max_threads = max_threads;
    pool_r->num_threads = 0;
    pool_r->queue = ne_doip_queue_new();
    pool_r->running = 1;
    pool_r->immediate = 0;
    pool_r->waiting = 0;
    pool_r->load_balance = balance;
    pool_r->idle_time = time;
    memset(pool_r->queue->idlistrun, 0, NE_DOIP_IDLIST_SIZE * sizeof(int));
    pthread_cond_init(&pool_r->cond, NULL);

    if (pool_r->idle_time < 0) {
        pool_r->idle_time = NE_DOIP_MAX_IDLE_TIME;
    }

    if (pool_r->load_balance > 0) {
        return (ne_doip_threadpool_t *)pool_r;
    }

    ne_doip_queue_lock(pool_r->queue);

    while (pool_r->num_threads < pool_r->max_threads) {
        int ret = ne_doip_threadpool_start_thread(pool_r);
        if (ret == -1) {
            NE_DOIP_PRINT("start thread failed!\n");
            break;
        }
    }

    ne_doip_queue_unlock(pool_r->queue);

    return (ne_doip_threadpool_t *)pool_r;
}

void
ne_doip_threadpool_free(ne_doip_threadpool_t *pool, int immediate, int wait)
{
    if (pool == NULL) {
        return;
    }

    ne_doip_threadpool_r_t *pool_r = (ne_doip_threadpool_r_t *)pool;

    if (pool_r->running <= 0) {
        return;
    }

    ne_doip_queue_lock(pool_r->queue);

    pool_r->running = 0;
    pool_r->immediate = immediate;
    pool_r->waiting = wait;

    if (wait) {
        int queue_length = ne_doip_queue_gr_length_unlock(pool_r->queue);
        int num_threads = pool_r->num_threads;
        while (queue_length != -num_threads && !(immediate && num_threads == 0)) {
            NE_DOIP_PRINT("wait for thread end, queue_length:[%d], num_threads:[%d]\n", queue_length, num_threads);
            pthread_cond_wait(&pool_r->cond, &(pool_r->queue->lock));
            queue_length = ne_doip_queue_gr_length_unlock(pool_r->queue);
            num_threads = pool_r->num_threads;
        }
    }

    int queue_length = ne_doip_queue_gr_length_unlock(pool_r->queue);
    int num_threads = pool_r->num_threads;
    if (immediate || queue_length == -num_threads) {
        NE_DOIP_PRINT("immediate end, queue_length:[%d], num_threads:[%d]\n", queue_length, num_threads);
        if (pool_r->num_threads == 0) {
            ne_doip_queue_unlock(pool_r->queue);
            NE_DOIP_PRINT("ne_doip_threadpool_free free pool\n");
            ne_doip_threadpool_in_free(pool_r);
            return;
        }

        ne_doip_threadpool_wakeup_stop(pool_r);
    }

    pool_r->waiting = 0;
    ne_doip_queue_unlock(pool_r->queue);
}

int
ne_doip_threadpool_push(ne_doip_threadpool_t *pool, ne_doip_task_t *task)
{
    int ret = 0;

    if (pool == NULL) {
        return -1;
    }

    if (task == NULL) {
        return -1;
    }

    ne_doip_threadpool_r_t *pool_r = (ne_doip_threadpool_r_t *)pool;

    if (pool_r->running <= 0) {
        NE_DOIP_PRINT("pool is not running\n");
        return -1;
    }

    if ((task->id < 0) || (task->id > 255)) {
        NE_DOIP_PRINT("groupid is invalid\n");
        return -1;
    }

    ne_doip_queue_lock(pool_r->queue);

    int inpool = ne_doip_queue_id_in_pool(pool_r->queue, task->id);
    int queuelength = ne_doip_queue_gr_length_unlock(pool_r->queue);

    if (pool_r->queue->taskcount >= NE_DOIP_MAX_QUEUE_CNT) {
        NE_DOIP_PRINT("the queue is full \n");
        ne_doip_queue_unlock(pool_r->queue);
        return -1;
    }

    ++pool_r->queue->taskcount;

    if (!inpool && queuelength >= 0) {
        ret = ne_doip_threadpool_start_thread(pool_r);
        if (ret == -1) {
            NE_DOIP_PRINT("start thread failed when push task!\n");
        }
    }

    ne_doip_queue_push_unlocked(pool_r->queue, task);

    ne_doip_queue_unlock(pool_r->queue);

    return ret;
}

int
ne_doip_threadpool_get_run_num(ne_doip_threadpool_t *pool)
{
    int num = 0;

    if (pool == NULL) {
        return num;
    }

    ne_doip_threadpool_r_t *pool_r = (ne_doip_threadpool_r_t *)pool;

    if (pool_r->running <= 0) {
        return num;
    }

    ne_doip_queue_lock(pool_r->queue);

    num = pool_r->num_threads;

    ne_doip_queue_unlock(pool_r->queue);

    return num;
}

int
ne_doip_threadpool_unprocessed(ne_doip_threadpool_t *pool)
{
    int num = 0;

    if (pool == NULL) {
        return num;
    }

    ne_doip_threadpool_r_t *pool_r = (ne_doip_threadpool_r_t *)pool;

    if (pool_r->running <= 0) {
        return num;
    }

    // return total task numbers regardless of same group id or not
    num = ne_doip_queue_length(pool_r->queue);

    if (num < 0) {
        num = 0;
    }

    return num;
}

int
ne_doip_queue_id_in_pool(ne_doip_async_queue_t *queue, int id)
{
    if (id == 0) {
        return 0;
    }

    if (queue->idlistrun[id]) {
        return 1;
    }

    ne_doip_async_t *async;
    ne_doip_async_t *tmp;
    ne_doip_list_t *list = &queue->queue;
    ne_doip_list_for_each_safe(async, tmp, list, base) {
        if (async->id == id) {
            return 1;
        }
    }

    return 0;
}

// ********************************************************************
//  async queue function
// ********************************************************************

ne_doip_async_queue_t *
ne_doip_queue_new()
{
    ne_doip_async_queue_t *queue = NULL;
    queue = malloc(sizeof *queue);
    if (queue == NULL) {
        return queue;
    }

    memset(queue, 0, sizeof *queue);
    queue->waiting_thread = 0;

    pthread_condattr_init(&queue->condattr);
    pthread_condattr_setclock(&queue->condattr, CLOCK_MONOTONIC);
    pthread_cond_init(&queue->cond, &queue->condattr);
    pthread_mutex_init(&queue->lock, NULL);
    ne_doip_list_init(&queue->queue);

    return queue;
}

void
ne_doip_queue_destroy(ne_doip_async_queue_t *queue)
{
    if (queue == NULL) {
        NE_DOIP_PRINT("ne_doip_queue_destroy is NULL\n");
        return;
    }

    if (queue->waiting_thread != 0) {
        NE_DOIP_PRINT("ne_doip_queue_destroy waiting_thread != 0\n");
        return;
    }

    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->cond);
    pthread_condattr_destroy(&queue->condattr);

    ne_doip_async_t *async;
    ne_doip_async_t *tmp;
    ne_doip_list_t *list = &queue->queue;
    ne_doip_list_for_each_safe(async, tmp, list, base) {
        ne_doip_list_remove((ne_doip_list_t *)async);
        free(async);
    }

    free(queue);
}

void
ne_doip_queue_lock(ne_doip_async_queue_t *queue)
{
    if (queue == NULL) {
       NE_DOIP_PRINT("ne_doip_queue_lock queue == NULL!\n");
       return;
    }

    pthread_mutex_lock(&queue->lock);
}

void
ne_doip_queue_unlock(ne_doip_async_queue_t *queue)
{
    if (queue == NULL) {
       NE_DOIP_PRINT("ne_doip_queue_unlock queue == NULL!\n");
       return;
    }

    pthread_mutex_unlock(&queue->lock);
}

int
ne_doip_queue_length(ne_doip_async_queue_t *queue)
{
    int num = 0;

    if (queue == NULL) {
        return num;
    }

    pthread_mutex_lock(&queue->lock);

    num = ne_doip_queue_length_unlock(queue);

    pthread_mutex_unlock(&queue->lock);

    return num;
}

int
ne_doip_queue_length_unlock(ne_doip_async_queue_t *queue)
{
    int num = 0;

    if (queue == NULL) {
        return num;
    }

    int length = ne_doip_list_length(&queue->queue);
    num = length - queue->waiting_thread;

    return num;
}

int
ne_doip_queue_gr_length(ne_doip_async_queue_t *queue)
{
    int num = 0;

    if (queue == NULL) {
        return num;
    }

    pthread_mutex_lock(&queue->lock);

    num = ne_doip_queue_gr_length_unlock(queue);

    pthread_mutex_unlock(&queue->lock);

    return num;
}

int
ne_doip_queue_gr_length_unlock(ne_doip_async_queue_t *queue)
{
    int num = 0;

    if (queue == NULL) {
        return num;
    }

    int array[NE_DOIP_IDLIST_SIZE];
    memset(array, 0, NE_DOIP_IDLIST_SIZE * sizeof(int));

    ne_doip_async_t *async;
    ne_doip_async_t *tmp;
    ne_doip_list_t *list = &queue->queue;
    ne_doip_list_for_each_safe(async, tmp, list, base) {
        if (async->id == 0) {
            ++num;
            continue;
        }

        if (array[async->id] == 0) {
            ++num;
            array[async->id] = 1;
        }
    }

    return num - queue->waiting_thread;
}

void
ne_doip_queue_push(ne_doip_async_queue_t *queue, ne_doip_task_t *task)
{
    if (queue == NULL) {
        return;
    }

    pthread_mutex_lock(&queue->lock);
    ne_doip_queue_push_unlocked(queue, task);
    pthread_mutex_unlock(&queue->lock);
}

void
ne_doip_queue_push_unlocked(ne_doip_async_queue_t *queue, ne_doip_task_t *task)
{
    if (queue == NULL) {
        return;
    }

    ne_doip_async_t *node = malloc(sizeof *node);
    ne_doip_list_init((ne_doip_list_t *)node);
    node->id = task->id;
    node->data = task;
    ne_doip_list_insert(&queue->queue, (ne_doip_list_t *)node);

    if (queue->idlistrun[task->id]) {
        return;
    }

    if (queue->waiting_thread > 0) {
        pthread_cond_signal(&queue->cond);
    }
}


ne_doip_async_t *
ne_doip_queue_valid_task(ne_doip_async_queue_t *queue)
{
    ne_doip_async_t *node = NULL;

    ne_doip_async_t *async;
    ne_doip_async_t *tmp;
    ne_doip_list_t *list = &(queue->queue);

    ne_doip_list_for_each_reverse_safe(async, tmp, list, base) {
        if (queue->idlistrun[async->id] == 0) {
            node = async;
            break;
        }
    }

    return node;
}

static ne_doip_task_t *
ne_doip_queue_pop_in_unlock(ne_doip_async_queue_t *queue, int wait, int end_time)
{
    ne_doip_task_t *task = NULL;
    ne_doip_async_t *node = ne_doip_queue_valid_task(queue);

    if ((node == NULL) && wait) {
        queue->waiting_thread++;
        while (node == NULL) {
            if (end_time == -1) {
                pthread_cond_wait(&queue->cond, &queue->lock);
            }
            else {
                struct timespec nptime;
                ne_doip_get_timespec(&nptime, end_time);
                int ret = pthread_cond_timedwait(&queue->cond, &queue->lock, &nptime);
                if (ret) {
                    break;
                }
            }

            node = ne_doip_queue_valid_task(queue);
        }
        queue->waiting_thread--;
    }

    if (node) {
        ne_doip_list_remove((ne_doip_list_t *)node);
        task = node->data;
        free(node);
    }

    return task;
}

ne_doip_task_t *
ne_doip_queue_pop(ne_doip_async_queue_t *queue)
{
    ne_doip_task_t *task = NULL;

    pthread_mutex_lock(&queue->lock);
    task = ne_doip_queue_pop_unlock(queue);
    pthread_mutex_unlock(&queue->lock);

    return task;
}

ne_doip_task_t *
ne_doip_queue_pop_unlock(ne_doip_async_queue_t *queue)
{
    ne_doip_task_t *task = NULL;

    task = ne_doip_queue_pop_in_unlock(queue, 1, -1);

    return task;
}

ne_doip_task_t *
ne_doip_queue_timeout_pop(ne_doip_async_queue_t *queue, int timeout)
{
    ne_doip_task_t *task = NULL;

    pthread_mutex_lock(&queue->lock);
    task = ne_doip_queue_timeout_pop_unlock(queue, timeout);
    pthread_mutex_unlock(&queue->lock);

    return task;
}

ne_doip_task_t *
ne_doip_queue_timeout_pop_unlock(ne_doip_async_queue_t *queue, int timeout)
{
    ne_doip_task_t *task = NULL;

    task = ne_doip_queue_pop_in_unlock(queue, 1, timeout);

    return task;
}

