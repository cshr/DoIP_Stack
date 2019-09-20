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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "ne_doip_event_loop.h"
#include "ne_doip_util.h"
#include "ne_doip_def.h"

#define NE_DOIP_MAX_EVENT_SIZE  32

static int
ne_doip_event_source_fd_dispatch(ne_doip_event_source_t *source,
                                 ne_doip_event_t *ep)
{
    struct ne_doip_event_source_fd *fd_source = (struct ne_doip_event_source_fd *) source;
    uint32_t mask;

    mask = 0;
    if (ep->events & NE_DOIP_EV_READ) {
        mask |= NE_DOIP_EVENT_READABLE;
    }
    if (ep->events & NE_DOIP_EV_WRITE) {
        mask |= NE_DOIP_EVENT_WRITABLE;
    }

    return fd_source->func(fd_source->fd, mask, source->data);
}

struct ne_doip_event_source_interface fd_source_interface = {
    ne_doip_event_source_fd_dispatch,
};

int
ne_doip_source_add(ne_doip_select_t *sel_ptr, ne_doip_event_source_t *source, uint32_t mask)
{
    ne_doip_event_t ep;

    memset(&ep, 0, sizeof ep);
    if (mask & NE_DOIP_EVENT_READABLE) {
        ep.events |= NE_DOIP_EV_READ;
    }
    if (mask & NE_DOIP_EVENT_WRITABLE) {
        ep.events |= NE_DOIP_EV_WRITE;
    }
    ep.data.ptr = source;

    if (ne_doip_select_ctl(sel_ptr, NE_DOIP_SEL_ADD, source->fd, &ep) < 0) {
        NE_DOIP_PRINT("source_add select_ctl error\n");
        return -1;
    }

    return 0;
}

ne_doip_event_source_t *
ne_doip_event_source_create(ne_doip_select_t *sel_ptr, int fd,
                          ne_doip_event_loop_fd_func_t func, void *data)
{
    struct ne_doip_event_source_fd *source;

    source = malloc(sizeof *source);
    memset(source, 0, sizeof *source);

    source->base.interface = &fd_source_interface;
    source->base.fd = fd;
    source->base.sel_ptr = sel_ptr;
    source->base.data = data;
    source->func = func;
    source->fd = fd;

    return (ne_doip_event_source_t *)source;
}

int
ne_doip_event_source_remove(ne_doip_event_source_t *source)
{
    if (source == NULL) {
        NE_DOIP_PRINT("event_source_remove source is NULL\n");
        return -1;
    }

    if (source->fd < 0) {
        NE_DOIP_PRINT("event_source_remove fd < 0\n");
        return -1;
    }

    errno = 0;
    int fd = source->fd;

    ne_doip_select_ctl(source->sel_ptr, NE_DOIP_SEL_DEL, fd, NULL);
    close(fd);
    source->fd = -1;

    NE_DOIP_PRINT("event_source_remove fd:[%d], code:%d, message:%s\n", fd, errno, strerror(errno));

    return 0;
}

int
ne_doip_event_source_fd_update(ne_doip_event_source_t *source, uint32_t mask)
{
    ne_doip_event_t ep;

    memset(&ep, 0, sizeof ep);
    if (mask & NE_DOIP_EVENT_READABLE) {
        ep.events |= NE_DOIP_EV_READ;
    }
    if (mask & NE_DOIP_EVENT_WRITABLE) {
        ep.events |= NE_DOIP_EV_WRITE;
    }
    ep.data.ptr = source;

    return ne_doip_select_ctl(source->sel_ptr, NE_DOIP_SEL_MOD, source->fd, &ep);
}

int
ne_doip_event_loop_dispatch(ne_doip_select_t *sel_ptr, int timeout, int type)
{
    ne_doip_event_t ep[NE_DOIP_MAX_EVENT_SIZE];
    int i, count;
    errno = 0;

    count = ne_doip_select_dispatch(sel_ptr, ep, ARRAY_LENGTH(ep), timeout);
    if (count < 0) {
        NE_DOIP_PRINT("epoll_wait error code:%d, message:%s\n", errno, strerror(errno));
        return -1;
    }

    NE_DOIP_PRINT("******event_loop_dispatch start type:[%d], count:[%d]\n", type, count);

    for (i = 0; i < count; i++) {
        ne_doip_event_source_t *source = ep[i].data.ptr;
        if (!source) {
            NE_DOIP_PRINT("event_loop_dispatch notify-->continue~~ \n");
            continue;
        }

        if (source->fd != -1) {
            source->interface->dispatch(source, &ep[i]);
        }
    }

    NE_DOIP_PRINT("******event_loop_dispatch end type:[%d]\n", type);

    return 0;
}

int
ne_doip_event_source_get_fd(ne_doip_event_source_t *source)
{
    if (source == NULL) {
        return -1;
    }

    return source->fd;
}

/* EOF */
