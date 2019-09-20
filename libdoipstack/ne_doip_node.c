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
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/un.h>
#include <assert.h>
#include <errno.h>
#include <stddef.h>

#include "ne_doip_node.h"
#include "ne_doip_def.h"
#include "ne_doip_os.h"
#include "ne_doip_node_manager.h"
#include "ne_doip_util.h"

// ********************************************************************
// date definition
// ********************************************************************

#define NE_DOIP_IPC_PATH                "/tmp/"
#define NE_DOIP_IPC_CLIENT_THREAD       "NE_DOIP_IPC_CLIENT_THREAD"
#define NE_DOIP_SERVER_NAME             "doip_server"

static ne_doip_node_list_t *g_node_list = NULL;

// ********************************************************************
// internal interface
// ********************************************************************

// main Thread function
void*
ne_doip_pthread_proc(ne_doip_thread_t *thread, void *arg)
{
    ne_doip_node_t *doip_node = (ne_doip_node_t *)arg;
    if (doip_node == NULL) {
        return NULL;
    }

    while (1) {
        int num = ne_doip_connection_read(doip_node->connection);
        if (num == 0) {
            NE_DOIP_PRINT("ne_doip_pthread_proc read EOF, socket closed\n");
            if (doip_node->connection != NULL) {
                ne_doip_connection_destroy(doip_node->connection);
                if (doip_node->connection != NULL) {
                    free(doip_node->connection);
                    doip_node->connection = NULL;
                }
            }
            return NULL;
        }

        if (num < 0) {
            continue;
        }

        ne_doip_node_unpack(doip_node);
    }

    return NULL;
}

// Create thread
static int
ne_doip_pthread_create(ne_doip_node_t *doip_node)
{
    if (doip_node == NULL) {
        return -1;
    }

    doip_node->thread = ne_doip_thread_create(ne_doip_pthread_proc, doip_node, NE_DOIP_IPC_CLIENT_THREAD);

    return 0;
}

// Create a socket, establish a link with the server, and return a file descriptor
static int
ne_doip_connect_to_socket(const char *name)
{
    int fd;
    int name_size;
    socklen_t size;
    struct sockaddr_un addr;

    errno = 0;
    fd = ne_doip_os_socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        NE_DOIP_PRINT("ne_doip_connect_to_socket fd error code:%d, message:%s\n", errno, strerror(errno));
        return -1;
    }

    addr.sun_family = AF_UNIX;
    name_size = snprintf(addr.sun_path, sizeof addr.sun_path,
                 "%s/%s", NE_DOIP_IPC_PATH, name) + 1;

    assert(name_size > 0);
    if (name_size > (int)sizeof addr.sun_path) {
        NE_DOIP_PRINT("ne_doip_connect_to_socket name_size error!\n");
        close(fd);
        fd = -1;
        /* to prevent programs reporting
         * "failed to add socket: Success" */
        errno = ENAMETOOLONG;
        return fd;
    };

    errno = 0;
    size = offsetof (struct sockaddr_un, sun_path) + name_size;
    if (ne_doip_os_connect(fd, (struct sockaddr *) &addr, size) < 0) {
        NE_DOIP_PRINT("ne_doip_connect_to_socket error code:%d, message:%s\n", errno, strerror(errno));
        close(fd);
        fd = -1;
        return fd;
    }

    return fd;
}

// Initialize the doip node (with the fd binding of the socket)
ne_doip_node_t *
ne_doip_client_connect()
{
    int ret = -1;
    ne_doip_node_t *doip_node = NULL;

    doip_node = malloc(sizeof *doip_node);
    memset(doip_node, 0, sizeof *doip_node);

    int fd = ne_doip_connect_to_socket(NE_DOIP_SERVER_NAME);
    if (fd < 0) {
        free(doip_node);
        return NULL;
    }

    doip_node->fd = fd;
    doip_node->connection = ne_doip_connection_create(fd);

    ret = ne_doip_pthread_create(doip_node);
    if (ret != 0) {
        free(doip_node);
        return NULL;
    }

    return doip_node;
}

ne_doip_node_t *
ne_doip_node_list_find_by_address(uint16_t logical_source_address)
{
    ne_doip_node_t *doip_node = NULL;
    ne_doip_list_for_each(doip_node, g_node_list->node_list, base) {
        if (doip_node->logical_source_address == logical_source_address) {
            return doip_node;
        }
    }
    return NULL;
}

/************************** for doip node I/F ********************************/

