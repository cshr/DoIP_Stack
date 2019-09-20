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
 * @file ne_doip_thread.h
 * @brief thread module
 */

#ifndef NE_DOIP_THREAD_H
#define NE_DOIP_THREAD_H

#include <pthread.h>

struct ne_doip_thread;

typedef void *(*ne_doip_thread_main_function)(struct ne_doip_thread *thread, void *arg);

typedef struct ne_doip_thread
{
    pthread_t thread;
    pid_t tid;
    char *name;
    void *arg;
    int quit_flag;
    pthread_attr_t attr;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_condattr_t condattr;
    ne_doip_thread_main_function main_fun;
}ne_doip_thread_t;


ne_doip_thread_t *ne_doip_thread_create(ne_doip_thread_main_function main_fun, void *arg, const char *name);
void ne_doip_thread_release(ne_doip_thread_t *thread);
const char * ne_doip_thread_get_name(ne_doip_thread_t *thread);
int ne_doip_thread_get_id(ne_doip_thread_t *thread);
int ne_doip_thread_wait(ne_doip_thread_t *thread);
int ne_doip_thread_wait_timeout(ne_doip_thread_t *thread, int timeout_ms);
int ne_doip_thread_notify(ne_doip_thread_t *thread);
int ne_doip_thread_join(ne_doip_thread_t *thread);
int ne_doip_thread_terminate(ne_doip_thread_t *thread);
int ne_doip_thread_stop(ne_doip_thread_t *thread);
int ne_doip_thread_checkquit(ne_doip_thread_t *thread);

typedef struct ne_doip_sync
{
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
}ne_doip_sync_t;


ne_doip_sync_t *ne_doip_sync_create();
void ne_doip_sync_start(ne_doip_sync_t *sync);
void ne_doip_sync_end(ne_doip_sync_t *sync);
int ne_doip_sync_start_try(ne_doip_sync_t *sync);
void ne_doip_sync_destroy(ne_doip_sync_t *sync);

#endif // NE_DOIP_THREAD_H
/* EOF */
