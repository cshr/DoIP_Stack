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
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/resource.h>
#include <sched.h>

#ifdef LINUX_OS
#include <sys/syscall.h>
#endif // LINUX_OS

#include "ne_doip_thread.h"
#include "ne_doip_util.h"
#include "ne_doip_timer.h"

#define NE_DOIP_TIMER_REQ_THREAD    "NE_DOIP_TIMER_REQ_THREAD"
#define NE_DOIP_INFINITE            0xFFFFFFFF


#ifdef LINUX_OS
pid_t
ne_doip_get_tid(void)
{
    return syscall(__NR_gettid);
}

#endif // LINUX_OS

int
ne_doip_nextID(ne_doip_timer_manager_t *manager)
{
    int m_dtid = 0;
    int i;
    for (i = 0; i < NE_DOIP_TIMER_MAX_NUM; i++) {
        m_dtid++;
        if (m_dtid != 0 && manager->timer_table[m_dtid] == 0) {
            return m_dtid;
        }
    }
    NE_DOIP_PRINT("The assigned Timer ID reaches the upper limit..\n");
    return 0;
}

void
ne_doip_notify(ne_doip_timer_manager_t *manager)
{
    ne_doip_sync_start(manager->sync);
    manager->m_bSignalflg = 1;
    int ret;
    ret = ne_doip_thread_notify(manager->thread);
    ne_doip_sync_end(manager->sync);
    if (0 != ret) {
        NE_DOIP_PRINT("NTimerManager ne_doip_notify signal failed %d [%s].", ret, strerror(ret));
    }
}

void
ne_doip_add_request(ne_doip_timer_manager_t *manager, ne_doip_timer_t *data)
{
    ne_doip_timer_t *timer;
    ne_doip_timer_t* tmp;
    int pos = 0;
    int flag = 0;

    ne_doip_list_for_each_safe(timer, tmp, manager->timer_head, base) {
        if (ne_doip_subtimespec(&timer->timeout_tick, &data->timeout_tick) > 0) {
            ne_doip_list_insert(((ne_doip_list_t *)timer)->prev, (ne_doip_list_t *)data);
            flag = 1;
            break;
        }
        pos++;
    }
    if (!flag) {
      ne_doip_list_insert(manager->timer_head->prev, (ne_doip_list_t *)data);
    }
    if (pos == 0) {
        ne_doip_notify(manager);
    }
}

void *
ne_doip_pthread_func(ne_doip_thread_t *thread, void *arg)
{
    unsigned int sleep_time = NE_DOIP_INFINITE;
    ne_doip_timer_manager_t *manager = (ne_doip_timer_manager_t *)arg;
    if (manager == NULL) {
        return NULL;
    }

// #ifdef LINUX_OS
//     int pri = -19; // highest
//       errno = 0;
//     int ret = setpriority(PRIO_PROCESS, ne_doip_get_tid(), pri);
//     if (ret < 0) {
//         NE_DOIP_PRINT("<<<< pthread_func[%d] priority : %d failed, ret = %d [%s]\n", ne_doip_get_tid(), pri, ret, strerror(errno));
//     }

//     pri = getpriority(PRIO_PROCESS, ne_doip_get_tid());
//     NE_DOIP_PRINT("<<<< pthread_func run (%d) prio: %d\n", ne_doip_get_tid(), pri);
// #else

//     int policy = SCHED_FIFO;
//     int pri = sched_get_priority_max(policy);
//     NE_DOIP_PRINT("pthread_func prio policy:[%d], maximum:[%d]\n", policy, pri);

//     struct sched_param param;
//     param.__sched_priority = pri;
//     int ret = pthread_setschedparam(pthread_self(), policy, &param);
//     if (ret != 0) {
//         NE_DOIP_PRINT("pthread_func[%ld] priority:[%d] set error\n", pthread_self(), pri);
//     }

//     param.__sched_priority = 0;
//     pthread_getschedparam(pthread_self(), &policy, &param);
//     NE_DOIP_PRINT("pthread_func prio setting value:[%d]\n", param.__sched_priority);
// #endif // LINUX_OS

    while (!manager->thread->quit_flag) {

        pthread_mutex_lock(&manager->thread->lock);
        if (!manager->m_bSignalflg) {
            if (NE_DOIP_INFINITE != sleep_time && sleep_time != 0) {
                ne_doip_thread_wait_timeout(manager->thread, sleep_time);

            }
            else {
                pthread_cond_wait(&manager->thread->cond, &manager->thread->lock);
            }
        }
        manager->m_bSignalflg = 0;
        pthread_mutex_unlock(&manager->thread->lock);
        // calculate by system time
        ne_doip_timer_t *timer;
        struct timespec tmp_req;
        ne_doip_sync_start(manager->sync);


        while (ne_doip_list_length(manager->timer_head)) {
            timer = (ne_doip_timer_t *)manager->timer_head->next;
            ne_doip_list_remove((ne_doip_list_t *)timer);

            tmp_req = timer->timeout_tick;
            struct timespec timeout_tick;
            memset(&timeout_tick, 0, sizeof(timeout_tick));
            ne_doip_get_timespec(&timeout_tick, 0);

            if (ne_doip_subtimespec(&timeout_tick, &tmp_req) >= 0) {
                timer->callback(timer);

                if (timer->m_bIterate > 0) {
                    ne_doip_set_timespec(&tmp_req, timer->period);
                    timer->timeout_tick = tmp_req;
                    ne_doip_add_request(manager, timer);
                }
                else {
                    free(timer);
                }

                sleep_time = NE_DOIP_INFINITE;
            }
            else {
                ne_doip_list_insert(manager->timer_head, (ne_doip_list_t*)timer);
                sleep_time = ne_doip_subtimespec(&tmp_req, &timeout_tick);
                break;
            }
        }

        if (!ne_doip_list_length(manager->timer_head)) {
            sleep_time = NE_DOIP_INFINITE;
        }

        ne_doip_sync_end(manager->sync);

    }
    return NULL;
}

