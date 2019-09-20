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
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stddef.h>

#include "ne_doip_client.h"
#include "ne_doip_def.h"
#include "ne_doip_os.h"
#include "ne_doip_client_manager.h"
#include "ne_doip_util.h"

// ********************************************************************
// date definition
// ********************************************************************

#define NE_DOIP_IPC_PATH                    "/tmp/"
#define NE_DOIP_IPC_TEST_EQUIP_THREAD       "NE_DOIP_IPC_TEST_EQUIP_THREAD"
#define NE_DOIP_SERVER_NAME                 "doip_server"

static ne_doip_test_equip_t *g_test_equip = NULL;

// ********************************************************************
// internal interface
// ********************************************************************

// main Thread function
void*
ne_doip_equip_pthread_proc(ne_doip_thread_t *thread, void *arg)
{
    ne_doip_test_equip_t *test_equip = (ne_doip_test_equip_t *)arg;
    if (test_equip == NULL) {
        return NULL;
    }

    while (1) {
        int num = ne_doip_connection_read(test_equip->connection);
        if (num == 0) {
            NE_DOIP_PRINT("ne_doip_equip_pthread_proc DOIP_EVENT_HANGUP\n");
            if (test_equip->connection != NULL) {
                ne_doip_connection_destroy(test_equip->connection);
                if (test_equip->connection != NULL) {
                    free(test_equip->connection);
                    test_equip->connection = NULL;
                }
            }
            return NULL;
        }

        if (num < 0) {
            continue;
        }

        ne_doip_equip_unpack(test_equip);
    }

    return NULL;
}

// Create thread
static int
ne_doip_pthread_create(ne_doip_test_equip_t *test_equip)
{
    if (test_equip == NULL) {
        return -1;
    }

    test_equip->thread = ne_doip_thread_create(ne_doip_equip_pthread_proc, test_equip, NE_DOIP_IPC_TEST_EQUIP_THREAD);

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

    size = offsetof (struct sockaddr_un, sun_path) + name_size;
    if (ne_doip_os_connect(fd, (struct sockaddr *) &addr, size) < 0) {
        NE_DOIP_PRINT("ne_doip_connect_to_socket error code:%d, message:%s\n", errno, strerror(errno));
        close(fd);
        fd = -1;
        return fd;
    }

    return fd;
}

// Initialize the doip test equipment (with the fd binding of the socket)
static ne_doip_test_equip_t *
ne_doip_test_equip_connect()
{
    int fd = ne_doip_connect_to_socket(NE_DOIP_SERVER_NAME);
    if (fd < 0) {
        return NULL;
    }
    ne_doip_test_equip_t *test_equip = NULL;

    test_equip = malloc(sizeof *test_equip);
    memset(test_equip, 0, sizeof *test_equip);

    test_equip->fd = fd;
    test_equip->connection = ne_doip_connection_create(fd);
    if (NULL == test_equip->connection) {
        free(test_equip);
        return NULL;
    }

    int ret = -1;
    ret = ne_doip_pthread_create(test_equip);
    if (ret != 0) {
        free(test_equip);
        return NULL;
    }

    return test_equip;
}

// for internal equipment I/F
DOIP_EXPORT
ne_doip_result_t ne_doip_equip_create(uint16_t logical_source_address,
                                      ne_doip_equip_callback_register_t callback_register)
{
    NE_DOIP_PRINT("ne_doip_equip_create\n");
    if (g_test_equip != NULL) {
        NE_DOIP_PRINT("internal test equipment is already created\n");
        return NE_DOIP_RESULT_ERROR;
    }

    g_test_equip = ne_doip_test_equip_connect();
    if (g_test_equip == NULL) {
        NE_DOIP_PRINT("ne_doip_equip_create is null\n");
        return NE_DOIP_RESULT_ERROR;
    }

    g_test_equip->logical_source_address = logical_source_address;
    g_test_equip->vehicle_identity_callback = callback_register.vehicle_identity_cb;
    g_test_equip->routing_active_callback = callback_register.routing_active_cb;
    g_test_equip->entity_status_callback = callback_register.entity_status_cb;
    g_test_equip->power_mode_callback = callback_register.power_mode_cb;
    g_test_equip->daig_pack_callback = callback_register.diag_pack_cb;
    g_test_equip->daig_nack_callback = callback_register.diag_nack_cb;
    g_test_equip->diagnostic_callback = callback_register.diagnostic_cb;

    ne_doip_connection_t *conn = g_test_equip->connection;
    memset(&conn->out, 0, sizeof conn->out);

    if (0 == ne_doip_pack_test_equip_regist(conn, logical_source_address)) {
        return NE_DOIP_RESULT_OK;
    }
    else {
        return NE_DOIP_RESULT_ERROR;
    }
}

DOIP_EXPORT
ne_doip_result_t ne_doip_equip_destroy()
{
    NE_DOIP_PRINT("ne_doip_equip_destroy\n");
    if (NULL == g_test_equip) {
        NE_DOIP_PRINT("internal test equipment is already destroied\n");
        return NE_DOIP_RESULT_ERROR;
    }

    if (g_test_equip->thread) {
        ne_doip_thread_release(g_test_equip->thread);
        g_test_equip->thread = NULL;
    }

    if (g_test_equip->connection) {
        ne_doip_connection_destroy(g_test_equip->connection);
        if (g_test_equip->connection != NULL) {
            free(g_test_equip->connection);
            g_test_equip->connection = NULL;
        }
    }

    free(g_test_equip);
    g_test_equip = NULL;

    return NE_DOIP_RESULT_OK;
}

