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
 * @file ne_doip_event_loop.h
 * @brief event operation
 */

#ifndef NE_DOIP_EVENT_LOOP_H
#define NE_DOIP_EVENT_LOOP_H

#include "ne_doip_select.h"

// callback funk definition
typedef int (*ne_doip_event_loop_fd_func_t)(int fd, uint32_t mask, void *data);

struct ne_doip_event_source
{
    struct ne_doip_event_source_interface *interface;
    int fd;
    ne_doip_select_t *sel_ptr;
    void *data;
};

typedef struct ne_doip_event_source ne_doip_event_source_t;

struct ne_doip_event_source_fd
{
    ne_doip_event_source_t base;
    ne_doip_event_loop_fd_func_t func;
    int fd;
};

struct ne_doip_event_source_interface
{
    int (*dispatch)(ne_doip_event_source_t *source, ne_doip_event_t *ep);
};

ne_doip_event_source_t *
ne_doip_event_source_create(ne_doip_select_t *sel_ptr, int fd,
             ne_doip_event_loop_fd_func_t func,
             void *data);

int ne_doip_source_add(ne_doip_select_t *sel_ptr, ne_doip_event_source_t *source, uint32_t mask);

int ne_doip_event_source_remove(ne_doip_event_source_t *source);

int ne_doip_event_source_fd_update(ne_doip_event_source_t *source, uint32_t mask);

int ne_doip_event_loop_dispatch(ne_doip_select_t *sel_ptr, int timeout, int type);

int ne_doip_event_source_get_fd(ne_doip_event_source_t *source);

#endif // NE_DOIP_EVENT_LOOP_H
/* EOF */