DOIP_EXPORT
ne_doip_result_t ne_doip_node_create(uint16_t logical_source_address,
                                     ne_doip_instence_type_t doip_instence_type,
                                     ne_doip_node_callback_register_t callback_register)
{
    NE_DOIP_PRINT("ne_doip_node_create is start..[%04X]\n", logical_source_address);
    if (NULL == g_node_list) {
        g_node_list = malloc(sizeof *g_node_list);
        memset(g_node_list, 0, sizeof *g_node_list);

        g_node_list->node_list_sync = ne_doip_sync_create();
        if (NULL == g_node_list->node_list_sync) {
            return NE_DOIP_RESULT_ERROR;
        }

        // create node list
        ne_doip_node_t *list;
        list = malloc(sizeof *list);
        memset(list, 0, sizeof *list);
        ne_doip_list_init((ne_doip_list_t *)list);

        ne_doip_sync_start(g_node_list->node_list_sync);
        g_node_list->node_list = (ne_doip_list_t *)list;
        ne_doip_sync_end(g_node_list->node_list_sync);
    }

    if (!((logical_source_address >= 0x0001 && logical_source_address <= 0x0DFF)
        || (logical_source_address >= 0x1000 && logical_source_address <= 0x7FFF))) {
        NE_DOIP_PRINT("created node address is invalid! \n");
        return NE_DOIP_RESULT_PARAMETER_ERROR;
    }

    if (doip_instence_type != NE_DOIP_INSTANCE_TYPE_ENTITY && doip_instence_type != NE_DOIP_INSTANCE_TYPE_ECU) {
        NE_DOIP_PRINT("node type is unkown! \n");
        return NE_DOIP_RESULT_PARAMETER_ERROR;
    }

    ne_doip_sync_start(g_node_list->node_list_sync);
    ne_doip_node_t *doip_node = ne_doip_node_list_find_by_address(logical_source_address);
    if (doip_node != NULL) {
        NE_DOIP_PRINT("The doip node has already registered with this logical address! \n");
        ne_doip_sync_end(g_node_list->node_list_sync);
        return NE_DOIP_RESULT_ERROR;
    }
    ne_doip_sync_end(g_node_list->node_list_sync);

    doip_node = ne_doip_client_connect();
    if (NULL == doip_node) {
        NE_DOIP_PRINT("ne_doip_node_create is failed! please retry!\n");
        return NE_DOIP_RESULT_NO_LINK;
    }

    doip_node->logical_source_address = logical_source_address;
    doip_node->doip_instence_type = doip_instence_type;
    if (NULL == callback_register.indication_cb || NULL == callback_register.confirm_cb) {
        return NE_DOIP_RESULT_PARAMETER_ERROR;
    }
    doip_node->indication_callback = callback_register.indication_cb;
    doip_node->confirm_callback = callback_register.confirm_cb;

    ne_doip_sync_start(g_node_list->node_list_sync);
    ne_doip_list_insert(g_node_list->node_list->prev, (ne_doip_list_t *) doip_node);
    ne_doip_sync_end(g_node_list->node_list_sync);

    ne_doip_connection_t *conn = doip_node->connection;
    if (NULL == conn) {
        return NE_DOIP_RESULT_ERROR;
    }
    memset(&conn->out, 0, sizeof conn->out);
    int result = ne_doip_pack_node_regist(conn, doip_node->logical_source_address,
                                          doip_node->doip_instence_type);
    if (0 == result) {
        ne_doip_connection_write(conn);
    }

    if (callback_register.user_conf_cb != NULL) {
        doip_node->user_conf_callback = callback_register.user_conf_cb;
        memset(&conn->out, 0, sizeof conn->out);
        if (0 == ne_doip_pack_regis_confirmation(conn)) {
            ne_doip_connection_write(conn);
        }
    }

    NE_DOIP_PRINT("ne_doip_node_create is end..[%04X]\n", logical_source_address);
    return NE_DOIP_RESULT_OK;
}

DOIP_EXPORT
ne_doip_result_t ne_doip_node_destroy(uint16_t logical_source_address)
{
    NE_DOIP_PRINT("ne_doip_node_destroy is start..[%04X] \n", logical_source_address);
    if (NULL == g_node_list) {
        return NE_DOIP_RESULT_NOT_INITIALIZED;
    }

    if (!((logical_source_address >= 0x0001 && logical_source_address <= 0x0DFF)
        || (logical_source_address >= 0x1000 && logical_source_address <= 0x7FFF))) {
        NE_DOIP_PRINT("destroy node address is invalid! \n");
        return NE_DOIP_RESULT_PARAMETER_ERROR;
    }

    ne_doip_sync_start(g_node_list->node_list_sync);
    ne_doip_node_t *doip_node = ne_doip_node_list_find_by_address(logical_source_address);
    if (NULL == doip_node) {
        ne_doip_sync_end(g_node_list->node_list_sync);
        return NE_DOIP_RESULT_ERROR;
    }

    ne_doip_list_remove((ne_doip_list_t *) doip_node);

    if (doip_node->connection) {
        ne_doip_connection_destroy(doip_node->connection);
        if (doip_node->connection != NULL) {
            free(doip_node->connection);
            doip_node->connection = NULL;
        }
    }

    if (doip_node->thread) {
        ne_doip_thread_stop(doip_node->thread);
        doip_node->thread = NULL;
    }

    free(doip_node);
    doip_node = NULL;
    ne_doip_sync_end(g_node_list->node_list_sync);

    if (0 == ne_doip_list_length(g_node_list->node_list)) {
        free(g_node_list->node_list);
        ne_doip_sync_destroy(g_node_list->node_list_sync);
        free(g_node_list);
        g_node_list = NULL;
    }

    NE_DOIP_PRINT("ne_doip_node_destroy is end..[%04X] \n", logical_source_address);
    return NE_DOIP_RESULT_OK;
}

