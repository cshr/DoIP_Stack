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
 * @file ne_doip_select.h
 * @brief doip ne_doip_select
 */

#ifndef NE_DOIP_SELECT_H
#define NE_DOIP_SELECT_H

#include <sys/select.h>
#include <stdint.h>

#include "ne_doip_util.h"
#include "ne_doip_thread.h"

#define NE_DOIP_EV_READ		0x01
#define NE_DOIP_EV_WRITE	0x02

#define NE_DOIP_EV_NORML    0x00
#define NE_DOIP_EV_INTER	0x01

#define NE_DOIP_SEL_ADD     0x00
#define NE_DOIP_SEL_DEL     0x01
#define NE_DOIP_SEL_MOD     0x02

typedef union event_data {
    void *ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
}event_data_t;

typedef struct ne_doip_event {
    ne_doip_list_t base;
    int fd;
    int ev_type;
    uint32_t events;
    event_data_t data;
}ne_doip_event_t;

typedef struct ne_doip_select {
    int max_fd;
    int fd_setsize;
    int notify_fd[2];
    int event_break;
    fd_set *event_readset_in;
    fd_set *event_writeset_in;
    fd_set *event_readset_out;
    fd_set *event_writeset_out;
    ne_doip_list_t event_list;
    ne_doip_sync_t *event_sync;
}ne_doip_select_t;


ne_doip_select_t *ne_doip_select_create();
void ne_doip_select_destroy(ne_doip_select_t *sel_ptr);
int ne_doip_select_ctl(ne_doip_select_t *sel_ptr, int type, int fd, ne_doip_event_t *event);
int ne_doip_select_dispatch(ne_doip_select_t *sel_ptr, ne_doip_event_t *event, int length, int timeout);
int ne_doip_select_notify(ne_doip_select_t *sel_ptr);

#endif // NE_DOIP_SELECT_H
/* EOF */