ne_doip_timer_manager_t *
ne_doip_create_timer_manager()
{
    ne_doip_timer_manager_t* manager = NULL;

    manager = malloc(sizeof *manager);
    if (manager == NULL) {
        return NULL;
    }
    memset(manager, 0, sizeof *manager);

    manager->sync = ne_doip_sync_create();

    ne_doip_list_t* list;
    list = malloc(sizeof *list);
    ne_doip_list_init(list);
    manager->timer_head = list;

    manager->thread = ne_doip_thread_create(ne_doip_pthread_func, manager, NE_DOIP_TIMER_REQ_THREAD);
    if (manager->thread == NULL) {
        free(list);
        free(manager);
        return NULL;
    }

    return manager;
}

int
ne_doip_timer_start(ne_doip_timer_manager_t *manager, int m_bIterate, int period,
                    void(*ne_doip_timer_callback)(void *arg1)) // Add timer interface
{
    if (period < 0) {
        NE_DOIP_PRINT("ne_doip_timer_start input error\n");
        return 0;
    }
    struct timespec timeout_tick;
    memset(&timeout_tick, 0, sizeof(timeout_tick));
    ne_doip_get_timespec(&timeout_tick, period);

    ne_doip_timer_t *data = malloc(sizeof *data);
    data->m_bIterate = m_bIterate;
    data->period = period;
    data->callback = ne_doip_timer_callback;
    data->timeid = ne_doip_nextID(manager);
    data->timeout_tick = timeout_tick;
    ne_doip_sync_start(manager->sync);
    int ret = (data->timeid != 0);
    if (ret) {
        manager->timer_table[data->timeid] = 1;
        ne_doip_add_request(manager, data);

    }
    ne_doip_sync_end(manager->sync);

    return data->timeid;
}


void
ne_doip_timer_stop(ne_doip_timer_manager_t *manager, int timeid) // Delete the timer interface according to the node index
{
    if (!timeid) {
        return;
    }

    ne_doip_sync_start(manager->sync);
    manager->timer_table[timeid] = 0;
    ne_doip_timer_t *timer;
    ne_doip_timer_t *tmp;
    ne_doip_list_for_each_safe(timer, tmp, manager->timer_head, base) {
        if (timer->timeid == timeid) {
            ne_doip_list_remove((ne_doip_list_t *)timer);
            free(timer);

            ne_doip_sync_end(manager->sync);
            return;
        }
    }
    ne_doip_sync_end(manager->sync);
}

// Refresh timer attribute interface according to node index
void
ne_doip_timer_restart(ne_doip_timer_manager_t *manager, int m_bIterate, int period, int timeid)
{

    if ((period < 0) || (timeid == 0)) {
        NE_DOIP_PRINT("doiptimer_restart input error\n");
        return;
    }

    ne_doip_sync_start(manager->sync);
    ne_doip_timer_t *timer;
    ne_doip_timer_t *tmp;
    ne_doip_list_for_each_safe(timer, tmp, manager->timer_head, base) {
        if (timer->timeid == timeid) {
            struct timespec timeout_tick;
            memset(&timeout_tick, 0, sizeof(timeout_tick));
            ne_doip_get_timespec(&timeout_tick, period);
            ne_doip_list_remove((ne_doip_list_t *)timer);
            timer->m_bIterate = m_bIterate;
            timer->period = period;
            timer->timeout_tick = timeout_tick;
            ne_doip_add_request(manager, timer);
            ne_doip_sync_end(manager->sync);
            return;
        }
    }

    ne_doip_sync_end(manager->sync);
    NE_DOIP_PRINT("doiptimer_restart timeid error\n");
}

void
ne_doip_destroy_timer_manager(ne_doip_timer_manager_t *manager)
{
    if (manager == NULL) {
        return;
    }

    if (manager->thread) {
        ne_doip_thread_stop(manager->thread);
        manager->thread = NULL;
    }

    // Waiting for thread to end
    if (manager->timer_head) {
        ne_doip_sync_start(manager->sync);

        ne_doip_timer_t *timer;
        ne_doip_timer_t *tmp;
        ne_doip_list_for_each_safe(timer, tmp, manager->timer_head, base) {
            ne_doip_list_remove((ne_doip_list_t*)timer);
            free(timer);
        }

        free(manager->timer_head);

        ne_doip_sync_end(manager->sync);
    }

    ne_doip_sync_destroy(manager->sync);
    free(manager);
}

uint32_t ne_doip_get_random_value(uint32_t max_value)
{
    srand((unsigned)time(NULL));
    uint32_t random_value = rand() % (max_value + 1);
    NE_DOIP_PRINT("ne_doip_get_random_value is [%d].\n", random_value);
    return random_value;
}
/* EOF */