DOIP_EXPORT
ne_doip_result_t ne_doip_equip_identity(char* data, // default data is NULL | identification by EID data is EID array | identification by VIN data is VIN array
                                        uint32_t length)
{
    if (NULL == g_test_equip) {
        return NE_DOIP_RESULT_NOT_INITIALIZED;
    }

    if (NULL == data) {
        if (0 == length) {
            // identification
            ne_doip_connection_t *conn = g_test_equip->connection;
            if (NULL == conn) {
                return NE_DOIP_RESULT_ERROR;
            }
            memset(&conn->out, 0, sizeof conn->out);

            if (0 == ne_doip_pack_vehicle_identify(conn)) {
                return NE_DOIP_RESULT_OK;
            }
            else {
                return NE_DOIP_RESULT_ERROR;
            }
        }
        else {
            return NE_DOIP_RESULT_PARAMETER_ERROR;
        }
    }
    else {
        if (NE_DOIP_EID_SIZE == length) {
            // identification by EID
            ne_doip_connection_t *conn = g_test_equip->connection;
            memset(&conn->out, 0, sizeof conn->out);

            if (0 == ne_doip_pack_vehicle_identify_eid(conn, data)) {
                return NE_DOIP_RESULT_OK;
            }
            else {
                return NE_DOIP_RESULT_ERROR;
            }
        }
        else if (NE_DOIP_VIN_SIZE == length) {
            // identification by VIN
            ne_doip_connection_t *conn = g_test_equip->connection;
            memset(&conn->out, 0, sizeof conn->out);

            if (0 == ne_doip_pack_vehicle_identify_vin(conn, data)) {
                return NE_DOIP_RESULT_OK;
            }
            else {
                return NE_DOIP_RESULT_ERROR;
            }
        }
        else {
            return NE_DOIP_RESULT_PARAMETER_ERROR;
        }
    }
}

DOIP_EXPORT
ne_doip_result_t ne_doip_equip_routing_active(uint16_t logical_source_address, uint8_t activation_type,
                                              uint32_t oem_specific_use, char* eid)
{
    if (NULL == g_test_equip) {
        return NE_DOIP_RESULT_NOT_INITIALIZED;
    }

    ne_doip_connection_t *conn = g_test_equip->connection;
    memset(&conn->out, 0, sizeof conn->out);

    if (0 == ne_doip_pack_routing_active(conn, logical_source_address, activation_type, oem_specific_use, eid)) {
        return NE_DOIP_RESULT_OK;
    }
    else {
        return NE_DOIP_RESULT_ERROR;
    }
}

DOIP_EXPORT
ne_doip_result_t ne_doip_equip_alive_check_res(uint16_t logical_target_address)
{
    if (NULL == g_test_equip) {
        return NE_DOIP_RESULT_NOT_INITIALIZED;
    }

    ne_doip_connection_t *conn = g_test_equip->connection;
    memset(&conn->out, 0, sizeof conn->out);

    if (0 == ne_doip_pack_alive_check_response(conn, logical_target_address)) {
        return NE_DOIP_RESULT_OK;
    }
    else {
        return NE_DOIP_RESULT_ERROR;
    }
}

DOIP_EXPORT
ne_doip_result_t ne_doip_equip_entity_status_req(char* eid)
{
    if (NULL == g_test_equip) {
        return NE_DOIP_RESULT_NOT_INITIALIZED;
    }

    ne_doip_connection_t *conn = g_test_equip->connection;
    memset(&conn->out, 0, sizeof conn->out);

    if (0 == ne_doip_pack_entity_status(conn, eid)) {
        return NE_DOIP_RESULT_OK;
    }
    else {
        return NE_DOIP_RESULT_ERROR;
    }
}

DOIP_EXPORT
ne_doip_result_t ne_doip_equip_power_mode_req(char* eid)
{
    if (NULL == g_test_equip) {
        return NE_DOIP_RESULT_NOT_INITIALIZED;
    }

    ne_doip_connection_t *conn = g_test_equip->connection;
    memset(&conn->out, 0, sizeof conn->out);

    if (0 == ne_doip_pack_power_mode(conn, eid)) {
        return NE_DOIP_RESULT_OK;
    }
    else {
        return NE_DOIP_RESULT_ERROR;
    }
}

DOIP_EXPORT
ne_doip_result_t ne_doip_equip_diagnostic(uint16_t logical_target_address,
                                          ne_doip_ta_type_t ta_type,
                                          char* data,
                                          uint32_t length)
{
    if (NULL == g_test_equip) {
        return NE_DOIP_RESULT_NOT_INITIALIZED;
    }

    if ((ta_type != NE_DOIP_TA_TYPE_PHYSICAL && ta_type != NE_DOIP_TA_TYPE_FUNCTIONAL) || NULL == data) {
        return NE_DOIP_RESULT_PARAMETER_ERROR;
    }

    ne_doip_connection_t *conn = g_test_equip->connection;
    memset(&conn->out, 0, sizeof conn->out);

    int result = ne_doip_pack_diagnositc_req(conn, g_test_equip->logical_source_address,
                                             logical_target_address, ta_type, data, length);
    if (0 == result) {
        return NE_DOIP_RESULT_OK;
    }
    else {
        return NE_DOIP_RESULT_ERROR;
    }
}
/* EOF */