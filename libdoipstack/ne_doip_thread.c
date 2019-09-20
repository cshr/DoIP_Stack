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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ne_doip_thread.h"
#include "ne_doip_util.h"

static void *ne_doip_thread_main(void *arg);

ne_doip_thread_t *
ne_doip_thread_create(ne_doip_thread_main_function main_fun, void *arg, const char *name)
{
    int ret = -1;

    if (main_fun == NULL || name == NULL) {
        return NULL;
    }

    ne_doip_thread_t *thread;
    thread = malloc(sizeof *thread);

    thread->quit_flag = 0;
    thread->arg = arg;
    thread->main_fun = main_fun;
    thread->thread = -1;
    thread->tid = -1;

    int str_size = strlen(name) + 1;
    thread->name = malloc(str_size);
    memset(thread->name, 0, str_size);
    memcpy(thread->name, name, strlen(name));

    ret = pthread_condattr_init(&thread->condattr);
    if (ret != 0) {
        free(thread->name);
        free(thread);
        return NULL;
    }

    ret = pthread_condattr_setclock(&thread->condattr, CLOCK_MONOTONIC);
    if (ret != 0) {
        free(thread->name);
        free(thread);
        return NULL;
    }


    ret = pthread_cond_init(&thread->cond, &thread->condattr);
    if (ret != 0) {
        free(thread->name);
        free(thread);
        return NULL;
    }

    ret = pthread_mutex_init(&thread->lock, NULL);
    if (ret != 0) {
        free(thread->name);
        free(thread);
        return NULL;
    }

    ret = pthread_attr_init(&thread->attr);
    if (0 != ret) {
        free(thread->name);
        free(thread);
        return NULL;
    }

    ret = pthread_attr_setscope(&thread->attr, PTHREAD_SCOPE_SYSTEM);
    if (0 != ret) {
        free(thread->name);
        free(thread);
        return NULL;
    }

    ret = pthread_create(&thread->thread, &thread->attr, ne_doip_thread_main, thread);
    if (ret != 0) {
        free(thread->name);
        free(thread);
        return NULL;
    }

    return thread;
}

void
ne_doip_thread_release(ne_doip_thread_t *thread)
{
    pthread_cond_destroy(&thread->cond);
    pthread_mutex_destroy(&thread->lock);
    pthread_condattr_destroy(&thread->condattr);
    pthread_attr_destroy(&thread->attr);

    free(thread->name);
    free(thread);
}

const char *
ne_doip_thread_get_name(ne_doip_thread_t *thread)
{
    return thread->name;
}

int
ne_doip_thread_get_id(ne_doip_thread_t *thread)
{
    return thread->tid;
}

int
ne_doip_thread_wait(ne_doip_thread_t *thread)
{
    return pthread_cond_wait(&thread->cond, &thread->lock);
}

int
ne_doip_thread_wait_timeout(ne_doip_thread_t *thread, int timeout_ms)
{
    struct timespec nptime;
    ne_doip_get_timespec(&nptime, timeout_ms);

    return pthread_cond_timedwait(&thread->cond, &thread->lock, &nptime);
}

int
ne_doip_thread_notify(ne_doip_thread_t *thread)
{
    return pthread_cond_signal(&thread->cond);
}

int
ne_doip_thread_join(ne_doip_thread_t *thread)
{
    return pthread_join(thread->thread, NULL);
}

int
ne_doip_thread_terminate(ne_doip_thread_t *thread)
{
    int ret;

    thread->quit_flag = 1;

    ret = pthread_cancel(thread->thread);

    if (ret == 0) {
        ret = ne_doip_thread_join(thread);
    }

    ne_doip_thread_release(thread);

    return ret;
}

int
ne_doip_thread_stop(ne_doip_thread_t *thread)
{
    int ret;

    thread->quit_flag = 1;

    ret = ne_doip_thread_notify(thread);

    if (ret == 0) {
        ret = ne_doip_thread_join(thread);
    }

    ne_doip_thread_release(thread);

    return ret;
}

static void *
ne_doip_thread_main(void *arg)
{
    void *ret;
    ne_doip_thread_t *thread = (ne_doip_thread_t *)arg;

    thread->tid = getpid();

    ret = thread->main_fun(thread, thread->arg);

    return ret;
}

int
ne_doip_thread_checkquit(ne_doip_thread_t *thread)
{
    return thread->quit_flag;
}

ne_doip_sync_t *
ne_doip_sync_create()
{
    ne_doip_sync_t *sync = malloc(sizeof *sync);
    if (!sync) {
        return NULL;
    }

    pthread_mutexattr_init(&sync->attr);
    pthread_mutexattr_settype(&sync->attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&sync->mutex, &sync->attr);

    return sync;
}

void
ne_doip_sync_start(ne_doip_sync_t *sync)
{
    pthread_mutex_lock(&sync->mutex);
}

void
ne_doip_sync_end(ne_doip_sync_t *sync)
{
    pthread_mutex_unlock(&sync->mutex);
}

int
ne_doip_sync_start_try(ne_doip_sync_t *sync)
{
    return pthread_mutex_trylock(&sync->mutex);
}

void
ne_doip_sync_destroy(ne_doip_sync_t *sync)
{
    if (!sync) {
        return;
    }

    pthread_mutexattr_destroy(&sync->attr);
    pthread_mutex_destroy(&sync->mutex);

    free(sync);
}
/* EOF */
