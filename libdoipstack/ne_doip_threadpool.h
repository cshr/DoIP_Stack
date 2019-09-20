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
/**
 * @file ne_doip_threadpool.h
 * @brief doip threadpool
 */

#ifndef NE_DOIP_THREADPOOL_H
#define NE_DOIP_THREADPOOL_H

typedef void (*ne_doip_func) (void *data, void *user_data);

typedef struct
{
    ne_doip_func func;
    void *user_data;
}ne_doip_threadpool_t;

typedef struct
{
    int id;
    void *data;
}ne_doip_task_t;

/**
 * @brief a function to execute in the threads of thread pool
 *
 * @param [in] ne_doip_func : function for callback
 * @param [in] user_data : user data that is handed over to
 * @param [in] max_threads : max number of threads to execute concurrently in thread pool
 * @param [in] balance : whether need load balance for thread pool
 * @param [in] time : idle time before a thread is terminated
 *
 * @attention This function creates a new thread pool.
 */
ne_doip_threadpool_t *ne_doip_threadpool_new(ne_doip_func func, void *user_data, int max_threads, int balance, int time);

/**
 * @brief ne_doip_threadpool_free
 *
 * @param [in] pool : a #ne_doip_threadpool_t
 * @param [in] immediate : should pool shut down immediately?
 * @param [in] wait : should the function wait for all tasks to be finished?
 *
 * @attention Frees all resources allocated for pool.
 */
void ne_doip_threadpool_free(ne_doip_threadpool_t *pool, int immediate, int wait);

/**
 * @brief ne_doip_threadpool_push:
 *
 * @param [in] pool : a #ne_doip_threadpool_t
 * @param [in] data : a new task for pool
 *
 * @return Returns the result wether task is pushed successfully
 *
 * @attention Inserts data into the list of tasks to be executed by pool.
 */
int ne_doip_threadpool_push(ne_doip_threadpool_t *pool, ne_doip_task_t *data);

/**
 * @brief ne_doip_threadpool_get_run_num:
 *
 * @param [in] pool : a #ne_doip_threadpool_t
 *
 * @return Returns the number of threads currently running in pool.
 *
 * @attention Return value : the number of threads currently running
 */
int ne_doip_threadpool_get_run_num(ne_doip_threadpool_t *pool);

/**
 * @brief ne_doip_threadpool_unprocessed:
 *
 * @param [in] pool : a #ne_doip_threadpool_t
 *
 * @return Returns the number of tasks still unprocessed in pool.
 *
 * @attention Return value: the number of unprocessed tasks
 */
int ne_doip_threadpool_unprocessed(ne_doip_threadpool_t *pool);

#endif // NE_DOIP_THREADPOOL_H
/* EOF */