DOIP_EXPORT
ne_doip_result_t ne_doip_node_send_user_conf_result(uint16_t logical_source_address,
                                                    uint16_t logical_target_address,
                                                    ne_doip_user_con_result_t result)
{
    if (NULL == g_node_list) {
        return NE_DOIP_RESULT_NOT_INITIALIZED;
    }

    if (!((logical_source_address >= 0x0001 && logical_source_address <= 0x0DFF)
        || (logical_source_address >= 0x1000 && logical_source_address <= 0x7FFF))) {
        NE_DOIP_PRINT("node address is invalid! \n");
        return NE_DOIP_RESULT_PARAMETER_ERROR;
    }

    ne_doip_sync_start(g_node_list->node_list_sync);
    ne_doip_node_t *doip_node = ne_doip_node_list_find_by_address(logical_source_address);

    if (NULL == doip_node) {
        ne_doip_sync_end(g_node_list->node_list_sync);
        NE_DOIP_PRINT("The corresponding node was not found by this logical address! \n");
        return NE_DOIP_RESULT_PARAMETER_ERROR;
    }
    else if (NULL == doip_node->user_conf_callback) {
        ne_doip_sync_end(g_node_list->node_list_sync);
        NE_DOIP_PRINT("User_conf callback function has not been registered! \n");
        return NE_DOIP_RESULT_NOT_INITIALIZED;
    }
    else {
        ne_doip_connection_t *conn = doip_node->connection;
        if (NULL == conn) {
            ne_doip_sync_end(g_node_list->node_list_sync);
            return NE_DOIP_RESULT_ERROR;
        }
        memset(&conn->out, 0, sizeof conn->out);
        if (0 == ne_doip_pack_user_conf_result(conn, logical_source_address, logical_target_address, result)) {
            ne_doip_connection_write(conn);
        }
        else {
            ne_doip_sync_end(g_node_list->node_list_sync);
            return NE_DOIP_RESULT_ERROR;
        }
    }
    ne_doip_sync_end(g_node_list->node_list_sync);

    return NE_DOIP_RESULT_OK;
}

DOIP_EXPORT
ne_doip_result_t ne_doip_node_diag_data_request(ne_doip_diag_data_request_t* diag_data_request)
{
    if (NULL == g_node_list) {
        return NE_DOIP_RESULT_NOT_INITIALIZED;
    }

    if (!((diag_data_request->logical_source_address >= 0x0001 && diag_data_request->logical_source_address <= 0x0DFF)
        || (diag_data_request->logical_source_address >= 0x1000 && diag_data_request->logical_source_address <= 0x7FFF))) {
        NE_DOIP_PRINT("node address is invalid! \n");
        return NE_DOIP_RESULT_PARAMETER_ERROR;
    }


    ne_doip_sync_start(g_node_list->node_list_sync);
    ne_doip_node_t *doip_node = ne_doip_node_list_find_by_address(diag_data_request->logical_source_address);

    if (NULL == doip_node) {
        ne_doip_sync_end(g_node_list->node_list_sync);
        NE_DOIP_PRINT("The corresponding node was not found by this logical address! \n");
        return NE_DOIP_RESULT_PARAMETER_ERROR;
    }
    else if ((NULL == doip_node->indication_callback) || NULL == doip_node->confirm_callback) {
        ne_doip_sync_end(g_node_list->node_list_sync);
        NE_DOIP_PRINT("callback function has not been registered! \n");
        return NE_DOIP_RESULT_NOT_INITIALIZED;
    }
    else {
        ne_doip_connection_t *conn = doip_node->connection;
        if (NULL == conn) {
            ne_doip_sync_end(g_node_list->node_list_sync);
            return NE_DOIP_RESULT_ERROR;
        }
        memset(&conn->out, 0, sizeof conn->out);
        int result = ne_doip_pack_diagnositc_res(conn, doip_node->logical_source_address,
                                                 diag_data_request->logical_target_address,
                                                 diag_data_request->data,
                                                 diag_data_request->data_length);
        if (0 == result) {
            ne_doip_connection_write(conn);
        }
        else {
            ne_doip_sync_end(g_node_list->node_list_sync);
            return NE_DOIP_RESULT_ERROR;
        }
    }
    ne_doip_sync_end(g_node_list->node_list_sync);

    return NE_DOIP_RESULT_OK;
}
/* EOF */