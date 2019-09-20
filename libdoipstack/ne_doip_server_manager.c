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

#include "ne_doip_server_manager.h"
#include "ne_doip_os.h"
#include "ne_doip_config.h"
#include "ne_doip_util.h"


static ne_doip_server_manager_t *global_server_manager = NULL;
static int g_task_id = 0;

void ne_doip_udp_send(ne_doip_connection_t *conn, char *ip, uint16_t port);
void ne_doip_udp6_send(ne_doip_connection_t *conn, char *ip, uint16_t port);
int ne_doip_test_tcp_create(ne_doip_server_t *server, char *ip, uint16_t port);
int ne_doip_test_tcp6_create(ne_doip_server_t *server, char *ip, uint16_t port);
int ne_doip_server_disconnect(ne_doip_server_t *server, int socket_type, int fd);
int ne_doip_server_disconnect_all(ne_doip_server_t *server, int socket_type);

void ne_doip_vehicle_announce_wait_timer_callback(void* arg1);
void ne_doip_vehicle_announce_interval_timer_callback(void* arg1);
void ne_doip_vehicle_indentify_wait_timer_callback(void* arg1);
void ne_doip_initial_timer_callback(void* arg1);
void ne_doip_general_timer_callback(void* arg1);
void ne_doip_alive_check_timer_callback(void* arg1);
void ne_doip_pack_diagnostic_positive_ack(ne_doip_link_data_t *link_data,
                                          uint16_t entity_logical_address,
                                          uint16_t equip_logical_address);
void ne_doip_pack_diagnostic_negative_ack(ne_doip_link_data_t *link_data, uint16_t entity_logical_address,
                                          uint16_t equip_logical_address, ne_doip_diagnostic_nack_code_t code);
void ne_doip_integrate_thread_task(ne_doip_diag_source_type_t diag_source_type,
                                   ne_doip_link_data_t *link_data,
                                   uint32_t pos, uint32_t payload_length);
void ne_doip_thread_callback_func(void *data, void *user_data);

ne_doip_routing_table_t *
ne_doip_routing_list_find_by_logic_address(uint16_t logical_address)
{
    ne_doip_routing_table_t *routing_table = NULL;
    ne_doip_list_for_each(routing_table, global_server_manager->server->config->routing_list, base) {
        if (routing_table->entity_logical_address == logical_address) {
            return routing_table;
        }
    }
    return NULL;
}

ne_doip_node_ipc_table_t *
ne_doip_node_ipc_list_find_by_fd(int fd)
{
    ne_doip_node_ipc_table_t *node_ipc_table = NULL;
    ne_doip_list_for_each(node_ipc_table, global_server_manager->node_ipc_list, base) {
        if (node_ipc_table->fd == fd) {
            return node_ipc_table;
        }
    }
    return NULL;
}

ne_doip_node_ipc_table_t *
ne_doip_node_ipc_list_find_by_logic_address(uint16_t logical_address)
{
    ne_doip_node_ipc_table_t *node_ipc_table = NULL;
    ne_doip_list_for_each(node_ipc_table, global_server_manager->node_ipc_list, base) {
        if (node_ipc_table->ecu_logical_address == logical_address) {
            return node_ipc_table;
        }
    }
    return NULL;
}

ne_doip_node_ipc_table_t *
ne_doip_node_ipc_list_find_entity()
{
    ne_doip_node_ipc_table_t *node_ipc_table = NULL;
    ne_doip_list_for_each(node_ipc_table, global_server_manager->node_ipc_list, base) {
        if (node_ipc_table->entity_logical_address != 0) {
            return node_ipc_table;
        }
    }
    return NULL;
}

ne_doip_node_tcp_table_t *
ne_doip_node_tcp_list_find_by_fd(int fd)
{
    ne_doip_node_tcp_table_t *node_tcp_table = NULL;
    ne_doip_list_for_each(node_tcp_table, global_server_manager->node_tcp_list, base) {
        if (node_tcp_table->fd == fd) {
            return node_tcp_table;
        }
    }
    return NULL;
}

ne_doip_node_tcp_table_t *
ne_doip_node_tcp_list_find_by_logic_address(uint16_t equip_logical_address)
{
    ne_doip_node_tcp_table_t *node_tcp_table = NULL;
    ne_doip_list_for_each(node_tcp_table, global_server_manager->node_tcp_list, base) {
        if (node_tcp_table->equip_logical_address == equip_logical_address) {
            return node_tcp_table;
        }
    }
    return NULL;
}

ne_doip_node_udp_table_t *
ne_doip_node_udp_list_find_by_ip_port(char* ip, uint16_t port)
{
    ne_doip_node_udp_table_t *node_udp_table = NULL;
    ne_doip_list_for_each(node_udp_table, global_server_manager->node_udp_list, base) {
        if (0 == strcmp(node_udp_table->ip, ip) && node_udp_table->port == port) {
            return node_udp_table;
        }
    }
    return NULL;
}

uint8_t
ne_doip_node_tcp_list_regist_fd_count()
{
    uint8_t count = 0;
    ne_doip_node_tcp_table_t *node_tcp_table = NULL;
    ne_doip_list_for_each(node_tcp_table, global_server_manager->node_tcp_list, base) {
        if (NE_DOIP_TRUE == node_tcp_table->fd_regist_flag) {
            ++count;
        }
    }
    return count;
}

ne_doip_equip_ipc_table_t *
ne_doip_equip_ipc_list_find_by_fd(int fd)
{
    ne_doip_equip_ipc_table_t *equip_ipc_table = NULL;
    ne_doip_list_for_each(equip_ipc_table, global_server_manager->equip_ipc_list, base) {
        if (equip_ipc_table->fd == fd) {
            return equip_ipc_table;
        }
    }
    return NULL;
}

ne_doip_equip_ipc_table_t *
ne_doip_equip_ipc_list_find_by_logic_address(uint16_t logical_address)
{
    ne_doip_equip_ipc_table_t *equip_ipc_table = NULL;
    ne_doip_list_for_each(equip_ipc_table, global_server_manager->equip_ipc_list, base) {
        if (equip_ipc_table->equip_logical_address == logical_address) {
            return equip_ipc_table;
        }
    }
    return NULL;
}

ne_doip_equip_tcp_table_t *
ne_doip_equip_tcp_list_find_by_fd(int fd)
{
    ne_doip_equip_tcp_table_t *equip_tcp_table = NULL;
    ne_doip_list_for_each(equip_tcp_table, global_server_manager->equip_tcp_list, base) {
        if (equip_tcp_table->fd == fd) {
            return equip_tcp_table;
        }
    }
    return NULL;
}

ne_doip_equip_tcp_table_t *
ne_doip_equip_tcp_list_find_by_logic_address(uint16_t entity_logical_address)
{
    ne_doip_equip_tcp_table_t *equip_tcp_table = NULL;
    ne_doip_list_for_each(equip_tcp_table, global_server_manager->equip_tcp_list, base) {
        if (equip_tcp_table->entity_logical_address == entity_logical_address) {
            return equip_tcp_table;
        }
    }
    return NULL;
}

ne_doip_equip_tcp_table_t *
ne_doip_equip_tcp_list_find_by_ip_port(char* ip, uint16_t port)
{
    ne_doip_equip_tcp_table_t *equip_tcp_table = NULL;
    ne_doip_list_for_each(equip_tcp_table, global_server_manager->equip_tcp_list, base) {
        if (equip_tcp_table->ip != NULL) {
            if (0 == strcmp(equip_tcp_table->ip, ip) && equip_tcp_table->port == port) {
                return equip_tcp_table;
            }
        }
    }
    return NULL;
}

ne_doip_equip_udp_table_t *
ne_doip_equip_udp_list_find_by_eid(char* eid)
{
    ne_doip_equip_udp_table_t *equip_udp_table = NULL;
    char eid_array[NE_DOIP_EID_SIZE] = { 0 };
    strncpy(eid_array, eid, NE_DOIP_EID_SIZE);
    ne_doip_list_for_each(equip_udp_table, global_server_manager->equip_udp_list, base) {
        if (0 == strncmp(equip_udp_table->eid, eid_array, NE_DOIP_EID_SIZE)) {
            return equip_udp_table;
        }
    }
    return NULL;
}

ne_doip_equip_udp_table_t *
ne_doip_equip_udp_list_find_by_ip_port(char* ip, uint16_t port)
{
    ne_doip_equip_udp_table_t *equip_udp_table = NULL;
    ne_doip_list_for_each(equip_udp_table, global_server_manager->equip_udp_list, base) {
        if (equip_udp_table->ip != NULL) {
            if (0 == strcmp(equip_udp_table->ip, ip) && equip_udp_table->port == port) {
                return equip_udp_table;
            }
        }
    }
    return NULL;
}

ne_doip_node_udp_table_t *
ne_doip_node_udp_list_find_by_timerid(int timer_id)
{
    ne_doip_node_udp_table_t *node_udp_table = NULL;
    ne_doip_list_for_each(node_udp_table, global_server_manager->node_udp_list, base) {
        if (node_udp_table->vehicle_identify_wait_timeid == timer_id) {
            return node_udp_table;
        }
    }
    return NULL;
}

ne_doip_node_tcp_table_t *
ne_doip_node_tcp_list_find_by_timerid(int timer_id)
{
    ne_doip_node_tcp_table_t *node_tcp_table = NULL;
    ne_doip_list_for_each(node_tcp_table, global_server_manager->node_tcp_list, base) {
        if (node_tcp_table->tcp_initial_inactivity_timeid == timer_id
            || node_tcp_table->tcp_generral_inactivity_timeid == timer_id
            || node_tcp_table->tcp_alive_check_timeid == timer_id) {
            return node_tcp_table;
        }
    }
    return NULL;
}

void ne_doip_server_manager_create(ne_doip_server_t *server)
{
    NE_DOIP_PRINT("ne_doip_server_manager_create is start.\n");
    if (NULL == server) {
        return;
    }
    global_server_manager = malloc(sizeof *global_server_manager);
    memset(global_server_manager, 0, sizeof *global_server_manager);

    global_server_manager->server = server;

    global_server_manager->node_ipc_list_sync = ne_doip_sync_create();
    global_server_manager->node_tcp_list_sync = ne_doip_sync_create();
    global_server_manager->node_udp_list_sync = ne_doip_sync_create();
    global_server_manager->equip_ipc_list_sync = ne_doip_sync_create();
    global_server_manager->equip_tcp_list_sync = ne_doip_sync_create();
    global_server_manager->equip_udp_list_sync = ne_doip_sync_create();

    // node ipc table list init
    ne_doip_node_ipc_table_t *node_ipc_table = NULL;
    node_ipc_table = malloc(sizeof *node_ipc_table);
    memset(node_ipc_table, 0, sizeof *node_ipc_table);

    ne_doip_list_init((ne_doip_list_t *)node_ipc_table);
    global_server_manager->node_ipc_list = (ne_doip_list_t *)node_ipc_table;

    // node tcp table list init
    ne_doip_node_tcp_table_t *node_tcp_table = NULL;
    node_tcp_table = malloc(sizeof *node_tcp_table);
    memset(node_tcp_table, 0, sizeof *node_tcp_table);

    ne_doip_list_init((ne_doip_list_t *)node_tcp_table);
    global_server_manager->node_tcp_list = (ne_doip_list_t *)node_tcp_table;

    // node udp table list init
    ne_doip_node_udp_table_t *node_udp_table = NULL;
    node_udp_table = malloc(sizeof *node_udp_table);
    memset(node_udp_table, 0, sizeof *node_udp_table);

    ne_doip_list_init((ne_doip_list_t *)node_udp_table);
    global_server_manager->node_udp_list = (ne_doip_list_t *)node_udp_table;

    // equip ipc table list init
    ne_doip_equip_ipc_table_t *equip_ipc_table = NULL;
    equip_ipc_table = malloc(sizeof *equip_ipc_table);
    memset(equip_ipc_table, 0, sizeof *equip_ipc_table);

    ne_doip_list_init((ne_doip_list_t *)equip_ipc_table);
    global_server_manager->equip_ipc_list = (ne_doip_list_t *)equip_ipc_table;

    // equip tcp table list init
    ne_doip_equip_tcp_table_t *equip_tcp_table = NULL;
    equip_tcp_table = malloc(sizeof *equip_tcp_table);
    memset(equip_tcp_table, 0, sizeof *equip_tcp_table);

    ne_doip_list_init((ne_doip_list_t *)equip_tcp_table);
    global_server_manager->equip_tcp_list = (ne_doip_list_t *)equip_tcp_table;

    // equip udp table list init
    ne_doip_equip_udp_table_t *equip_udp_table = NULL;
    equip_udp_table = malloc(sizeof *equip_udp_table);
    memset(equip_udp_table, 0, sizeof *equip_udp_table);

    ne_doip_list_init((ne_doip_list_t *)equip_udp_table);
    global_server_manager->equip_udp_list = (ne_doip_list_t *)equip_udp_table;

    global_server_manager->threadpool = ne_doip_threadpool_new(ne_doip_thread_callback_func, NULL, 20, 1, 10000);

    global_server_manager->timer_manager = ne_doip_create_timer_manager();
    NE_DOIP_PRINT("ne_doip_server_manager_create is end.\n");
}

void ne_doip_server_manager_destroy()
{
    NE_DOIP_PRINT("ne_doip_server_manager_destroy is enter..\n");
    // Release timer resources
    ne_doip_node_tcp_table_t* node_tcp_table = NULL;
    ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
    ne_doip_list_for_each(node_tcp_table, global_server_manager->node_tcp_list, base) {
        if (node_tcp_table->tcp_initial_inactivity_timeid != 0) {
            ne_doip_timer_stop(global_server_manager->timer_manager, node_tcp_table->tcp_initial_inactivity_timeid);
        }
        if (node_tcp_table->tcp_generral_inactivity_timeid != 0) {
            ne_doip_timer_stop(global_server_manager->timer_manager, node_tcp_table->tcp_generral_inactivity_timeid);
        }
        if (node_tcp_table->tcp_alive_check_timeid != 0) {
            ne_doip_timer_stop(global_server_manager->timer_manager, node_tcp_table->tcp_alive_check_timeid);
        }
    }
    ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
    NE_DOIP_PRINT("stop tcp timer resource is complete..\n");

    ne_doip_node_udp_table_t* node_udp_table = NULL;
    ne_doip_sync_start(global_server_manager->node_udp_list_sync);
    ne_doip_list_for_each(node_udp_table, global_server_manager->node_udp_list, base) {
        if (node_udp_table->vehicle_identify_wait_timeid != 0) {
            ne_doip_timer_stop(global_server_manager->timer_manager, node_udp_table->vehicle_identify_wait_timeid);
        }
    }
    ne_doip_sync_end(global_server_manager->node_udp_list_sync);
    NE_DOIP_PRINT("stop udp timer resource is complete..\n");

    ne_doip_destroy_timer_manager(global_server_manager->timer_manager);

    ne_doip_threadpool_free(global_server_manager->threadpool, 0, 1);

    // Release the node ipc table list resource
    ne_doip_node_ipc_table_t* node_ipc_table = NULL;
    ne_doip_node_ipc_table_t *node_ipc_table_tmp = NULL;
    ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
    ne_doip_list_for_each_safe(node_ipc_table, node_ipc_table_tmp, global_server_manager->node_ipc_list, base) {
        ne_doip_list_t *list_node = (ne_doip_list_t *) node_ipc_table;
        ne_doip_list_remove(list_node);
        free(node_ipc_table);
    }
    ne_doip_sync_end(global_server_manager->node_ipc_list_sync);

    free(global_server_manager->node_ipc_list);
    global_server_manager->node_ipc_list = NULL;
    ne_doip_sync_destroy(global_server_manager->node_ipc_list_sync);
    NE_DOIP_PRINT("remove node ipc list is complete..\n");

    // Release the node tcp table list resource
    ne_doip_node_tcp_table_t *node_tcp_table_tmp = NULL;
    ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
    ne_doip_list_for_each_safe(node_tcp_table, node_tcp_table_tmp, global_server_manager->node_tcp_list, base) {
        if (node_tcp_table->ip != NULL) {
            free(node_tcp_table->ip);
        }
        ne_doip_list_t *list_node = (ne_doip_list_t *) node_tcp_table;
        ne_doip_list_remove(list_node);
        free(node_tcp_table);
    }
    ne_doip_sync_end(global_server_manager->node_tcp_list_sync);

    free(global_server_manager->node_tcp_list);
    global_server_manager->node_tcp_list = NULL;
    ne_doip_sync_destroy(global_server_manager->node_tcp_list_sync);
    NE_DOIP_PRINT("remove node tcp list is complete..\n");

    // Release the node udp table list resource
    ne_doip_node_udp_table_t *node_udp_table_tmp = NULL;
    ne_doip_sync_start(global_server_manager->node_udp_list_sync);
    ne_doip_list_for_each_safe(node_udp_table, node_udp_table_tmp, global_server_manager->node_udp_list, base) {
        if (node_udp_table->ip != NULL) {
            free(node_udp_table->ip);
        }
        ne_doip_list_t *list_node = (ne_doip_list_t *) node_udp_table;
        ne_doip_list_remove(list_node);
        free(node_udp_table);
    }
    ne_doip_sync_end(global_server_manager->node_udp_list_sync);

    free(global_server_manager->node_udp_list);
    global_server_manager->node_udp_list = NULL;
    ne_doip_sync_destroy(global_server_manager->node_udp_list_sync);
    NE_DOIP_PRINT("remove node udp list is complete..\n");

    // Release the equip ipc table list resource
    ne_doip_equip_ipc_table_t* equip_ipc_table = NULL;
    ne_doip_equip_ipc_table_t *equip_ipc_table_tmp = NULL;
    ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
    ne_doip_list_for_each_safe(equip_ipc_table, equip_ipc_table_tmp, global_server_manager->equip_ipc_list, base) {
        ne_doip_list_t *list_node = (ne_doip_list_t *) equip_ipc_table;
        ne_doip_list_remove(list_node);
        free(equip_ipc_table);
    }
    ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

    free(global_server_manager->equip_ipc_list);
    global_server_manager->equip_ipc_list = NULL;
    ne_doip_sync_destroy(global_server_manager->equip_ipc_list_sync);
    NE_DOIP_PRINT("remove equip ipc list is complete..\n");

    // Release the equip tcp table list resource
    ne_doip_equip_tcp_table_t *equip_tcp_table = NULL;
    ne_doip_equip_tcp_table_t *equip_tcp_table_tmp = NULL;
    ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
    ne_doip_list_for_each_safe(equip_tcp_table, equip_tcp_table_tmp, global_server_manager->equip_tcp_list, base) {
        if (equip_tcp_table->ip != NULL) {
            free(equip_tcp_table->ip);
        }
        if (equip_tcp_table->queue_manager != NULL) {
            ne_doip_queue_deinit(equip_tcp_table->queue_manager);
        }

        ne_doip_list_t *list_node = (ne_doip_list_t *) equip_tcp_table;
        ne_doip_list_remove(list_node);
        free(equip_tcp_table);
    }
    ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);

    free(global_server_manager->equip_tcp_list);
    global_server_manager->equip_tcp_list = NULL;
    ne_doip_sync_destroy(global_server_manager->equip_tcp_list_sync);
    NE_DOIP_PRINT("remove equip tcp list is complete..\n");

    // Release the equip udp table list resource
    ne_doip_equip_udp_table_t *equip_udp_table = NULL;
    ne_doip_equip_udp_table_t *equip_udp_table_tmp = NULL;
    ne_doip_sync_start(global_server_manager->equip_udp_list_sync);
    ne_doip_list_for_each_safe(equip_udp_table, equip_udp_table_tmp, global_server_manager->equip_udp_list, base) {
        if (equip_udp_table->ip != NULL) {
            free(equip_udp_table->ip);
        }
        ne_doip_list_t *list_node = (ne_doip_list_t *) equip_udp_table;
        ne_doip_list_remove(list_node);
        free(equip_udp_table);
    }
    ne_doip_sync_end(global_server_manager->equip_udp_list_sync);

    free(global_server_manager->equip_udp_list);
    global_server_manager->equip_udp_list = NULL;
    ne_doip_sync_destroy(global_server_manager->equip_udp_list_sync);
    NE_DOIP_PRINT("remove equip udp list is complete..\n");

    free(global_server_manager);
    global_server_manager = NULL;
    NE_DOIP_PRINT("ne_doip_server_manager_destroy is end..\n");
}

void ne_doip_add_connection_table(ne_doip_addr_data_t *addr_data)
{
    if (NULL == addr_data) {
        return;
    }
    NE_DOIP_PRINT("ne_doip_add_connection_table is enter...fd is [%d] comm_type is [%d]\n", addr_data->fd, addr_data->comm_type);

    if (NE_DOIP_SOCKET_TYPE_TCP == addr_data->comm_type) {
        ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
        ne_doip_node_tcp_table_t* node_tcp_table = malloc(sizeof *node_tcp_table);
        memset(node_tcp_table, 0, sizeof *node_tcp_table);

        node_tcp_table->fd = addr_data->fd;
        if (NULL != addr_data->ip) {
            node_tcp_table->ip = (char*)malloc(strlen(addr_data->ip) + 1);
            if (NULL != node_tcp_table->ip) {
                memcpy(node_tcp_table->ip, addr_data->ip, strlen(addr_data->ip) + 1);
            }
        }
        node_tcp_table->port = addr_data->port;
        node_tcp_table->comm_type = addr_data->comm_type;
        node_tcp_table->connection_state = addr_data->connection_state;
        ne_doip_list_insert(global_server_manager->node_tcp_list->prev, (ne_doip_list_t *) node_tcp_table);
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
        // Start initial inactivity timer
        node_tcp_table->tcp_initial_inactivity_timeid = ne_doip_timer_start(global_server_manager->timer_manager,
                                                                           -1, global_server_manager->server->config->initial_inactivity_time,
                                                                           ne_doip_initial_timer_callback);
    }
    else if (NE_DOIP_SOCKET_TYPE_TEST == addr_data->comm_type) {
        ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
        ne_doip_equip_tcp_table_t* equip_tcp_table = malloc(sizeof *equip_tcp_table);
        memset(equip_tcp_table, 0, sizeof *equip_tcp_table);
        equip_tcp_table->fd = addr_data->fd;
        if (NULL != addr_data->ip) {
            equip_tcp_table->ip = (char*)malloc(strlen(addr_data->ip) + 1);
            if (NULL != equip_tcp_table->ip) {
                memcpy(equip_tcp_table->ip, addr_data->ip, strlen(addr_data->ip) + 1);
            }
        }
        equip_tcp_table->port = addr_data->port;
        equip_tcp_table->comm_type = NE_DOIP_SOCKET_TYPE_TEST;
        ne_doip_list_insert(global_server_manager->equip_tcp_list->prev, (ne_doip_list_t *) equip_tcp_table);
        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);
    }
}

void ne_doip_remove_connection_table(int fd, uint8_t socket_type)
{
    if (NE_DOIP_SOCKET_TYPE_IPC == socket_type) {
        NE_DOIP_PRINT("[ne_doip_remove_connection_table] remove ipc fd [%d] start \n", fd);
        ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
        ne_doip_node_ipc_table_t* node_ipc_table = ne_doip_node_ipc_list_find_by_fd(fd);
        if (node_ipc_table != NULL) {
            ne_doip_list_remove((ne_doip_list_t *) node_ipc_table);
            free(node_ipc_table);
            ne_doip_server_disconnect(global_server_manager->server, NE_DOIP_SOCKET_TYPE_TCP, node_ipc_table->tcp_fd);
        }
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);

        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t* equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(fd);
        if (equip_ipc_table != NULL) {
            int fd_array[NE_DOIP_ALL_ECU_NUM] = { 0 };

            ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
            ne_doip_equip_tcp_table_t *equip_tcp_table = NULL;
            int i = 0;
            ne_doip_list_for_each(equip_tcp_table, global_server_manager->equip_tcp_list, base) {
                if (equip_tcp_table->source_fd == equip_ipc_table->fd) {
                    fd_array[i++] = equip_tcp_table->fd;
                }
            }
            ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);
            ne_doip_list_remove((ne_doip_list_t *) equip_ipc_table);
            free(equip_ipc_table);

            for (i = 0; i < NE_DOIP_ALL_ECU_NUM; ++i) {
                if (fd_array[i] != 0) {
                    printf("remove equip tcp fd is %d\n", fd_array[i]);
                    ne_doip_server_disconnect(global_server_manager->server, NE_DOIP_SOCKET_TYPE_TEST, fd_array[i]);
                }
            }

        }
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
        NE_DOIP_PRINT("[ne_doip_remove_connection_table] remove ipc fd [%d] end \n", fd);
    }
    else if (NE_DOIP_SOCKET_TYPE_TCP == socket_type) {
        NE_DOIP_PRINT("[ne_doip_remove_connection_table] remove tcp fd [%d] start \n", fd);
        ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
        ne_doip_node_tcp_table_t* node_tcp_table = ne_doip_node_tcp_list_find_by_fd(fd);
        if (NULL == node_tcp_table) {
            ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
            return;
        }
        if (node_tcp_table->ip != NULL) {
            free(node_tcp_table->ip);
        }

        if (node_tcp_table->tcp_initial_inactivity_timeid != 0) {
            ne_doip_timer_stop(global_server_manager->timer_manager, node_tcp_table->tcp_initial_inactivity_timeid);
        }
        if (node_tcp_table->tcp_generral_inactivity_timeid != 0) {
            ne_doip_timer_stop(global_server_manager->timer_manager, node_tcp_table->tcp_generral_inactivity_timeid);
        }
        if (node_tcp_table->tcp_alive_check_timeid != 0) {
            ne_doip_timer_stop(global_server_manager->timer_manager, node_tcp_table->tcp_alive_check_timeid);
        }

        ne_doip_list_remove((ne_doip_list_t *) node_tcp_table);
        free(node_tcp_table);
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
        NE_DOIP_PRINT("[ne_doip_remove_connection_table] remove tcp fd [%d] end \n", fd);
    }
    else if (NE_DOIP_SOCKET_TYPE_TEST == socket_type) {
        NE_DOIP_PRINT("[ne_doip_remove_connection_table] remove test tcp fd [%d] start \n", fd);
        ne_doip_equip_tcp_table_t *equip_tcp_table = NULL;
        ne_doip_equip_tcp_table_t *equip_tcp_table_tmp = NULL;
        ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
        ne_doip_list_for_each_safe(equip_tcp_table, equip_tcp_table_tmp, global_server_manager->equip_tcp_list, base) {
            if (fd == equip_tcp_table->fd) {
                if (equip_tcp_table->ip != NULL) {
                    free(equip_tcp_table->ip);
                    equip_tcp_table->ip = NULL;
                }
                ne_doip_list_t *list_node = (ne_doip_list_t *) equip_tcp_table;
                ne_doip_list_remove(list_node);
                free(equip_tcp_table);

                ne_doip_routing_table_t *routing_table = NULL;
                ne_doip_list_for_each(routing_table, global_server_manager->server->config->routing_list, base) {
                    if (routing_table->fd == fd) {
                        routing_table->fd = 0;
                    }
                }
            }
        }
        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);
        NE_DOIP_PRINT("[ne_doip_remove_connection_table] remove test tcp     [%d] end \n", fd);
    }
}

void ne_doip_pack_announce_or_identityresponse(ne_doip_link_data_t *link_data,
                                               uint16_t entity_logical_address,
                                               unsigned char futher_action)
{
    NE_DOIP_PRINT("ne_doip_pack_announce_or_identityresponse start...\n");

    char header_without_payload[NE_DOIP_HEADER_COMMON_LENGTH] =
        { global_server_manager->server->config->protocol_version,
         ~global_server_manager->server->config->protocol_version,
          0x00, 0x04, 0x00, 0x00, 0x00, 0x00 };

    uint32_t data_length = NE_DOIP_HEADER_COMMON_LENGTH;
    if (NE_DOIP_TRUE == global_server_manager->server->config->need_vin_gid_sync) {
        header_without_payload[NE_DOIP_HEADER_COMMON_LENGTH - 1] = NE_DOIP_ANNOUNCE_OR_IDENTITYRES_ALL_LENGTH;
        data_length += NE_DOIP_ANNOUNCE_OR_IDENTITYRES_ALL_LENGTH;
    }
    else {
        header_without_payload[NE_DOIP_HEADER_COMMON_LENGTH - 1] = NE_DOIP_ANNOUNCE_OR_IDENTITYRES_MAND_LENGTH;
        data_length += NE_DOIP_ANNOUNCE_OR_IDENTITYRES_MAND_LENGTH;
    }

    char* send_data = (char*)malloc(data_length);
    memset(send_data, 0, data_length);

    uint8_t pos = 0;
    memcpy(send_data, header_without_payload, NE_DOIP_HEADER_COMMON_LENGTH);
    pos += NE_DOIP_HEADER_COMMON_LENGTH;
    memcpy(send_data + pos, global_server_manager->server->config->vin, NE_DOIP_VIN_SIZE);
    pos += NE_DOIP_VIN_SIZE;
    entity_logical_address = ne_doip_bswap_16(entity_logical_address);
    memcpy(send_data + pos, &entity_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    memcpy(send_data + pos, global_server_manager->server->config->eid, NE_DOIP_EID_SIZE);
    pos += NE_DOIP_EID_SIZE;
    memcpy(send_data + pos, global_server_manager->server->config->gid, NE_DOIP_GID_SIZE);
    pos += NE_DOIP_GID_SIZE;
    memcpy(send_data + pos, &futher_action, NE_DOIP_FURTHER_ACTION_LENGTH);
    if (NE_DOIP_TRUE == global_server_manager->server->config->need_vin_gid_sync) {
        pos += NE_DOIP_FURTHER_ACTION_LENGTH;
        memcpy(send_data + pos, &global_server_manager->server->vin_gid_sync_flag, NE_DOIP_VIN_GID_SYNC_LENGTH);
    }

    if (NE_DOIP_SOCKET_TYPE_UDP_UNI == link_data->comm_type) {
        ne_doip_pack(link_data, NE_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_RESPONSE, send_data, data_length);
    }
    else if (NE_DOIP_SOCKET_TYPE_UDP_MOUTI == link_data->comm_type) {
        ne_doip_pack(link_data, NE_DOIP_PAYLOAD_TYPE_VEHICLE_ANNOUNCEMENT, send_data, data_length);
    }
    else if (NE_DOIP_SOCKET_TYPE_IPC == link_data->comm_type) {
        // Transfer to internal test equipment
        NE_DOIP_PRINT("vehicle identity response to internal test equipemt...\n");
        ne_doip_connection_t *conn = malloc(sizeof *conn);
        if (conn != NULL) {
            memset(conn, 0, sizeof *conn);
            uint8_t pos_t = 0;
            uint32_t payload_length = data_length - NE_DOIP_HEADER_COMMON_LENGTH;
            conn->out.data[pos_t] = NE_DOIP_IN_EQUIP_ANN_IDEN_RES;
            pos_t += NE_DOIP_IN_COMMAND_LENGTH;
            memcpy(conn->out.data + pos_t, &payload_length, NE_DOIP_IN_DATA_LENGTH);
            pos_t += NE_DOIP_IN_DATA_LENGTH;
            memcpy(conn->out.data + pos_t, send_data + NE_DOIP_HEADER_COMMON_LENGTH, payload_length);
            conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
            conn->fd = link_data->fd;
            ne_doip_connection_write(conn);
            free(conn);
        }
    }
    free(send_data);
}

void ne_doip_pack_vehicle_identification_req(ne_doip_link_data_t *link_data)
{
    // If there is a doipnode registered in the doipstack, then the response of the own entity is sent back.
    ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
    ne_doip_node_ipc_table_t* ipc_entity_table = ne_doip_node_ipc_list_find_entity();
    if (ipc_entity_table != NULL) {
        if (global_server_manager->server->config->authen_info != 0
            || NE_DOIP_TRUE == ipc_entity_table->confirmation_flag) {
            ne_doip_pack_announce_or_identityresponse(link_data, ipc_entity_table->entity_logical_address,
                                                      NE_DOIP_FURTHER_ACTION_REQUIRED);
        }
        else {
            ne_doip_pack_announce_or_identityresponse(link_data, ipc_entity_table->entity_logical_address,
                                                      NE_DOIP_NO_FURTHER_ACTION_REQUIRED);
        }
    }
    ne_doip_sync_end(global_server_manager->node_ipc_list_sync);

    // empty the udp table list
    ne_doip_equip_udp_table_t *equip_udp_table = NULL;
    ne_doip_equip_udp_table_t *equip_udp_table_tmp = NULL;
    ne_doip_sync_start(global_server_manager->equip_udp_list_sync);
    ne_doip_list_for_each_safe(equip_udp_table, equip_udp_table_tmp, global_server_manager->equip_udp_list, base) {
        if (equip_udp_table->ip != NULL) {
            free(equip_udp_table->ip);
            equip_udp_table->ip = NULL;
        }
        ne_doip_list_t *list_node = (ne_doip_list_t *) equip_udp_table;
        ne_doip_list_remove(list_node);
        free(equip_udp_table);
    }
    ne_doip_sync_end(global_server_manager->equip_udp_list_sync);

    // Package request sent to other entities in the LAN
    char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
        { global_server_manager->server->config->protocol_version,
         ~global_server_manager->server->config->protocol_version,
          0x00, 0x01, 0x00, 0x00, 0x00, 0x00 };

    ne_doip_pack(link_data, NE_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST,
                 header_data, NE_DOIP_HEADER_COMMON_LENGTH);
}

void ne_doip_pack_vehicle_identification_req_eid(ne_doip_link_data_t *link_data, uint8_t pos)
{
    char eid[NE_DOIP_EID_SIZE] = { 0 };
    memcpy(eid, link_data->data + pos, NE_DOIP_EID_SIZE);

    uint8_t result = strncmp(global_server_manager->server->config->eid, eid, NE_DOIP_EID_SIZE);
    if (0 == result) {
        // If there is a doipnode registered in the doipstack, then the response of the own entity is sent back.
        ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
        ne_doip_node_ipc_table_t* ipc_entity_table = ne_doip_node_ipc_list_find_entity();
        if (ipc_entity_table != NULL) {
            if (global_server_manager->server->config->authen_info != 0
                || NE_DOIP_TRUE == ipc_entity_table->confirmation_flag) {
                ne_doip_pack_announce_or_identityresponse(link_data, ipc_entity_table->entity_logical_address,
                                                          NE_DOIP_FURTHER_ACTION_REQUIRED);
            }
            else {
                ne_doip_pack_announce_or_identityresponse(link_data, ipc_entity_table->entity_logical_address,
                                                          NE_DOIP_NO_FURTHER_ACTION_REQUIRED);
            }
        }
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
    }
    else {
        // Find the equip udp table by EID
        ne_doip_sync_start(global_server_manager->equip_udp_list_sync);
        ne_doip_equip_udp_table_t* equip_udp_table = ne_doip_equip_udp_list_find_by_eid(eid);
        if (NULL == equip_udp_table) {
            ne_doip_sync_end(global_server_manager->equip_udp_list_sync);
            return;
        }

        ne_doip_link_data_t* link_data_t = malloc(sizeof *link_data_t);
        memset(link_data_t, 0, sizeof *link_data_t);

        link_data_t->fd = equip_udp_table->fd;
        link_data_t->ip = (char*)malloc(strlen(equip_udp_table->ip) + 1);
        if (NULL != link_data_t->ip) {
            memcpy(link_data_t->ip, equip_udp_table->ip, strlen(equip_udp_table->ip) + 1);
        }
        link_data_t->port = equip_udp_table->port;

        // remove current list node, wait for the new node to update
        if (equip_udp_table->ip != NULL) {
            free(equip_udp_table->ip);
            equip_udp_table->ip = NULL;
        }
        ne_doip_list_t *list_node = (ne_doip_list_t *) equip_udp_table;
        ne_doip_list_remove(list_node);
        free(equip_udp_table);
        ne_doip_sync_end(global_server_manager->equip_udp_list_sync);

        uint32_t data_size = NE_DOIP_HEADER_COMMON_LENGTH + NE_DOIP_EID_SIZE;
        char* data = (char*)malloc(data_size);
        memset(data, 0, data_size);

        char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
            { global_server_manager->server->config->protocol_version,
             ~global_server_manager->server->config->protocol_version,
              0x00, 0x02, 0x00, 0x00, 0x00, 0x06 };

        memcpy(data, header_data, NE_DOIP_HEADER_COMMON_LENGTH);
        memcpy(data + NE_DOIP_HEADER_COMMON_LENGTH, link_data->data + pos, NE_DOIP_EID_SIZE);

        ne_doip_pack(link_data_t, NE_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST_EID, data, data_size);
        if (link_data_t->ip != NULL) {
            free(link_data_t->ip);
        }
        free(link_data_t);
        free(data);
    }
}

void ne_doip_pack_vehicle_identification_req_vin(ne_doip_link_data_t *link_data, uint8_t pos)
{
    char vin[NE_DOIP_VIN_SIZE] = { 0 };
    memcpy(vin, link_data->data + pos, NE_DOIP_VIN_SIZE);

    uint8_t result = strncmp(global_server_manager->server->config->vin, vin, NE_DOIP_VIN_SIZE);
    if (0 == result) {
        // If there is a doipnode registered in the doipstack, then the response of the own entity is sent back.
        ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
        ne_doip_node_ipc_table_t* ipc_entity_table = ne_doip_node_ipc_list_find_entity();
        if (ipc_entity_table != NULL) {
            if (global_server_manager->server->config->authen_info != 0
                || NE_DOIP_TRUE == ipc_entity_table->confirmation_flag) {
                ne_doip_pack_announce_or_identityresponse(link_data, ipc_entity_table->entity_logical_address,
                                                          NE_DOIP_FURTHER_ACTION_REQUIRED);
            }
            else {
                ne_doip_pack_announce_or_identityresponse(link_data, ipc_entity_table->entity_logical_address,
                                                          NE_DOIP_NO_FURTHER_ACTION_REQUIRED);
            }
        }
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
    }
    else {
        // empty the equip udp table list
        ne_doip_equip_udp_table_t *equip_udp_table = NULL;
        ne_doip_equip_udp_table_t *equip_udp_table_tmp = NULL;
        ne_doip_sync_start(global_server_manager->equip_udp_list_sync);
        ne_doip_list_for_each_safe(equip_udp_table, equip_udp_table_tmp, global_server_manager->equip_udp_list, base) {
            if (equip_udp_table->ip != NULL) {
                free(equip_udp_table->ip);
                equip_udp_table->ip = NULL;
            }
            ne_doip_list_t *list_node = (ne_doip_list_t *) equip_udp_table;
            ne_doip_list_remove(list_node);
            free(equip_udp_table);
        }
        ne_doip_sync_end(global_server_manager->equip_udp_list_sync);

        // Package request sent to other entities in the LAN
        uint32_t data_size = NE_DOIP_HEADER_COMMON_LENGTH + NE_DOIP_VIN_SIZE;
        char* data = (char*)malloc(data_size);
        memset(data, 0, data_size);

        char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
            { global_server_manager->server->config->protocol_version,
             ~global_server_manager->server->config->protocol_version,
              0x00, 0x03, 0x00, 0x00, 0x00, 0x11 };

        memcpy(data, header_data, NE_DOIP_HEADER_COMMON_LENGTH);
        memcpy(data + NE_DOIP_HEADER_COMMON_LENGTH, link_data->data + pos, NE_DOIP_VIN_SIZE);

        ne_doip_pack(link_data, NE_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST_VIN, data, data_size);
        free(data);
    }
}

void ne_doip_pack_routing_activation_res(ne_doip_link_data_t *link_data,
                                         uint16_t equip_logical_address,
                                         uint16_t entity_logical_address,
                                         ne_doip_routing_activation_res_code_t code,
                                         uint32_t reserved)
{
    if (code < NE_DOIP_RA_RES_UNKNOWN_SOURCE_ADDRESS || code > NE_DOIP_RA_RES_CONFIRMATION_REQUIRED) {
        NE_DOIP_PRINT("[%s] parameter is invalid! ", __FUNCTION__);
        return;
    }

    char* send_data = (char*)malloc(NE_DOIP_ROUTING_ACTIVATION_RES_LENGTH);
    memset(send_data, 0, NE_DOIP_ROUTING_ACTIVATION_RES_LENGTH);

    char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
        { global_server_manager->server->config->protocol_version,
         ~global_server_manager->server->config->protocol_version,
          0x00, 0x06, 0x00, 0x00, 0x00, 0x09 };

    uint8_t pos = 0;
    memcpy(send_data, header_data, NE_DOIP_HEADER_COMMON_LENGTH);
    pos += NE_DOIP_HEADER_COMMON_LENGTH;
    equip_logical_address = ne_doip_bswap_16(equip_logical_address);
    memcpy(send_data + pos, &equip_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    entity_logical_address = ne_doip_bswap_16(entity_logical_address);
    memcpy(send_data + pos, &entity_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    memcpy(send_data + pos, &code, NE_DOIP_RA_RES_CODE_LENGTH);
    pos += NE_DOIP_RA_RES_CODE_LENGTH;
    memcpy(send_data + pos, &reserved, NE_DOIP_S_RESERVED_LENGTH);

    ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
    ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(link_data->fd);
    if (equip_ipc_table != NULL) {
        int fd = equip_ipc_table->fd;
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

        // Transfer to internal test equipment
        NE_DOIP_PRINT("routing activation response to internal test equipemt...\n");
        ne_doip_connection_t *conn = malloc(sizeof *conn);
        if (conn != NULL) {
            memset(conn, 0, sizeof *conn);
            uint8_t pos_t = 0;
            uint32_t payload_length = NE_DOIP_ROUTING_ACTIVATION_RES_LENGTH - NE_DOIP_HEADER_COMMON_LENGTH;
            conn->out.data[pos_t] = NE_DOIP_IN_EQUIP_ROUTING_ACTIVE;
            pos_t += NE_DOIP_IN_COMMAND_LENGTH;
            memcpy(conn->out.data + pos_t, &payload_length, NE_DOIP_IN_DATA_LENGTH);
            pos_t += NE_DOIP_IN_DATA_LENGTH;
            memcpy(conn->out.data + pos_t, send_data + NE_DOIP_HEADER_COMMON_LENGTH, payload_length);
            conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
            conn->fd = fd;
            ne_doip_connection_write(conn);
            free(conn);
        }
    }
    else {
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
        ne_doip_pack(link_data, NE_DOIP_PAYLOAD_TYPE_ROUTING_ACTIVE_RESPONSE,
                     send_data, NE_DOIP_ROUTING_ACTIVATION_RES_LENGTH);
    }

    free(send_data);
}

void ne_doip_pack_routing_activation_req(ne_doip_link_data_t *link_data, uint8_t pos, uint32_t payload_size)
{
    char eid[NE_DOIP_EID_SIZE] = { 0 };
    memcpy(eid, link_data->data + pos, NE_DOIP_EID_SIZE);

    uint8_t result = strncmp(global_server_manager->server->config->eid, eid, NE_DOIP_EID_SIZE);
    if (0 == result || 0 == strlen(eid)) {
        // Routing activates itself
        NE_DOIP_PRINT("routing activation doip entity of self network..\n");

        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(link_data->fd);
        if (NULL == equip_ipc_table) {
            NE_DOIP_PRINT("<pack_routing_activation_req step> get equip ipc table is failed! \n");
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
            return;
        }
        else {
            uint16_t equip_logical_address = equip_ipc_table->equip_logical_address;
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

            ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
            ne_doip_node_ipc_table_t *node_ipc_table = ne_doip_node_ipc_list_find_entity();
            if (NULL == node_ipc_table) {
                NE_DOIP_PRINT("<pack_routing_activation_req step> get entity ipc_table is failed! \n");
                ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
                return;
            }
            else {
                uint16_t entity_logical_address = node_ipc_table->entity_logical_address;
                ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
                ne_doip_pack_routing_activation_res(link_data, equip_logical_address,
                                                    entity_logical_address,
                                                    NE_DOIP_RA_RES_ROUTING_SUCCESSFULLY_ACTIVATED,
                                                    0x00000000);
            }
        }
    }
    else {
        // Routing activates other
        NE_DOIP_PRINT("routing activation doip entity of other network..\n");
        // Find the test udp table by EID
        ne_doip_sync_start(global_server_manager->equip_udp_list_sync);
        ne_doip_equip_udp_table_t* equip_udp_table = ne_doip_equip_udp_list_find_by_eid(eid);
        if (NULL == equip_udp_table) {
            NE_DOIP_PRINT("<pack_routing_activation_req step> get equip_udp_table is failed! \n");
            ne_doip_sync_end(global_server_manager->equip_udp_list_sync);
            return;
        }

        ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
        ne_doip_equip_tcp_table_t* equip_tcp_table = ne_doip_equip_tcp_list_find_by_ip_port(equip_udp_table->ip,
                                                                                            equip_udp_table->port);
        ne_doip_link_data_t link_data_t;
        memset(&link_data_t, 0, sizeof link_data_t);

        if (equip_tcp_table != NULL) {
            link_data_t.fd = equip_tcp_table->fd;
            ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);

            ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
            ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(link_data->fd);
            if (equip_ipc_table != NULL) {
                equip_ipc_table->tcp_fd = link_data_t.fd;
            }
            else {
                NE_DOIP_PRINT("<pack_routing_activation_req step> get equip ipc table is failed! \n");
            }
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
        }
        else {
            ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);

            if (NE_DOIP_NET_TYPE_IPV6 == global_server_manager->server->config->net_type) {
                link_data_t.fd = ne_doip_test_tcp6_create(global_server_manager->server,
                                                         equip_udp_table->ip, equip_udp_table->port);
            }
            else if (NE_DOIP_NET_TYPE_IPV4 == global_server_manager->server->config->net_type) {
                link_data_t.fd = ne_doip_test_tcp_create(global_server_manager->server,
                                                        equip_udp_table->ip, equip_udp_table->port);
            }
            if (-1 == link_data_t.fd) {
                NE_DOIP_PRINT("create test tcp fd is failed!\n");
                ne_doip_sync_end(global_server_manager->equip_udp_list_sync);
                return;
            }

            ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
            ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(link_data->fd);
            if (equip_ipc_table != NULL) {
                equip_ipc_table->tcp_fd = link_data_t.fd;
            }
            else {
                NE_DOIP_PRINT("<pack_routing_activation_req step> get equip ipc table is failed! \n");
            }
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

            /***********equip tcp information is added to euip tcp list***********/
            ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
            ne_doip_equip_tcp_table_t* equip_tcp_table = malloc(sizeof *equip_tcp_table);
            memset(equip_tcp_table, 0, sizeof *equip_tcp_table);

            equip_tcp_table->fd = link_data_t.fd;
            equip_tcp_table->source_fd = link_data->fd;

            equip_tcp_table->ip = (char*)malloc(strlen(equip_udp_table->ip) + 1);
            if (NULL != equip_tcp_table->ip) {
                memcpy(equip_tcp_table->ip, equip_udp_table->ip, strlen(equip_udp_table->ip) + 1);
            }

            equip_tcp_table->port = equip_udp_table->port;
            equip_tcp_table->comm_type = NE_DOIP_SOCKET_TYPE_TEST;
            ne_doip_list_insert(global_server_manager->equip_tcp_list->prev, (ne_doip_list_t *) equip_tcp_table);
            ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);
            /**********************************************************************/
        }

        ne_doip_sync_end(global_server_manager->equip_udp_list_sync);

        // pack routing activation request message
        uint32_t data_size = NE_DOIP_HEADER_COMMON_LENGTH + payload_size - NE_DOIP_EID_SIZE;
        char* data = (char*)malloc(data_size);
        memset(data, 0, data_size);

        char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
            { global_server_manager->server->config->protocol_version,
             ~global_server_manager->server->config->protocol_version,
              0x00, 0x05, 0x00, 0x00, 0x00, 0x00 };
        header_data[NE_DOIP_HEADER_COMMON_LENGTH -1] = (unsigned char)(payload_size - NE_DOIP_EID_SIZE);


        memcpy(data, header_data, NE_DOIP_HEADER_COMMON_LENGTH);
        memcpy(data + NE_DOIP_HEADER_COMMON_LENGTH, link_data->data + pos + NE_DOIP_EID_SIZE,
               payload_size - NE_DOIP_EID_SIZE);

        ne_doip_pack(&link_data_t, NE_DOIP_PAYLOAD_TYPE_ROUTING_ACTIVE_REQUEST, data, data_size);
        free(data);
    }
}

void ne_doip_pack_alive_check_res(ne_doip_link_data_t *link_data, uint8_t pos)
{
    char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
    { global_server_manager->server->config->protocol_version,
     ~global_server_manager->server->config->protocol_version,
      0x00, 0x08, 0x00, 0x00, 0x00, 0x02 };

    uint32_t data_size = NE_DOIP_HEADER_COMMON_LENGTH + NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    char* data = (char*)malloc(data_size);
    memset(data, 0, data_size);

    memcpy(data, header_data, NE_DOIP_HEADER_COMMON_LENGTH);

    if (NE_DOIP_SOCKET_TYPE_IPC == link_data->comm_type) {
        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(link_data->fd);
        if (NULL == equip_ipc_table) {
            NE_DOIP_PRINT("<pack alive check res step> get equip ipc table is failed! by fd[%d].\n", link_data->fd);
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
        }
        else {
            uint16_t equip_logical_address = ne_doip_bswap_16(equip_ipc_table->equip_logical_address);
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
            memcpy(data + NE_DOIP_HEADER_COMMON_LENGTH, &equip_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);

            uint16_t entity_logical_address = 0x0000;
            memcpy(&entity_logical_address, link_data->data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            entity_logical_address = ne_doip_bswap_16(entity_logical_address);
            ne_doip_link_data_t link_data_t;
            memset(&link_data_t, 0, sizeof link_data_t);

            ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
            ne_doip_equip_tcp_table_t* equip_tcp_table = ne_doip_equip_tcp_list_find_by_logic_address(entity_logical_address);
            if (NULL == equip_tcp_table) {
                NE_DOIP_PRINT("<pack_alive_check_res step> get test tcp table is failed! by LA[%04X].\n", entity_logical_address);
                ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);
                free(data);
                return;
            }
            else {
                link_data_t.fd = equip_tcp_table->fd;
                link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TEST;
            }
            ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);
            ne_doip_pack(&link_data_t, NE_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_RESPONSE, data, data_size);
        }
    }
    else {
        ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
        ne_doip_equip_tcp_table_t *equip_tcp_table = ne_doip_equip_tcp_list_find_by_fd(link_data->fd);
        if (NULL == equip_tcp_table) {
            NE_DOIP_PRINT("<pack alive check res step> get test tcp table is failed! by fd[%d].\n", link_data->fd);
            ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);
        }
        else {
            int fd = equip_tcp_table->source_fd;
            ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);

            ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
            ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(fd);
            if (NULL == equip_ipc_table) {
                NE_DOIP_PRINT("internal test equipment has been released... do not send alive check response...\n");
                ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
            }
            else {
                uint16_t equip_logical_address = ne_doip_bswap_16(equip_ipc_table->equip_logical_address);
                ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
                memcpy(data + NE_DOIP_HEADER_COMMON_LENGTH, &equip_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
                ne_doip_pack(link_data, NE_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_RESPONSE, data, data_size);
            }
        }
    }
    free(data);
}

void ne_doip_pack_entity_status_res(ne_doip_link_data_t *link_data, unsigned char ncts)
{
    char* send_data = (char*)malloc(NE_DOIP_ENTITY_STATUS_RES_LENGTH);
    memset(send_data, 0, NE_DOIP_ENTITY_STATUS_RES_LENGTH);

    char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
        { global_server_manager->server->config->protocol_version,
         ~global_server_manager->server->config->protocol_version,
          0x40, 0x02, 0x00, 0x00, 0x00, 0x07 };

    uint8_t pos = 0;
    memcpy(send_data, header_data, NE_DOIP_HEADER_COMMON_LENGTH);
    pos += NE_DOIP_HEADER_COMMON_LENGTH;
    unsigned char node_type = NE_DOIP_NT_UNKOWN;
    if (NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server_manager->server->config->entity_type
        || NE_DOIP_ENTITY_TYPE_GATEWAY == global_server_manager->server->config->entity_type) {
        node_type = NE_DOIP_NT_GATEWAY;
    }
    else if (NE_DOIP_ENTITY_TYPE_NODE == global_server_manager->server->config->entity_type) {
        node_type = NE_DOIP_NT_NODE;
    }
    memcpy(send_data + pos, &node_type, NE_DOIP_NODE_TYPE_LENGTH);
    pos += NE_DOIP_NODE_TYPE_LENGTH;
    memcpy(send_data + pos, &global_server_manager->server->config->mcts, NE_DOIP_MCTS_LENGTH);
    pos += NE_DOIP_MCTS_LENGTH;
    memcpy(send_data + pos, &ncts, NE_DOIP_NCTS_LENGTH);
    pos += NE_DOIP_NCTS_LENGTH;
    uint32_t mds = ne_doip_bswap_32(global_server_manager->server->config->mds);
    memcpy(send_data + pos, &mds, NE_DOIP_MDS_LENGTH);

    if (NE_DOIP_SOCKET_TYPE_IPC == link_data->comm_type) {
        NE_DOIP_PRINT("entity status response to internal test equipemt...\n");
        ne_doip_connection_t *conn = malloc(sizeof *conn);
        if (conn != NULL) {
            memset(conn, 0, sizeof *conn);
            uint8_t pos = 0;
            conn->out.data[pos] = NE_DOIP_IN_EQUIP_ENTITY_STATUS;
            pos += NE_DOIP_IN_COMMAND_LENGTH;
            uint32_t payload_length = NE_DOIP_ENTITY_STATUS_RES_LENGTH - NE_DOIP_HEADER_COMMON_LENGTH;
            memcpy(conn->out.data + pos, &payload_length, NE_DOIP_IN_DATA_LENGTH);
            pos += NE_DOIP_IN_DATA_LENGTH;
            memcpy(conn->out.data + pos, send_data + NE_DOIP_HEADER_COMMON_LENGTH, payload_length);
            conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
            conn->fd = link_data->fd;
            ne_doip_connection_write(conn);
            free(conn);
        }
    }
    else {
        ne_doip_pack(link_data, NE_DOIP_PAYLOAD_TYPE_ENTITY_STATUS_RESPONSE,
                     send_data, NE_DOIP_ENTITY_STATUS_RES_LENGTH);
    }

    free(send_data);
}

void ne_doip_pack_entity_status_req(ne_doip_link_data_t *link_data, uint8_t pos)
{
    char eid[NE_DOIP_EID_SIZE] = { 0 };
    memcpy(eid, link_data->data + pos, NE_DOIP_EID_SIZE);

    uint8_t result = strncmp(global_server_manager->server->config->eid, eid, NE_DOIP_EID_SIZE);
    if (0 == result || 0 == strlen(eid)) {
        ne_doip_sync_start(global_server_manager->server->net_list_sync);
        uint8_t open_socket_num = ne_doip_list_length(global_server_manager->server->net_client_list);
        ne_doip_sync_end(global_server_manager->server->net_list_sync);
        ne_doip_pack_entity_status_res(link_data, open_socket_num);
    }
    else {
        // Find the test udp table by EID
        ne_doip_sync_start(global_server_manager->equip_udp_list_sync);
        ne_doip_equip_udp_table_t* equip_udp_table = ne_doip_equip_udp_list_find_by_eid(eid);
        if (NULL == equip_udp_table) {
            NE_DOIP_PRINT("<pack_entity_status_req step> get test udp table is failed! \n");
            ne_doip_sync_end(global_server_manager->equip_udp_list_sync);
            return;
        }

        equip_udp_table->source_fd = link_data->fd;

        ne_doip_link_data_t* link_data_t = malloc(sizeof *link_data_t);
        memset(link_data_t, 0, sizeof *link_data_t);

        link_data_t->fd = equip_udp_table->fd;
        link_data_t->ip = (char*)malloc(strlen(equip_udp_table->ip) + 1);
        if (NULL != link_data_t->ip) {
            memcpy(link_data_t->ip, equip_udp_table->ip, strlen(equip_udp_table->ip) + 1);
        }
        link_data_t->port = equip_udp_table->port;

        ne_doip_sync_end(global_server_manager->equip_udp_list_sync);

        char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
            { global_server_manager->server->config->protocol_version,
             ~global_server_manager->server->config->protocol_version,
              0x40, 0x01, 0x00, 0x00, 0x00, 0x00 };

        ne_doip_pack(link_data_t, NE_DOIP_PAYLOAD_TYPE_ENTITY_STATUS_REQUEST,
                     header_data, NE_DOIP_HEADER_COMMON_LENGTH);
        if (link_data_t->ip != NULL) {
            free(link_data_t->ip);
        }
        free(link_data_t);
    }
}

void ne_doip_pack_power_mode_res(ne_doip_link_data_t *link_data, unsigned char power_mode)
{
    if (NE_DOIP_SOCKET_TYPE_IPC == link_data->comm_type) {
        NE_DOIP_PRINT("power mode response to internal test equipemt...\n");
        ne_doip_connection_t *conn = malloc(sizeof *conn);
        if (conn != NULL) {
            memset(conn, 0, sizeof *conn);
            uint8_t pos = 0;
            conn->out.data[pos] = NE_DOIP_IN_EQUIP_POWER_MODE;
            pos += NE_DOIP_IN_COMMAND_LENGTH;
            uint32_t payload_length = NE_DOIP_POWERMODE_LENGTH;
            memcpy(conn->out.data + pos, &payload_length, NE_DOIP_IN_DATA_LENGTH);
            pos += NE_DOIP_IN_DATA_LENGTH;
            memcpy(conn->out.data + pos, &power_mode, NE_DOIP_POWERMODE_LENGTH);
            conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
            conn->fd = link_data->fd;
            ne_doip_connection_write(conn);
            free(conn);
        }
    }
    else {
        char header_data[NE_DOIP_POWERMODE_INFO_RES_LENGTH] =
            { global_server_manager->server->config->protocol_version,
             ~global_server_manager->server->config->protocol_version,
              0x40, 0x04, 0x00, 0x00, 0x00, 0x01 };
        header_data[NE_DOIP_POWERMODE_INFO_RES_LENGTH - 1] = power_mode;
        ne_doip_pack(link_data, NE_DOIP_PAYLOAD_TYPE_POWER_MODE_INFO_RESPONSE,
                     header_data, NE_DOIP_POWERMODE_INFO_RES_LENGTH);
    }
}

void ne_doip_pack_power_mode_req(ne_doip_link_data_t *link_data, uint8_t pos)
{
    char eid[NE_DOIP_EID_SIZE] = { 0 };
    memcpy(eid, link_data->data + pos, NE_DOIP_EID_SIZE);

    uint8_t result = strncmp(global_server_manager->server->config->eid, eid, NE_DOIP_EID_SIZE);
    if (0 == result || 0 == strlen(eid)) {
        ne_doip_pack_power_mode_res(link_data, global_server_manager->server->power_mode);
    }
    else {
        // Find the test udp table by EID
        ne_doip_sync_start(global_server_manager->equip_udp_list_sync);
        ne_doip_equip_udp_table_t* equip_udp_table = ne_doip_equip_udp_list_find_by_eid(eid);
        if (NULL == equip_udp_table) {
            NE_DOIP_PRINT("<pack_power_mode_req step> get equip_udp_table is failed! \n");
            ne_doip_sync_end(global_server_manager->equip_udp_list_sync);
            return;
        }

        equip_udp_table->source_fd = link_data->fd;

        ne_doip_link_data_t* link_data_t = malloc(sizeof *link_data_t);
        memset(link_data_t, 0, sizeof *link_data_t);

        link_data_t->fd = equip_udp_table->fd;
        link_data_t->ip = (char*)malloc(strlen(equip_udp_table->ip) + 1);
        if (NULL != link_data_t->ip) {
            memcpy(link_data_t->ip, equip_udp_table->ip, strlen(equip_udp_table->ip) + 1);
        }
        link_data_t->port = equip_udp_table->port;

        ne_doip_sync_end(global_server_manager->equip_udp_list_sync);

        char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
            { global_server_manager->server->config->protocol_version,
             ~global_server_manager->server->config->protocol_version,
              0x40, 0x03, 0x00, 0x00, 0x00, 0x00 };

        ne_doip_pack(link_data_t, NE_DOIP_PAYLOAD_TYPE_POWER_MODE_INFO_REQUEST,
                     header_data, NE_DOIP_HEADER_COMMON_LENGTH);
        if (link_data_t->ip != NULL) {
            free(link_data_t->ip);
        }
        free(link_data_t);
    }
}

void ne_doip_egw_store_routing(ne_doip_link_data_t *link_data, uint8_t pos,
                               uint32_t payload_length, int fd)
{
    NE_DOIP_PRINT("ne_doip_egw_store_routing is enter...fd is %d.\n", fd);

    ne_doip_routing_data_t* routing_data = malloc(sizeof *routing_data);
    memset(routing_data, 0, sizeof *routing_data);

    routing_data->data = malloc(payload_length);
    memset(routing_data->data, 0, payload_length);
    routing_data->fd = fd;

    memcpy(routing_data->data, link_data->data + pos, payload_length);
    routing_data->data_size = payload_length;

    ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
    ne_doip_equip_tcp_table_t *equip_tcp_table = ne_doip_equip_tcp_list_find_by_fd(fd);
    if (NULL == equip_tcp_table) {
        free(routing_data->data);
        free(routing_data);
        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);
        return;
    }

    ne_doip_inser_queue(equip_tcp_table->queue_manager, routing_data);
    ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);

    free(routing_data->data);
    free(routing_data);
}

void ne_doip_egw_func_routing(ne_doip_link_data_t *link_data,
                              uint8_t pos, uint32_t payload_length,
                              ne_doip_list_t *list)
{
    NE_DOIP_PRINT("ne_doip_egw_func_routing is enter...\n");
    uint16_t entity_logical_address = 0x0000;
    if (NE_DOIP_SOCKET_TYPE_IPC == link_data->comm_type) {
        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t *equip_ipc_table = (ne_doip_equip_ipc_table_t*)list;
        entity_logical_address = equip_ipc_table->entity_logical_address;
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
    }
    else {
        ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
        ne_doip_node_tcp_table_t *node_tcp_table = (ne_doip_node_tcp_table_t*)list;
        entity_logical_address = node_tcp_table->entity_logical_address;
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
    }

    ne_doip_func_group_t *func_group = NULL;
    ne_doip_list_for_each(func_group, global_server_manager->server->config->func_group_list, base) {
        if (func_group->group_address == entity_logical_address) {
            NE_DOIP_PRINT("func_group->group_address [%04X] is match..\n", func_group->group_address);
            int i;
            for (i = 0; i < func_group->group_member_num; ++i) {
                ne_doip_routing_table_t *routing_table_t = NULL;
                ne_doip_list_for_each(routing_table_t, global_server_manager->server->config->routing_list, base) {
                    if (routing_table_t->entity_logical_address == func_group->logical_address_array[i]) {
                        if (strcmp(routing_table_t->ip, "127.0.0.1") != 0) {
                            ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
                            ne_doip_equip_tcp_table_t *equip_tcp_table = ne_doip_equip_tcp_list_find_by_fd(routing_table_t->fd);
                            if (NULL == equip_tcp_table) {
                                ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);
                                continue;
                            }
                            ne_doip_diag_routing_step_t current_step = equip_tcp_table->diag_routing_step;
                            ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);

                            if (NE_DOIP_DIAG_ROUTING_STEP_2 == current_step) {
                                ne_doip_link_data_t link_data_t;
                                memset(&link_data_t, 0, sizeof link_data_t);
                                link_data_t.fd = routing_table_t->fd;
                                link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TEST;
                                ne_doip_pack(&link_data_t, NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_FROM_EQUIP, link_data->data + pos, payload_length);
                            }
                            else {
                                ne_doip_egw_store_routing(link_data, pos, payload_length, routing_table_t->fd);
                            }
                        }
                        else {
                            ne_doip_connection_t *conn = malloc(sizeof *conn);
                            memset(conn, 0, sizeof *conn);
                            memcpy(conn->out.data, link_data->data + pos, payload_length);
                            conn->out.data_size = payload_length;
                            conn->fd = routing_table_t->fd;
                            ne_doip_connection_write(conn);
                            free(conn);
                        }
                    }
                }
            }
            break;
        }
    }
}

void ne_doip_egw_phys_routing(ne_doip_link_data_t *link_data,
                              uint8_t pos, uint32_t payload_length,
                              ne_doip_list_t *list)
{
    NE_DOIP_PRINT("ne_doip_egw_phys_routing is enter..\n");
    uint16_t entity_logical_address = 0x0000;
    int fd = 0;
    if (NE_DOIP_SOCKET_TYPE_IPC == link_data->comm_type) {
        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t *equip_ipc_table = (ne_doip_equip_ipc_table_t*)list;
        entity_logical_address = equip_ipc_table->entity_logical_address;
        fd = equip_ipc_table->cache_fd;
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
    }
    else {
        ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
        ne_doip_node_tcp_table_t *node_tcp_table = (ne_doip_node_tcp_table_t*)list;
        entity_logical_address = node_tcp_table->entity_logical_address;
        fd = node_tcp_table->cache_fd;
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
    }
    ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
    ne_doip_node_ipc_table_t *node_ipc_table = ne_doip_node_ipc_list_find_by_logic_address(entity_logical_address);
    ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
    if (NULL == node_ipc_table) {
        ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
        ne_doip_equip_tcp_table_t *equip_tcp_table = ne_doip_equip_tcp_list_find_by_logic_address(entity_logical_address);
        if (NULL == equip_tcp_table) {
            ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);
            return;
        }
        fd = equip_tcp_table->fd;
        ne_doip_diag_routing_step_t current_step = equip_tcp_table->diag_routing_step;
        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);

        if (NE_DOIP_DIAG_ROUTING_STEP_2 == current_step) {
            ne_doip_link_data_t link_data_t;
            memset(&link_data_t, 0, sizeof link_data_t);
            link_data_t.fd = fd;
            link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TEST;
            ne_doip_pack(&link_data_t, NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_FROM_EQUIP, link_data->data + pos, payload_length);
        }
        else {
            ne_doip_egw_store_routing(link_data, pos, payload_length, fd);
        }
    }
    else {
        ne_doip_connection_t *conn = malloc(sizeof *conn);
        memset(conn, 0, sizeof *conn);

        memcpy(conn->out.data, link_data->data + pos, payload_length);
        conn->out.data_size = payload_length;
        conn->fd = fd;
        ne_doip_connection_write(conn);
        free(conn);
    }
}

void ne_doip_sub_diagnostic_to_self(ne_doip_link_data_t *link_data, uint8_t pos,
                                    uint32_t payload_length, uint32_t total_length,
                                    uint16_t equip_logical_address,
                                    ne_doip_node_ipc_table_t *node_ipc_table,
                                    ne_doip_ta_type_t ta_type)
{
    uint32_t user_data_size = payload_length - NE_DOIP_LOGICAL_ADDRESS_LENGTH * 2;
    if (total_length > global_server_manager->server->config->mds) {
        NE_DOIP_PRINT("[%s] diagnostic message size is generally supported by the target network/TP.\n", __FUNCTION__);
        ne_doip_pack_diagnostic_negative_ack(link_data, node_ipc_table->ecu_logical_address,
                                             equip_logical_address,
                                             NE_DOIP_DIAGNOSTIC_NACK_DIAG_MSG_TOO_LARGE);
    }
    else if (user_data_size > NE_DOIP_MAX_BUFFER_SIZE) {
        NE_DOIP_PRINT("[%s] buffer can not handle the diagnostic message.\n", __FUNCTION__);
        ne_doip_pack_diagnostic_negative_ack(link_data, node_ipc_table->ecu_logical_address,
                                             equip_logical_address,
                                             NE_DOIP_DIAGNOSTIC_NACK_OUT_OF_MEMORY);
    }
    else {
        ne_doip_connection_t *conn = malloc(sizeof *conn);
        memset(conn, 0, sizeof *conn);

        uint8_t pos_t = 0;
        conn->out.data[pos_t] = NE_DOIP_IN_PAYLOADTYPE_DIAG_INDICATION;
        pos_t += NE_DOIP_IN_COMMAND_LENGTH;
        memcpy(conn->out.data + pos_t, &total_length, NE_DOIP_IN_DATA_LENGTH);
        pos_t += NE_DOIP_IN_DATA_LENGTH;
        memcpy(conn->out.data + pos_t, &ta_type, NE_DOIP_TA_TYPE_LENGTH);
        pos_t += NE_DOIP_TA_TYPE_LENGTH;
        if (NE_DOIP_SOCKET_TYPE_IPC == link_data->comm_type) {
            memcpy(conn->out.data + pos_t, link_data->data + pos + NE_DOIP_TA_TYPE_LENGTH, payload_length);
        }
        else {
            memcpy(conn->out.data + pos_t, link_data->data + pos, payload_length);
        }
        conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + NE_DOIP_TA_TYPE_LENGTH + payload_length;
        conn->fd = node_ipc_table->fd;
        int num = ne_doip_connection_write(conn);
        if (num != (int)conn->out.data_size) {
            ne_doip_pack_diagnostic_negative_ack(link_data, node_ipc_table->ecu_logical_address,
                                                 equip_logical_address,
                                                 NE_DOIP_DIAGNOSTIC_NACK_TRANS_PROTO_ERROR);
        }
        else {
            ne_doip_pack_diagnostic_positive_ack(link_data, node_ipc_table->ecu_logical_address,
                                                 equip_logical_address);
        }
        free(conn);
    }
}

void ne_doip_sub_diagnostic_to_other(ne_doip_link_data_t *link_data, uint8_t pos,
                                     uint32_t payload_length, uint32_t total_length,
                                     ne_doip_routing_table_t *routing_table,
                                     unsigned char functional_addressing_flag)
{
    if (routing_table->fd > 0) {
        // Direct transmission of diagnostic data when routing is activated
        ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
        ne_doip_equip_tcp_table_t *equip_tcp_table = ne_doip_equip_tcp_list_find_by_fd(routing_table->fd);
        if (equip_tcp_table != NULL) {
            equip_tcp_table->diag_routing_step = NE_DOIP_DIAG_ROUTING_STEP_2;
        }
        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);

        NE_DOIP_PRINT("Routing activation has been performed, and fd is valid, and data can be sent directly.\n");
        ne_doip_link_data_t link_data_t;
        memset(&link_data_t, 0, sizeof link_data_t);
        link_data_t.fd = routing_table->fd;
        link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TEST;

        uint32_t data_size = NE_DOIP_HEADER_COMMON_LENGTH + payload_length;
        char* data = malloc(data_size);
        memset(data, 0, data_size);

        ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
        ne_doip_node_ipc_table_t *node_ipc_table = ne_doip_node_ipc_list_find_entity();
        if (NULL == node_ipc_table) {
            NE_DOIP_PRINT("<sub_diagnostic_to_other step> get entity ipc table is failed! \n");
            free(data);
            ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
            return;
        }

        uint16_t logical_address = ne_doip_bswap_16(node_ipc_table->entity_logical_address);
        uint16_t target_logical_address = ne_doip_bswap_16(routing_table->entity_logical_address);
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);

        if (NE_DOIP_SOCKET_TYPE_IPC == link_data->comm_type) {
            char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
                { global_server_manager->server->config->protocol_version,
                 ~global_server_manager->server->config->protocol_version,
                  0x80, 0x01, 0x00, 0x00, 0x00, 0x00 };

            ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
            ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(link_data->fd);
            if (NULL == equip_ipc_table) {
                free(data);
                ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
                return;
            }
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

            uint8_t i;
            for (i = 0; i < 4; ++i) {
                header_data[NE_DOIP_HEADER_COMMON_LENGTH - i -1] = (total_length >> (i * 8)) & 0xFF;
            }

            memcpy(data, header_data, NE_DOIP_HEADER_COMMON_LENGTH);
            memcpy(data + NE_DOIP_HEADER_COMMON_LENGTH, link_data->data + pos + NE_DOIP_TA_TYPE_LENGTH, payload_length);
            memcpy(data + NE_DOIP_HEADER_COMMON_LENGTH, &logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            memcpy(data + NE_DOIP_HEADER_COMMON_LENGTH + NE_DOIP_LOGICAL_ADDRESS_LENGTH, &target_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        }
        else { 
            memcpy(data, link_data->data + pos - NE_DOIP_HEADER_COMMON_LENGTH, data_size);
            memcpy(data + NE_DOIP_HEADER_COMMON_LENGTH, &logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            memcpy(data + NE_DOIP_HEADER_COMMON_LENGTH + NE_DOIP_LOGICAL_ADDRESS_LENGTH, &target_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        }

        ne_doip_pack(&link_data_t, NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_FROM_EQUIP, data, data_size);
        free(data);
    }
    else {
        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);
        NE_DOIP_PRINT("Cache data and routing activation.\n");
        ne_doip_routing_data_t* routing_data = malloc(sizeof *routing_data);
        memset(routing_data, 0, sizeof *routing_data);

        uint32_t routing_data_size = NE_DOIP_HEADER_COMMON_LENGTH + payload_length;
        routing_data->data = malloc(routing_data_size);
        memset(routing_data->data, 0, routing_data_size);

        ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
        ne_doip_node_ipc_table_t *node_ipc_table = ne_doip_node_ipc_list_find_entity();
        if (NULL == node_ipc_table) {
            NE_DOIP_PRINT("<sub_diagnostic_to_other step> get entity ipc table is failed! \n");
            free(routing_data->data);
            free(routing_data);
            ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
            return;
        }

        uint16_t logical_address = ne_doip_bswap_16(node_ipc_table->entity_logical_address);
        uint16_t target_logical_address = ne_doip_bswap_16(routing_table->entity_logical_address);
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);

        if (NE_DOIP_SOCKET_TYPE_IPC == link_data->comm_type) {
            ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
            ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(link_data->fd);
            if (NULL == equip_ipc_table) {
                free(routing_data->data);
                free(routing_data);
                ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
                return;
            }
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

            char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
                { global_server_manager->server->config->protocol_version,
                 ~global_server_manager->server->config->protocol_version,
                  0x80, 0x01, 0x00, 0x00, 0x00, 0x00 };

            uint8_t i;
            for (i = 0; i < 4; ++i) {
                header_data[NE_DOIP_HEADER_COMMON_LENGTH - i -1] = (total_length >> (i * 8)) & 0xFF;
            }

            memcpy(routing_data->data, header_data, NE_DOIP_HEADER_COMMON_LENGTH);
            memcpy(routing_data->data + NE_DOIP_HEADER_COMMON_LENGTH, link_data->data + pos + NE_DOIP_TA_TYPE_LENGTH, payload_length);
            memcpy(routing_data->data + NE_DOIP_HEADER_COMMON_LENGTH, &logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            memcpy(routing_data->data + NE_DOIP_HEADER_COMMON_LENGTH + NE_DOIP_LOGICAL_ADDRESS_LENGTH, &target_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        }
        else {
            memcpy(routing_data->data, link_data->data + pos - NE_DOIP_HEADER_COMMON_LENGTH, routing_data_size);
            memcpy(routing_data->data + NE_DOIP_HEADER_COMMON_LENGTH, &logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            memcpy(routing_data->data + NE_DOIP_HEADER_COMMON_LENGTH + NE_DOIP_LOGICAL_ADDRESS_LENGTH, &target_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        }

        routing_data->data_size = routing_data_size;

        if (NE_DOIP_NET_TYPE_IPV6 == global_server_manager->server->config->net_type) {
            routing_data->fd = ne_doip_test_tcp6_create(global_server_manager->server,
                                                        routing_table->ip, NE_DOIP_TCP_DATA_PORT);
        }
        else if (NE_DOIP_NET_TYPE_IPV4 == global_server_manager->server->config->net_type) {
            routing_data->fd = ne_doip_test_tcp_create(global_server_manager->server,
                                                       routing_table->ip, NE_DOIP_TCP_DATA_PORT);
        }
        NE_DOIP_PRINT("create test tcp socket fd is [%d]...\n", routing_data->fd);
        if (-1 == routing_data->fd) {

            if (NE_DOIP_SOCKET_TYPE_IPC == link_data->comm_type) {

                uint16_t logical_source_address = 0x0000;
                uint16_t logical_target_address = 0x0000;
                memcpy(&logical_target_address, link_data->data + pos + NE_DOIP_TA_TYPE_LENGTH, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
                memcpy(&logical_source_address, link_data->data + pos + NE_DOIP_TA_TYPE_LENGTH + NE_DOIP_LOGICAL_ADDRESS_LENGTH, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
                logical_source_address = ne_doip_bswap_16(logical_source_address);
                logical_target_address = ne_doip_bswap_16(logical_target_address);
                ne_doip_pack_diagnostic_negative_ack(link_data, logical_source_address,
                                                     logical_target_address,
                                                     NE_DOIP_DIAGNOSTIC_NACK_TARGET_UNREACHABLE);
            }
            if (NE_DOIP_SOCKET_TYPE_TCP == link_data->comm_type) {
                uint16_t logical_source_address = 0x0000;
                uint16_t logical_target_address = 0x0000;
                memcpy(&logical_target_address, link_data->data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
                memcpy(&logical_source_address, link_data->data + pos + NE_DOIP_LOGICAL_ADDRESS_LENGTH, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
                logical_source_address = ne_doip_bswap_16(logical_source_address);
                logical_target_address = ne_doip_bswap_16(logical_target_address);
                ne_doip_pack_diagnostic_negative_ack(link_data, logical_source_address,
                                                     logical_target_address,
                                                     NE_DOIP_DIAGNOSTIC_NACK_TARGET_UNREACHABLE);
            }
            free(routing_data->data);
            free(routing_data);
            return;
        }

        routing_table->fd = routing_data->fd;
        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(link_data->fd);
        if (equip_ipc_table != NULL) {
            equip_ipc_table->tcp_fd = routing_data->fd;
        }
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

        ne_doip_addr_data_t* addr_data = malloc(sizeof *addr_data);
        memset(addr_data, 0, sizeof *addr_data);
        if (addr_data != NULL) {
            addr_data->fd = routing_data->fd;
            addr_data->comm_type = NE_DOIP_SOCKET_TYPE_TEST;
            addr_data->ip = (char*)malloc(strlen(routing_table->ip) + 1);
            if (addr_data->ip != NULL) {
                memcpy(addr_data->ip, routing_table->ip, strlen(routing_table->ip) + 1);
            }
            addr_data->port = NE_DOIP_TCP_DATA_PORT;
            ne_doip_add_connection_table(addr_data);
            if (addr_data->ip != NULL) {
                free(addr_data->ip);
            }
            free(addr_data);
        }

        ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
        ne_doip_equip_tcp_table_t *equip_tcp_table = ne_doip_equip_tcp_list_find_by_fd(routing_data->fd);
        if (equip_tcp_table != NULL) {
            equip_tcp_table->source_fd = link_data->fd;
            equip_tcp_table->entity_logical_address = ne_doip_bswap_16(target_logical_address);
            equip_tcp_table->queue_manager = ne_doip_queue_init();
        }
        // Route activation after diagnostic data is inserted into the cache queue
        ne_doip_inser_queue(equip_tcp_table->queue_manager, routing_data);
        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);

        ne_doip_link_data_t link_data_t;
        memset(&link_data_t, 0, sizeof link_data_t);
        link_data_t.fd = routing_data->fd;
        link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TEST;
        free(routing_data->data);
        free(routing_data);

        // pack routing activation request message
        uint32_t data_size = NE_DOIP_HEADER_COMMON_LENGTH + NE_DOIP_ROUTING_ACTIVATION_MAND_LENGTH;
        char* data = (char*)malloc(data_size);
        memset(data, 0, data_size);

        char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
            { global_server_manager->server->config->protocol_version,
             ~global_server_manager->server->config->protocol_version,
              0x00, 0x05, 0x00, 0x00, 0x00, 0x07 };

        memcpy(data, header_data, NE_DOIP_HEADER_COMMON_LENGTH);
        uint8_t pos_t = NE_DOIP_HEADER_COMMON_LENGTH;

        memcpy(data + pos_t, &logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        pos_t += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        data[pos_t] = 0x00;
        pos_t += NE_DOIP_ACTIVATION_TYPE_LENGTH;
        memset(data + pos_t, 0, NE_DOIP_S_RESERVED_LENGTH);

        ne_doip_pack(&link_data_t, NE_DOIP_PAYLOAD_TYPE_ROUTING_ACTIVE_REQUEST, data, data_size);
        free(data);
    }
}

void ne_doip_sub_diagnostic_functional(ne_doip_link_data_t *link_data, uint8_t pos,
                                       uint32_t payload_length, uint32_t total_length,
                                       uint16_t entity_logical_address,
                                       uint16_t equip_logical_address)
{
    if (NE_DOIP_TRUE == global_server_manager->server->config->egw_control
        && NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server_manager->server->config->entity_type) {
        ne_doip_func_group_t *func_group = NULL;
        ne_doip_list_for_each(func_group, global_server_manager->server->config->func_group_list, base) {
            if (func_group->group_address == entity_logical_address) {
                int i;
                for (i = 0; i < func_group->group_member_num; ++i) {
                    ne_doip_routing_table_t *routing_table_t = NULL;
                    ne_doip_list_for_each(routing_table_t, global_server_manager->server->config->routing_list, base) {
                        if (routing_table_t->entity_logical_address == func_group->logical_address_array[i]) {
                            if (strcmp(routing_table_t->ip, "127.0.0.1") != 0) {
                                ne_doip_sub_diagnostic_to_other(link_data, pos, payload_length, total_length, routing_table_t, NE_DOIP_TRUE);
                            }
                            else {
                                ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
                                ne_doip_node_ipc_table_t* node_ipc_table = ne_doip_node_ipc_list_find_by_logic_address(func_group->logical_address_array[i]);
                                if (node_ipc_table != NULL) {
                                    routing_table_t->fd = node_ipc_table->fd;
                                    ne_doip_sub_diagnostic_to_self(link_data, pos, payload_length, total_length, equip_logical_address, node_ipc_table, NE_DOIP_TA_TYPE_FUNCTIONAL);
                                }
                                ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
                            }
                        }
                    }
                }
                break;
            }
        }
    }
    else {
        uint8_t num = ne_doip_list_length(global_server_manager->node_ipc_list);
        if (0 == num) {
            ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
            ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(link_data->fd);

            if (NULL == equip_ipc_table) {
                ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
                return;
            }

            char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
                { global_server_manager->server->config->protocol_version,
                 ~global_server_manager->server->config->protocol_version,
                  0x80, 0x01, 0x00, 0x00, 0x00, 0x00 };

            ne_doip_link_data_t link_data_t;
            memset(&link_data_t, 0, sizeof link_data_t);
            link_data_t.fd = equip_ipc_table->tcp_fd;
            equip_ipc_table->cache_fd = equip_ipc_table->tcp_fd;
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

            uint8_t i;
            for (i = 0; i < 4; ++i) {
                header_data[NE_DOIP_HEADER_COMMON_LENGTH - i -1] = (total_length >> (i * 8)) & 0xFF;
            }

            uint32_t data_size = NE_DOIP_HEADER_COMMON_LENGTH + payload_length;
            char* data = (char*)malloc(data_size);
            memset(data, 0, data_size);

            memcpy(data, header_data, NE_DOIP_HEADER_COMMON_LENGTH);
            memcpy(data + NE_DOIP_HEADER_COMMON_LENGTH, link_data->data + pos + NE_DOIP_TA_TYPE_LENGTH, payload_length);
            ne_doip_pack(&link_data_t, NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_FROM_EQUIP, data, data_size);
            free(data);
        }
        else {
            ne_doip_func_group_t *func_group = NULL;
            ne_doip_list_for_each(func_group, global_server_manager->server->config->func_group_list, base) {
                if (func_group->group_address == entity_logical_address) {
                    int i;
                    for (i = 0; i < func_group->group_member_num; ++i) {
                        ne_doip_routing_table_t *routing_table_t = NULL;
                        ne_doip_list_for_each(routing_table_t, global_server_manager->server->config->routing_list, base) {
                            if (routing_table_t->entity_logical_address == func_group->logical_address_array[i]) {
                                ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
                                ne_doip_node_ipc_table_t* node_ipc_table = ne_doip_node_ipc_list_find_by_logic_address(func_group->logical_address_array[i]);
                                if (node_ipc_table != NULL) {
                                    routing_table_t->fd = node_ipc_table->fd;
                                    ne_doip_sub_diagnostic_to_self(link_data, pos, payload_length, total_length, equip_logical_address, node_ipc_table, NE_DOIP_TA_TYPE_FUNCTIONAL);
                                }
                                ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
}

void ne_doip_pack_diagnostic_from_internal_equip(ne_doip_link_data_t *link_data, uint8_t pos, uint32_t payload_length)
{
    ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
    ne_doip_equip_ipc_table_t* equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(link_data->fd);
    if (equip_ipc_table != NULL) {
        if (equip_ipc_table->diag_data_current_pos > 0) {
            // Large Data Diagnosis
            equip_ipc_table->diag_data_current_pos += payload_length;
            NE_DOIP_PRINT("send diag_data_total_length is [%u byte], diag_data_current_pos is [%u byte]..\n",
                equip_ipc_table->diag_data_total_length, equip_ipc_table->diag_data_current_pos);

            if (equip_ipc_table->diag_data_current_pos == equip_ipc_table->diag_data_total_length) {
                equip_ipc_table->diag_data_current_pos = 0;
                equip_ipc_table->diag_data_total_length = 0;
            }   

            if (NE_DOIP_TRUE == global_server_manager->server->config->egw_control
                && NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server_manager->server->config->entity_type) {

                if (NE_DOIP_TRUE == equip_ipc_table->functional_addressing_flag) {
                    ne_doip_egw_func_routing(link_data, pos, payload_length, (ne_doip_list_t*)equip_ipc_table);
                }
                else {
                    ne_doip_egw_phys_routing(link_data, pos, payload_length, (ne_doip_list_t*)equip_ipc_table);
                }
            }
            else {
                ne_doip_connection_t *conn = malloc(sizeof *conn);
                if (conn != NULL) {
                    memset(conn, 0, sizeof *conn);
                    memcpy(conn->out.data, link_data->data + pos, payload_length);
                    conn->out.data_size = payload_length;
                    conn->fd = equip_ipc_table->cache_fd;
                    ne_doip_connection_write(conn);
                    free(conn);
                }
            }
        }
        else {
            // Small Data Diagnosis
            equip_ipc_table->diag_data_current_pos += payload_length;
            uint32_t total_length_tmp = equip_ipc_table->diag_data_total_length;
            NE_DOIP_PRINT("send diag_data_total_length is [%u byte], diag_data_current_pos is [%u byte]..\n",
                equip_ipc_table->diag_data_total_length, equip_ipc_table->diag_data_current_pos);

            if (equip_ipc_table->diag_data_current_pos == equip_ipc_table->diag_data_total_length) {
                equip_ipc_table->diag_data_total_length = 0;
                equip_ipc_table->diag_data_current_pos = 0;
            }

            ne_doip_ta_type_t ta_type = NE_DOIP_TA_TYPE_PHYSICAL;
            memcpy(&ta_type, link_data->data + pos, NE_DOIP_TA_TYPE_LENGTH);
            uint16_t logical_source_address = 0x0000;
            memcpy(&logical_source_address, link_data->data + pos + NE_DOIP_TA_TYPE_LENGTH, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            logical_source_address = ne_doip_bswap_16(logical_source_address);
            uint16_t logical_target_address = 0x0000;
            memcpy(&logical_target_address, link_data->data + pos + NE_DOIP_TA_TYPE_LENGTH + NE_DOIP_LOGICAL_ADDRESS_LENGTH, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            logical_target_address = ne_doip_bswap_16(logical_target_address);

            if (NE_DOIP_TA_TYPE_FUNCTIONAL == ta_type) {
                // Functional diagnosis
                if (NE_DOIP_TRUE == ne_doip_is_functianal_address(global_server_manager->server->config, logical_target_address)) {
                    // equip_ipc_table->diag_routing_step = NE_DOIP_DIAG_ROUTING_STEP_2;
                    equip_ipc_table->functional_addressing_flag = NE_DOIP_TRUE;
                    equip_ipc_table->entity_logical_address = logical_target_address;

                    ne_doip_sub_diagnostic_functional(link_data, pos, payload_length,
                                                      total_length_tmp, logical_target_address,
                                                      logical_source_address);
                }
                else {
                    NE_DOIP_PRINT("diagnostic functional address is invalid!\n");
                    ne_doip_pack_diagnostic_negative_ack(link_data, logical_target_address,
                                                         logical_source_address,
                                                         NE_DOIP_DIAGNOSTIC_NACK_UNKNOWN_TA);
                }
            }
            else {
                // Physical diagnosis
                uint8_t res = ne_doip_is_functianal_address(global_server_manager->server->config, logical_target_address);
                if (res == NE_DOIP_TRUE) {
                    ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
                    ne_doip_pack_diagnostic_negative_ack(link_data, logical_target_address,
                                                         logical_source_address,
                                                         NE_DOIP_DIAGNOSTIC_NACK_UNKNOWN_TA);
                   return;
                }

                ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
                ne_doip_node_ipc_table_t *node_ipc_table = ne_doip_node_ipc_list_find_by_logic_address(logical_target_address);
                if (NULL == node_ipc_table) {
                    // external test equpiment
                    ne_doip_sync_end(global_server_manager->node_ipc_list_sync);

                    if (NE_DOIP_TRUE == global_server_manager->server->config->egw_control
                        && NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server_manager->server->config->entity_type) {

                        ne_doip_routing_table_t *routing_table = ne_doip_routing_list_find_by_logic_address(logical_target_address);
                        if (routing_table != NULL) {
                            NE_DOIP_PRINT("Edge gatewey routing start..to [%04X]..\n", logical_target_address);
                            equip_ipc_table->functional_addressing_flag = NE_DOIP_FALSE;
                            equip_ipc_table->entity_logical_address = logical_target_address;
                            ne_doip_sub_diagnostic_to_other(link_data, pos, payload_length, total_length_tmp,
                                                            routing_table, NE_DOIP_FALSE);
                        }
                        else {
                            NE_DOIP_PRINT("[%s] The TA[%04X] is unknown.\n", __FUNCTION__, logical_target_address);
                            ne_doip_pack_diagnostic_negative_ack(link_data, logical_target_address,
                                                                 logical_source_address,
                                                                 NE_DOIP_DIAGNOSTIC_NACK_UNKNOWN_TA);
                        }
                    }
                    else {
                        char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
                            { global_server_manager->server->config->protocol_version,
                             ~global_server_manager->server->config->protocol_version,
                              0x80, 0x01, 0x00, 0x00, 0x00, 0x00 };

                        ne_doip_link_data_t link_data_t;
                        memset(&link_data_t, 0, sizeof link_data_t);
                        link_data_t.fd = equip_ipc_table->tcp_fd;
                        equip_ipc_table->cache_fd = equip_ipc_table->tcp_fd;

                        uint8_t i;
                        for (i = 0; i < 4; ++i) {
                            header_data[NE_DOIP_HEADER_COMMON_LENGTH - i -1] = (total_length_tmp >> (i * 8)) & 0xFF;
                        }

                        uint32_t data_size = NE_DOIP_HEADER_COMMON_LENGTH + payload_length;
                        char* data = (char*)malloc(data_size);
                        memset(data, 0, data_size);

                        memcpy(data, header_data, NE_DOIP_HEADER_COMMON_LENGTH);
                        memcpy(data + NE_DOIP_HEADER_COMMON_LENGTH, link_data->data + pos + NE_DOIP_TA_TYPE_LENGTH, payload_length);
                        ne_doip_pack(&link_data_t, NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_FROM_EQUIP, data, data_size);
                        free(data);
                    }
                }
                else {
                    // internal test equipment(diagnosis self node)
                    NE_DOIP_PRINT("[diagnostic]target address is %04X, to doip node\n", node_ipc_table->ecu_logical_address);
                    int fd = node_ipc_table->fd;
                    equip_ipc_table->cache_fd = node_ipc_table->fd;
                    equip_ipc_table->functional_addressing_flag = NE_DOIP_FALSE;
                    equip_ipc_table->entity_logical_address = logical_target_address;
                    ne_doip_sync_end(global_server_manager->node_ipc_list_sync);

                    ne_doip_pack_diagnostic_positive_ack(link_data, logical_target_address, logical_source_address);

                    // This case is to diagnose the internal doip node in the LAN.
                    ne_doip_connection_t *connection = malloc(sizeof *connection);
                    if (connection != NULL) {
                        memset(connection, 0, sizeof *connection);
                        connection->fd = fd;
                        connection->out.data_size = payload_length + NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_TA_TYPE_LENGTH + NE_DOIP_IN_DATA_LENGTH;
                        uint8_t pos_t = 0;
                        connection->out.data[pos_t] = NE_DOIP_IN_PAYLOADTYPE_DIAG_INDICATION;
                        pos_t += NE_DOIP_IN_COMMAND_LENGTH;
                        memcpy(connection->out.data + pos_t, link_data->data + pos - NE_DOIP_IN_DATA_LENGTH - NE_DOIP_BLANK_2_LENGTH, NE_DOIP_IN_DATA_LENGTH);
                        pos_t += NE_DOIP_IN_DATA_LENGTH;
                        memcpy(connection->out.data + pos_t, &ta_type, NE_DOIP_TA_TYPE_LENGTH);
                        pos_t += NE_DOIP_TA_TYPE_LENGTH;
                        memcpy(connection->out.data + pos_t, link_data->data + pos + NE_DOIP_TA_TYPE_LENGTH, payload_length);
                        ne_doip_connection_write(connection);
                        free(connection);
                    }
                }
            }
        }
    }
    ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
}

void ne_doip_pack_header_negative_ack(ne_doip_link_data_t *link_data,
                                      ne_doip_header_nack_code_t code)
{
    if (code < NE_DOIP_HEADER_NACK_INCORRECT_PATTERN_FORMAT 
        || code > NE_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH) {
        NE_DOIP_PRINT("[%s] parameter is invalid! ", __FUNCTION__);
        return;
    }

    char header_nack_data[NE_DOIP_HEADER_NEGATIVE_ACK_LENGTH] =
        { global_server_manager->server->config->protocol_version,
         ~global_server_manager->server->config->protocol_version,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
    header_nack_data[NE_DOIP_HEADER_NEGATIVE_ACK_LENGTH - 1] = code;

    ne_doip_pack(link_data, NE_DOIP_PAYLOAD_TYPE_HEADER_NEGATIVE_ACK,
                 header_nack_data, NE_DOIP_HEADER_NEGATIVE_ACK_LENGTH);

    if (NE_DOIP_SOCKET_TYPE_TCP == link_data->comm_type) {
        if (NE_DOIP_HEADER_NACK_INCORRECT_PATTERN_FORMAT == code
            || NE_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH == code) {
            ne_doip_server_disconnect(global_server_manager->server, NE_DOIP_SOCKET_TYPE_TCP, link_data->fd);
        }
    }
}

void ne_doip_pack_alive_check_req(ne_doip_link_data_t *link_data)
{
    char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
        { global_server_manager->server->config->protocol_version,
         ~global_server_manager->server->config->protocol_version,
          0x00, 0x07, 0x00, 0x00, 0x00, 0x00 };

    ne_doip_pack(link_data, NE_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_REQUEST,
                 header_data, NE_DOIP_HEADER_COMMON_LENGTH);
}

void ne_doip_pack_diagnostic_confirmation(int fd, uint16_t equip_logical_address,
                                          uint16_t ecu_logical_address)
{
    ne_doip_connection_t *connection = malloc(sizeof *connection);
    memset(connection, 0, sizeof *connection);

    int8_t pos = 0;
    connection->out.data[pos] = NE_DOIP_IN_PAYLOADTYPE_DIAG_CONFIRM;
    pos += NE_DOIP_IN_COMMAND_LENGTH;
    uint32_t payload_length = NE_DOIP_LOGICAL_ADDRESS_LENGTH * 2;
    memcpy(connection->out.data + pos,  &payload_length, NE_DOIP_IN_DATA_LENGTH);
    pos += NE_DOIP_IN_DATA_LENGTH;
    equip_logical_address = ne_doip_bswap_16(equip_logical_address);
    memcpy(connection->out.data + pos, &equip_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    ecu_logical_address = ne_doip_bswap_16(ecu_logical_address);
    memcpy(connection->out.data + pos, &ecu_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    connection->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
    connection->fd = fd;
    ne_doip_connection_write(connection);
    free(connection);
}

void ne_doip_pack_diagnostic_positive_ack(ne_doip_link_data_t *link_data,
                                          uint16_t entity_logical_address,
                                          uint16_t equip_logical_address)
{
    NE_DOIP_PRINT("ne_doip_pack_diagnostic_positive_ack is enter...\n");
    if (NE_DOIP_SOCKET_TYPE_IPC == link_data->comm_type) {
        // to internal equipment
        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_logic_address(equip_logical_address);
        if (NULL == equip_ipc_table) {
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
            return;
        }
        int fd = equip_ipc_table->fd;
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

        ne_doip_connection_t *conn = malloc(sizeof *conn);
        memset(conn, 0, sizeof *conn);

        uint8_t pos_t = 0;
        conn->out.data[pos_t] = NE_DOIP_IN_EQUIP_DIAGNOSTIC_PACK;
        pos_t += NE_DOIP_IN_COMMAND_LENGTH;
        uint32_t payload_length = NE_DOIP_LOGICAL_ADDRESS_LENGTH * 2 + NE_DOIP_ACK_CODE_LENGTH;
        memcpy(conn->out.data + pos_t,  &payload_length, NE_DOIP_IN_DATA_LENGTH);
        pos_t += NE_DOIP_IN_DATA_LENGTH;
        entity_logical_address = ne_doip_bswap_16(entity_logical_address);
        memcpy(conn->out.data + pos_t, &entity_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        pos_t += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        equip_logical_address = ne_doip_bswap_16(equip_logical_address);
        memcpy(conn->out.data + pos_t, &equip_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        pos_t += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        conn->out.data[pos_t] = 0x00; // positive ack code 0x00
        conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
        conn->fd = fd;
        ne_doip_connection_write(conn);
        free(conn);
    }
    else {
        ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
        ne_doip_node_tcp_table_t *node_tcp_table = ne_doip_node_tcp_list_find_by_fd(link_data->fd);
        if (NULL == node_tcp_table) {
            NE_DOIP_PRINT("<pack_diagnostic_positive_ack step> get tcp table is failed! by fd[%d].\n", link_data->fd);
            ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
            return;
        }
        else {
            // to external equipment
            ne_doip_link_data_t link_data_t;
            memset(&link_data_t, 0, sizeof link_data_t);
            link_data_t.fd = node_tcp_table->fd;
            link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TCP;
            ne_doip_sync_end(global_server_manager->node_tcp_list_sync);

            char* send_data = (char*)malloc(NE_DOIP_DIAG_POSITIVE_ACK_LENGTH);
            memset(send_data, 0, NE_DOIP_DIAG_POSITIVE_ACK_LENGTH);

            char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
                { global_server_manager->server->config->protocol_version,
                 ~global_server_manager->server->config->protocol_version,
                  0x80, 0x02, 0x00, 0x00, 0x00, 0x05 };

            uint8_t pos = 0;
            memcpy(send_data, header_data, NE_DOIP_HEADER_COMMON_LENGTH);
            pos += NE_DOIP_HEADER_COMMON_LENGTH;
            entity_logical_address = ne_doip_bswap_16(entity_logical_address);
            memcpy(send_data + pos, &entity_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
            equip_logical_address = ne_doip_bswap_16(equip_logical_address);
            memcpy(send_data + pos, &equip_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
            send_data[pos] = 0x00; // positive ack code 0x00

            ne_doip_pack(&link_data_t, NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_POSITIVE_ACK,
                         send_data, NE_DOIP_DIAG_POSITIVE_ACK_LENGTH);
            free(send_data);
        }
    }
}

void ne_doip_pack_diagnostic_negative_ack(ne_doip_link_data_t *link_data, uint16_t entity_logical_address,
                                          uint16_t equip_logical_address, ne_doip_diagnostic_nack_code_t code)
{
    NE_DOIP_PRINT("ne_doip_pack_diagnostic_negative_ack is enter...\n");
    if (code < NE_DOIP_DIAGNOSTIC_NACK_INVALID_SA || code > NE_DOIP_DIAGNOSTIC_NACK_TRANS_PROTO_ERROR) {
        NE_DOIP_PRINT("[%s] parameter is invalid! ", __FUNCTION__);
        return;
    }

    if (NE_DOIP_SOCKET_TYPE_IPC == link_data->comm_type) {
        // to internal equipment
        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_logic_address(equip_logical_address);
        if (NULL == equip_ipc_table) {
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
            return;
        }
        int fd = equip_ipc_table->fd;
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

        ne_doip_connection_t *conn = malloc(sizeof *conn);
        memset(conn, 0, sizeof *conn);

        uint8_t pos_t = 0;
        conn->out.data[pos_t] = NE_DOIP_IN_EQUIP_DIAGNOSTIC_NACK;
        pos_t += NE_DOIP_IN_COMMAND_LENGTH;
        uint32_t payload_length = NE_DOIP_LOGICAL_ADDRESS_LENGTH * 2 + NE_DOIP_NACK_CODE_LENGTH;
        memcpy(conn->out.data + pos_t,  &payload_length, NE_DOIP_IN_DATA_LENGTH);
        pos_t += NE_DOIP_IN_DATA_LENGTH;
        entity_logical_address = ne_doip_bswap_16(entity_logical_address);
        memcpy(conn->out.data + pos_t, &entity_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        pos_t += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        equip_logical_address = ne_doip_bswap_16(equip_logical_address);
        memcpy(conn->out.data + pos_t, &equip_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        pos_t += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        conn->out.data[pos_t] = code;
        conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
        conn->fd = fd;
        ne_doip_connection_write(conn);
        free(conn);
    }
    else {
        ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
        ne_doip_node_tcp_table_t *node_tcp_table = ne_doip_node_tcp_list_find_by_fd(link_data->fd);
        if (NULL == node_tcp_table) {
            NE_DOIP_PRINT("<ne_doip_pack_diagnostic_negative_ack step> get tcp table is failed! by fd[%d].\n", link_data->fd);
            ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
            return;
        }
        else {
            // to external equipment
            ne_doip_link_data_t link_data_t;
            memset(&link_data_t, 0, sizeof link_data_t);
            link_data_t.fd = node_tcp_table->fd;
            link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TCP;
            ne_doip_sync_end(global_server_manager->node_tcp_list_sync);

            char* send_data = (char*)malloc(NE_DOIP_DIAG_NEGATIVE_ACK_LENGTH);
            memset(send_data, 0, NE_DOIP_DIAG_NEGATIVE_ACK_LENGTH);

            char header_data[NE_DOIP_HEADER_COMMON_LENGTH] =
                { global_server_manager->server->config->protocol_version,
                 ~global_server_manager->server->config->protocol_version,
                  0x80, 0x03, 0x00, 0x00, 0x00, 0x05 };

            entity_logical_address = ne_doip_bswap_16(entity_logical_address);
            equip_logical_address = ne_doip_bswap_16(equip_logical_address);

            uint8_t pos = 0;
            memcpy(send_data, header_data, NE_DOIP_HEADER_COMMON_LENGTH);
            pos += NE_DOIP_HEADER_COMMON_LENGTH;
            memcpy(send_data + pos, &entity_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
            memcpy(send_data + pos, &equip_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
            memcpy(send_data + pos, &code, NE_DOIP_NACK_CODE_LENGTH);

            ne_doip_pack(&link_data_t, NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_NEGATIVE_ACK,
                         send_data, NE_DOIP_DIAG_NEGATIVE_ACK_LENGTH);
            free(send_data);
        }
    }
}

void ne_doip_pack_diagnostic_from_entity(ne_doip_link_data_t *link_data, uint8_t pos,
                                         uint32_t payload_data_length)
{
    if (NULL == link_data || payload_data_length <= 0) {
        NE_DOIP_PRINT("[%s] data is invalid! ", __FUNCTION__);
        return;
    }

    uint8_t pos_t = pos;
    uint16_t entity_logical_address = 0x0000;
    memcpy(&entity_logical_address, link_data->data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    entity_logical_address = ne_doip_bswap_16(entity_logical_address);
    uint16_t equip_logical_address = 0x0000;
    pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    memcpy(&equip_logical_address, link_data->data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    equip_logical_address = ne_doip_bswap_16(equip_logical_address);
    pos = pos_t;

    ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
    ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_logic_address(equip_logical_address);
    if (equip_ipc_table != NULL) {
        // send to the internal test equipment
        // This case is a diagnostic response received from the doip node in the LAN and sent back to internal test equipment.
        int fd = equip_ipc_table->fd;
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

        ne_doip_connection_t *connection = malloc(sizeof *connection);
        memset(connection, 0, sizeof *connection);

        pos_t = 0;
        connection->out.data[pos_t] = NE_DOIP_IN_EQUIP_DIAGNOSTIC;
        pos_t += NE_DOIP_IN_COMMAND_LENGTH;
        memcpy(connection->out.data + pos_t, &payload_data_length, NE_DOIP_IN_DATA_LENGTH);
        pos_t += NE_DOIP_IN_DATA_LENGTH;
        memcpy(connection->out.data + pos_t, link_data->data + pos, payload_data_length);
        connection->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_data_length;
        connection->fd = fd;
        ne_doip_connection_write(connection);
        free(connection);
    }
    else {
        // send to the external test equipment
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
        ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
        ne_doip_node_ipc_table_t *node_ipc_table = ne_doip_node_ipc_list_find_by_fd(link_data->fd);
        if (NULL == node_ipc_table) {
            NE_DOIP_PRINT("<pack_diagnostic_from_entity step> get ipc table is failed! by fd[%d].\n", link_data->fd);
            ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
            return;
        }

        uint16_t ecu_logical_address = node_ipc_table->ecu_logical_address;
        int ipc_fd = node_ipc_table->fd;
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);

        uint32_t length_all = NE_DOIP_HEADER_COMMON_LENGTH + payload_data_length;
        char* send_data = (char*)malloc(length_all);
        memset(send_data, 0, length_all);

        char diag_res_data[NE_DOIP_HEADER_COMMON_LENGTH - NE_DOIP_PAYLOAD_LENGTH_LENGTH] =
            { global_server_manager->server->config->protocol_version,
             ~global_server_manager->server->config->protocol_version, 0x80, 0x01 };

        pos_t = 0;
        memcpy(send_data, diag_res_data, NE_DOIP_HEADER_COMMON_LENGTH - NE_DOIP_PAYLOAD_LENGTH_LENGTH);
        pos_t += NE_DOIP_HEADER_COMMON_LENGTH - NE_DOIP_PAYLOAD_LENGTH_LENGTH;
        uint32_t length_t = ne_doip_bswap_32(payload_data_length);
        memcpy(send_data + pos_t, &length_t, NE_DOIP_PAYLOAD_LENGTH_LENGTH);
        pos_t += NE_DOIP_PAYLOAD_LENGTH_LENGTH;
        memcpy(send_data + pos_t, link_data->data + pos, payload_data_length);

        ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
        ne_doip_node_tcp_table_t* tcp_table = ne_doip_node_tcp_list_find_by_logic_address(equip_logical_address);
        if (NULL == tcp_table) {
            free(send_data);
            ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
            return;
        }

        ne_doip_link_data_t link_data_t;
        memset(&link_data_t, 0, sizeof link_data_t);
        link_data_t.fd = tcp_table->fd;
        link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TCP;
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);

        ne_doip_pack(&link_data_t, NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_FROM_ENTITY, send_data, length_all);
        free(send_data);

        // When the diagnostic response is sent, return to the client to confirm the result.
        ne_doip_pack_diagnostic_confirmation(ipc_fd, equip_logical_address, ecu_logical_address);
    }
}

void ne_doip_pack_vin_gid_sync_boadcast()
{
    NE_DOIP_PRINT("ne_doip_pack_vin_gid_sync_boadcast is enter..\n");
    uint32_t data_size = NE_DOIP_HEADER_COMMON_LENGTH + NE_DOIP_VIN_SIZE;
    char* send_data = malloc(data_size);
    memset(send_data, 0, data_size);

    uint32_t payload_length = NE_DOIP_VIN_SIZE;
    uint16_t payload_type = NE_DOIP_OUT_VIN_GID_SYNC;

    uint8_t pos = 0;
    send_data[pos] = global_server_manager->server->config->protocol_version;
    pos += NE_DOIP_PROTOCOL_VERSION_LENGTH;
    send_data[pos] = ~global_server_manager->server->config->protocol_version;
    pos += NE_DOIP_INVERSE_PROTOCOL_VERSION_LENGTH;
    memcpy(send_data + pos, &payload_type, NE_DOIP_PAYLOAD_TYPE_LENGTH);
    pos += NE_DOIP_PAYLOAD_TYPE_LENGTH;
    payload_length = ne_doip_bswap_32(payload_length);
    memcpy(send_data + pos, &payload_length, NE_DOIP_PAYLOAD_LENGTH_LENGTH);
    pos += NE_DOIP_PAYLOAD_LENGTH_LENGTH;
    memcpy(send_data + pos, global_server_manager->server->config->vin, NE_DOIP_VIN_SIZE);

    ne_doip_connection_t *conn = malloc(sizeof *conn);
    memset(conn, 0, sizeof *conn);

    memcpy(conn->out.data, send_data, data_size);
    conn->out.data_size = data_size;

    if (NE_DOIP_NET_TYPE_IPV6 == global_server_manager->server->config->net_type) {
        conn->fd = global_server_manager->server->ipv6_udp_socket->fd;
        ne_doip_udp6_send(conn, NE_DOIP_UDP6_BROADCAST_IP, NE_DOIP_UDP_DISCOVERY_PORT);
    }
    else {
        conn->fd = global_server_manager->server->ipv4_udp_socket->fd;
        ne_doip_udp_send(conn, NE_DOIP_UDP_BROADCAST_IP, NE_DOIP_UDP_DISCOVERY_PORT);
    }

    free(send_data);
    free(conn);
}

void ne_doip_unpack_vin_gid_sync_broadcast(ne_doip_link_data_t *link_data, uint8_t pos)
{
    printf("recv edge vin is ");
    uint8_t i;
    for (i = 0; i < NE_DOIP_VIN_SIZE; ++i) {
        printf("%02X", (unsigned char)link_data->data[i + pos]);
    }
    printf("\n");
    memcpy(global_server_manager->server->config->vin, link_data->data + pos, NE_DOIP_VIN_SIZE);
    global_server_manager->server->vin_gid_sync_flag = 0x00; // synchronized
}

void ne_doip_unpack_vehicle_identification_req(ne_doip_link_data_t *link_data, uint8_t pos)
{
    ne_doip_sync_start(global_server_manager->node_udp_list_sync);
    ne_doip_node_udp_table_t* node_udp_table = ne_doip_node_udp_list_find_by_ip_port(link_data->ip, link_data->port);
    if (NULL == node_udp_table) {
        node_udp_table = malloc(sizeof *node_udp_table);
        memset(node_udp_table, 0, sizeof *node_udp_table);

        node_udp_table->fd = link_data->fd;
        node_udp_table->ip = (char*)malloc(strlen(link_data->ip) + 1);
        if (NULL != node_udp_table->ip) {
            memcpy(node_udp_table->ip, link_data->ip, strlen(link_data->ip) + 1);
        }
        node_udp_table->port = link_data->port;
        ne_doip_list_insert(global_server_manager->node_udp_list->prev, (ne_doip_list_t *) node_udp_table);
    }

    uint32_t random_time_value = ne_doip_get_random_value(NE_DOIP_A_DOIP_ANNOUNCE_WAIT_MAX);
    node_udp_table->vehicle_identify_wait_timeid = ne_doip_timer_start(global_server_manager->timer_manager,
                                          -1, random_time_value, ne_doip_vehicle_indentify_wait_timer_callback);
    ne_doip_sync_end(global_server_manager->node_udp_list_sync);
}

void ne_doip_unpack_vehicle_identification_req_eid(ne_doip_link_data_t *link_data,
                                                   uint8_t pos, uint32_t payload_length)
{
    if (NE_DOIP_EID_SIZE != payload_length) {
        NE_DOIP_PRINT("[%s] invalid payload length ! \n", __FUNCTION__);
        ne_doip_pack_header_negative_ack(link_data, NE_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH);
        return;
    }

    uint8_t result = strncmp(global_server_manager->server->config->eid, link_data->data + pos, payload_length);
    if (0 == result) {
        NE_DOIP_PRINT("EID is match ! \n");
        ne_doip_unpack_vehicle_identification_req(link_data, pos);
    }
    else {
        NE_DOIP_PRINT("EID is not match ! \n");
        return;
    }
}

void ne_doip_unpack_vehicle_identification_req_vin(ne_doip_link_data_t *link_data,
                                                   uint8_t pos, uint32_t payload_length)
{
    if (NE_DOIP_VIN_SIZE != payload_length) {
        NE_DOIP_PRINT("[%s] invalid payload length ! \n", __FUNCTION__);
        ne_doip_pack_header_negative_ack(link_data, NE_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH);
        return;
    }

    uint8_t result = strncmp(global_server_manager->server->config->vin, link_data->data + pos, payload_length);
    if (0 == result) {
        NE_DOIP_PRINT("VIN is match ! \n");
        ne_doip_unpack_vehicle_identification_req(link_data, pos);
    }
    else {
        NE_DOIP_PRINT("VIN is not match ! \n");
        return;
    }
}

void ne_doip_routing_activation_check_confirmation(ne_doip_link_data_t *link_data,
                                                   ne_doip_node_ipc_table_t *ipc_table)
{
    NE_DOIP_PRINT("ne_doip_routing_activation_check_confirmation is enter. \n");
    ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
    ne_doip_node_tcp_table_t *node_tcp_table = ne_doip_node_tcp_list_find_by_fd(link_data->fd);
    if (NULL == node_tcp_table) {
        NE_DOIP_PRINT("<routing_activation_check_confirmation step> get tcp table is failed! by fd[%d].\n", link_data->fd);
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
        return;
    }
    node_tcp_table->connection_state = NE_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_CONF;
    uint16_t equip_logical_address = node_tcp_table->equip_logical_address;
    ne_doip_sync_end(global_server_manager->node_tcp_list_sync);

    if (ipc_table != NULL) {
        if (NE_DOIP_TRUE == ipc_table->confirmation_flag) {
            NE_DOIP_PRINT("need confirmation.. \n");
            ne_doip_connection_t *conn = malloc(sizeof *conn);
            memset(conn, 0, sizeof *conn);

            uint8_t pos = 0;
            conn->out.data[pos] = NE_DOIP_IN_PAYLOADTYPE_USER_CONF_RESULT;
            pos += NE_DOIP_IN_COMMAND_LENGTH;
            uint32_t payload_length = NE_DOIP_LOGICAL_ADDRESS_LENGTH * 2;
            memcpy(conn->out.data + pos,  &payload_length, NE_DOIP_IN_DATA_LENGTH);
            pos += NE_DOIP_IN_DATA_LENGTH;
            equip_logical_address = ne_doip_bswap_16(equip_logical_address);
            memcpy(conn->out.data + pos, &equip_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
            uint16_t entity_logical_address = ne_doip_bswap_16(ipc_table->entity_logical_address);
            memcpy(conn->out.data + pos, &entity_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);

            conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
            conn->fd = ipc_table->fd;
            ne_doip_connection_write(conn);
            free(conn);
        }
        else {
            node_tcp_table->connection_state = NE_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE;
            node_tcp_table->fd_regist_flag = NE_DOIP_TRUE;
            ne_doip_pack_routing_activation_res(link_data, equip_logical_address,
                                                ipc_table->entity_logical_address,
                                                NE_DOIP_RA_RES_ROUTING_SUCCESSFULLY_ACTIVATED,
                                                0x00000000);
        }
    }
}

void ne_doip_routing_activation_check_authentication(ne_doip_link_data_t *link_data,
                                                     uint32_t authen_info)
{
    NE_DOIP_PRINT("ne_doip_routing_activation_check_authentication is enter. \n");
    ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
    ne_doip_node_tcp_table_t *node_tcp_table = ne_doip_node_tcp_list_find_by_fd(link_data->fd);
    if (NULL == node_tcp_table) {
        NE_DOIP_PRINT("<routing_activation_check_authentication step> get tcp table is failed! by fd[%d].\n", link_data->fd);
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
        return;
    }
    node_tcp_table->connection_state = NE_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_AUTH;
    uint16_t equip_logical_address = node_tcp_table->equip_logical_address;
    ne_doip_sync_end(global_server_manager->node_tcp_list_sync);

    ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
    ne_doip_node_ipc_table_t *ipc_entity_table = ne_doip_node_ipc_list_find_entity();
    if (ipc_entity_table != NULL) {
        if (authen_info == global_server_manager->server->config->authen_info
            || 0 == global_server_manager->server->config->authen_info) {
            ne_doip_routing_activation_check_confirmation(link_data, ipc_entity_table);
        }
        else {
            NE_DOIP_PRINT("missing authentication.. \n");
            ne_doip_pack_routing_activation_res(link_data, equip_logical_address,
                                                ipc_entity_table->entity_logical_address,
                                                NE_DOIP_RA_RES_MISSING_AUTHENTICATION,
                                                0x00000000);
        }
    }
    ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
}

void ne_doip_unpack_routing_activation_req(ne_doip_link_data_t *link_data, uint8_t pos,
                                           uint32_t payload_length)
{
    if (NE_DOIP_ROUTING_ACTIVATION_MAND_LENGTH != payload_length
        && NE_DOIP_ROUTING_ACTIVATION_ALL_LENGTH != payload_length) {
        NE_DOIP_PRINT("[%s] invalid payload length ! \n", __FUNCTION__);
        ne_doip_pack_header_negative_ack(link_data, NE_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH);
        return;
    }

    uint16_t equip_logical_address = 0x0000;
    memcpy(&equip_logical_address, link_data->data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    equip_logical_address = ne_doip_bswap_16(equip_logical_address);
    pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;

    if (NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server_manager->server->config->entity_type
        || NE_DOIP_FALSE == global_server_manager->server->config->egw_control) {

        if (global_server_manager->server->config->tester_sa != 0) {
            if (equip_logical_address != global_server_manager->server->config->tester_sa) {
                // DoIP-059: Unknown source address
                
                ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
                ne_doip_node_ipc_table_t* ipc_entity_table = ne_doip_node_ipc_list_find_entity();
                if (ipc_entity_table != NULL) {
                    ne_doip_pack_routing_activation_res(link_data, equip_logical_address,
                                                        ipc_entity_table->entity_logical_address,
                                                        NE_DOIP_RA_RES_UNKNOWN_SOURCE_ADDRESS,
                                                        0x00000000);
                }
                ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
                ne_doip_server_disconnect(global_server_manager->server, NE_DOIP_SOCKET_TYPE_TCP, link_data->fd);
                return;
            }
        }
        else {
            if (equip_logical_address < 0x0F00 || equip_logical_address > 0x0F7F) {
                // DoIP-059: Unknown source address
                if (equip_logical_address < 0x0E00 || equip_logical_address > 0x0EFF) {
                    NE_DOIP_PRINT("Unknown source address.To do routing active res.\n");

                    ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
                    ne_doip_node_ipc_table_t* ipc_entity_table = ne_doip_node_ipc_list_find_entity();
                    if (ipc_entity_table != NULL) {
                        ne_doip_pack_routing_activation_res(link_data, equip_logical_address,
                                                            ipc_entity_table->entity_logical_address,
                                                            NE_DOIP_RA_RES_UNKNOWN_SOURCE_ADDRESS,
                                                            0x00000000);
                    }

                    ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
                    ne_doip_server_disconnect(global_server_manager->server, NE_DOIP_SOCKET_TYPE_TCP, link_data->fd);
                    return;
                }
            }
        }
    }

    uint8_t activation_type = 0x00;
    memcpy(&activation_type, link_data->data + pos, NE_DOIP_ACTIVATION_TYPE_LENGTH);
    pos += NE_DOIP_ACTIVATION_TYPE_LENGTH;

    // DoIP-151: Routing activation type not supported
    if (activation_type >= 0x02 && activation_type <= 0xDF) {
        NE_DOIP_PRINT("Routing activation type not supported.To do routing active res.\n");

        ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
        ne_doip_node_ipc_table_t* ipc_entity_table = ne_doip_node_ipc_list_find_entity();
        if (ipc_entity_table != NULL) {
            ne_doip_pack_routing_activation_res(link_data, equip_logical_address,
                                                ipc_entity_table->entity_logical_address,
                                                NE_DOIP_RA_RES_UNSUPPORTED_RA_TYPE,
                                                0x00000000);
        }
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
        ne_doip_server_disconnect(global_server_manager->server, NE_DOIP_SOCKET_TYPE_TCP, link_data->fd);
        return;
    }

    uint32_t ra_reserved1 = 0;
    memcpy(&ra_reserved1, link_data->data + pos, NE_DOIP_S_RESERVED_LENGTH);
    ra_reserved1 = ne_doip_bswap_32(ra_reserved1);
    pos += NE_DOIP_S_RESERVED_LENGTH;
    NE_DOIP_PRINT("[%s] equip_logical_address is %04X \n", __FUNCTION__, equip_logical_address);
    NE_DOIP_PRINT("[%s] activation_type is %02X \n", __FUNCTION__, activation_type);
    NE_DOIP_PRINT("[%s] ra_reserved1 is %08X \n", __FUNCTION__, ra_reserved1);

    uint32_t authen_info = 0;
    if (NE_DOIP_ROUTING_ACTIVATION_ALL_LENGTH == payload_length) {
        memcpy(&authen_info, link_data->data + pos, NE_DOIP_S_RESERVED_LENGTH);
        authen_info = ne_doip_bswap_32(authen_info);
        pos += NE_DOIP_S_RESERVED_LENGTH;
        NE_DOIP_PRINT("[%s] authen_info is %08X \n", __FUNCTION__, authen_info);
    }


    ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
    ne_doip_node_tcp_table_t *node_tcp_table = ne_doip_node_tcp_list_find_by_fd(link_data->fd);
    if (NULL == node_tcp_table) {
        NE_DOIP_PRINT("<unpack_routing_activation_req step> get tcp_table is failed! by fd[%d].\n", link_data->fd);
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
        return;
    }

    // Immediately stop the initial inactivity timer of the TCP_DATA socket after receiving a valid route activation request
    if (node_tcp_table->tcp_initial_inactivity_timeid != 0) {
        ne_doip_timer_stop(global_server_manager->timer_manager, node_tcp_table->tcp_initial_inactivity_timeid);
    }

    node_tcp_table->authen_info = authen_info;

    // Check if tcp socket has been registered
    ne_doip_node_tcp_table_t *tcp_table_t = NULL;
    uint8_t result = NE_DOIP_FALSE;
    ne_doip_list_for_each(tcp_table_t, global_server_manager->node_tcp_list, base) {
        if (NE_DOIP_TRUE == tcp_table_t->fd_regist_flag) {
            result = NE_DOIP_TRUE;
            break;
        }
    }

    // The following logic refers to ISO13400-2-2011 Figure 13.
    if (NE_DOIP_FALSE == result) {
        NE_DOIP_PRINT("number of registered tcp socket is 0.. \n");
        node_tcp_table->fd_regist_flag = NE_DOIP_TRUE;
        node_tcp_table->equip_logical_address = equip_logical_address;
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);

        ne_doip_routing_activation_check_authentication(link_data, authen_info);
    }
    else {
        if (NE_DOIP_TRUE == node_tcp_table->fd_regist_flag) {
            NE_DOIP_PRINT("tcp socket is already registered.. \n");
            if (equip_logical_address == node_tcp_table->equip_logical_address) {
                NE_DOIP_PRINT("received sa is already assigned to this tcp socket.. \n");
                ne_doip_sync_end(global_server_manager->node_tcp_list_sync);

                ne_doip_routing_activation_check_authentication(link_data, authen_info);
            }
            else {
                ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
                // DoIP-149: An SA different from the table connection entry was received on the already activated TCP_DATA socket.
                NE_DOIP_PRINT("Received sa is not assigned to this tcp socket.To do routing active res. \n");

                ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
                ne_doip_node_ipc_table_t* ipc_entity_table = ne_doip_node_ipc_list_find_entity();
                if (ipc_entity_table != NULL) {
                    ne_doip_pack_routing_activation_res(link_data, equip_logical_address,
                                                        ipc_entity_table->entity_logical_address,
                                                        NE_DOIP_RA_RES_SA_DIFFERENT_ACTIVATED_SOCKET,
                                                        0x00000000);
                }
                ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
                ne_doip_server_disconnect(global_server_manager->server, NE_DOIP_SOCKET_TYPE_TCP, link_data->fd);
            }
        }
        else {
            ne_doip_node_tcp_table_t *tcp_table_other = ne_doip_node_tcp_list_find_by_logic_address(equip_logical_address);
            if (tcp_table_other != NULL) {
                ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
                // NE_DOIP_PRINT("sa is already assigned to another tcp socket.. \n");
                // alive check（single SA）
                node_tcp_table->connection_state = NE_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE;
                node_tcp_table->fd_regist_flag = NE_DOIP_TRUE;
                node_tcp_table->equip_logical_address = equip_logical_address;
                ne_doip_link_data_t link_data_t;
                memset(&link_data_t, 0, sizeof link_data_t);
                link_data_t.fd = node_tcp_table->fd;
                link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TCP;
                ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
                ne_doip_node_ipc_table_t* ipc_entity_table = ne_doip_node_ipc_list_find_entity();
                if (ipc_entity_table != NULL) {
                    ne_doip_pack_routing_activation_res(&link_data_t, equip_logical_address,
                                                        ipc_entity_table->entity_logical_address,
                                                        NE_DOIP_RA_RES_ROUTING_SUCCESSFULLY_ACTIVATED,
                                                        0x00000000);
                }
                ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
            }
            else {
                uint8_t regist_fd_count = ne_doip_node_tcp_list_regist_fd_count();
                if (regist_fd_count == global_server_manager->server->config->mcts) {
                    ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
                    NE_DOIP_PRINT("tcp sockets are not available.. \n");
                    // alive check（all registered sockets）
                    node_tcp_table->equip_logical_address = equip_logical_address;
                    ne_doip_node_tcp_table_t *tcp_tables = NULL;
                    ne_doip_list_for_each(tcp_tables, global_server_manager->node_tcp_list, base) {
                        if (NE_DOIP_TRUE == tcp_tables->fd_regist_flag) {
                            ne_doip_link_data_t link_data_t;
                            memset(&link_data_t, 0, sizeof link_data_t);
                            link_data_t.fd = tcp_tables->fd;
                            link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TCP;
                            ne_doip_pack_alive_check_req(&link_data_t);
                            tcp_tables->all_alive_check_flag = NE_DOIP_TRUE;
                            tcp_tables->logical_routing_address = equip_logical_address;
                            tcp_tables->tcp_alive_check_timeid = ne_doip_timer_start(global_server_manager->timer_manager,
                                                                   -1, global_server_manager->server->config->alive_check_time,
                                                                   ne_doip_alive_check_timer_callback);
                        }
                    }
                }
                else {
                    NE_DOIP_PRINT("tcp sockets are available.. \n");
                    node_tcp_table->fd_regist_flag = NE_DOIP_TRUE;
                    node_tcp_table->equip_logical_address = equip_logical_address;
                    ne_doip_sync_end(global_server_manager->node_tcp_list_sync);

                    ne_doip_routing_activation_check_authentication(link_data, authen_info);
                }
            }
        }
    }
}

void ne_doip_unpack_alive_check_res(ne_doip_link_data_t *link_data, uint8_t pos, uint32_t payload_length)
{
    if (NE_DOIP_LOGICAL_ADDRESS_LENGTH != payload_length) {
        NE_DOIP_PRINT("[%s] invalid payload length ! \n", __FUNCTION__);
        ne_doip_pack_header_negative_ack(link_data, NE_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH);
        return;
    }
    uint16_t equip_logical_address = 0x0000;
    memcpy(&equip_logical_address, link_data->data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    equip_logical_address = ne_doip_bswap_16(equip_logical_address);

    ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
    ne_doip_node_tcp_table_t *node_tcp_table = ne_doip_node_tcp_list_find_by_logic_address(equip_logical_address);
    if (NULL != node_tcp_table) {
        NE_DOIP_PRINT("[%s] node_tcp_table fd is %d \n", __FUNCTION__, node_tcp_table->fd);
        ne_doip_timer_stop(global_server_manager->timer_manager, node_tcp_table->tcp_alive_check_timeid);
    }
    else {
        NE_DOIP_PRINT("[%s] invalid alive check response ! \n", __FUNCTION__);
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
        return;
    }

    if (node_tcp_table->all_alive_check_flag) {
        // alive check（all re/gistered sockets） response
        node_tcp_table->all_alive_check_flag = NE_DOIP_FALSE;
        if (++global_server_manager->alive_check_ncts == global_server_manager->server->config->mcts) {
            NE_DOIP_PRINT("All concurrently supported TCP_DATA sockets are registered and active.To do routing active res.\n");
            uint16_t logical_routing_address = node_tcp_table->logical_routing_address;
            ne_doip_node_tcp_table_t *routing_tcp_table = ne_doip_node_tcp_list_find_by_logic_address(logical_routing_address);
            if (routing_tcp_table != NULL) {
                if (NE_DOIP_FALSE == routing_tcp_table->fd_regist_flag) {
                    ne_doip_link_data_t link_data_t;
                    memset(&link_data_t, 0, sizeof link_data_t);
                    link_data_t.fd = routing_tcp_table->fd;
                    link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TCP;
                    ne_doip_sync_end(global_server_manager->node_tcp_list_sync);

                    ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
                    ne_doip_node_ipc_table_t* ipc_entity_table = ne_doip_node_ipc_list_find_entity();
                    ne_doip_pack_routing_activation_res(&link_data_t, logical_routing_address,
                                                        ipc_entity_table->entity_logical_address,
                                                        NE_DOIP_RA_RES_ALL_SOCKET_REGISTED_AND_ACTIVE,
                                                        0x00000000);
                    ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
                    // Close the fd that was previously activated by the route and remove the element from the tcp list
                    ne_doip_server_disconnect(global_server_manager->server, NE_DOIP_SOCKET_TYPE_TCP, routing_tcp_table->fd);
                }
                else {
                    ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
                }
            }
            else {
                ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
            }
            global_server_manager->alive_check_ncts = 0; // Set back to the initial value
        }
        else {
            ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
        }
    }
    else if (node_tcp_table->single_alive_check_flag) {
        // alive check（single SA） response
        node_tcp_table->single_alive_check_flag = NE_DOIP_FALSE;
        int routing_fd = node_tcp_table->routing_fd;
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
        // DoIP-150: SA is already registered and active on a different TCP_DATA socket.
        NE_DOIP_PRINT("SA is already registered and active on a different TCP_DATA socket.To do routing active res.\n");

        ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
        ne_doip_node_ipc_table_t* ipc_entity_table = ne_doip_node_ipc_list_find_entity();
        if (ipc_entity_table != NULL) {
            ne_doip_link_data_t link_data_t;
            memset(&link_data_t, 0, sizeof link_data_t);
            link_data_t.fd = routing_fd;
            link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TCP;
            ne_doip_pack_routing_activation_res(&link_data_t, equip_logical_address,
                                                ipc_entity_table->entity_logical_address,
                                                NE_DOIP_RA_RES_SA_REGISTED_DIFFERENT_SOCKET,
                                                0x00000000);
        }
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
        ne_doip_server_disconnect(global_server_manager->server, NE_DOIP_SOCKET_TYPE_TCP, routing_fd);
    }
    else {
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
        NE_DOIP_PRINT("One-way acceptance of the alivecheck response from the external test equipment, do nothing.\n");
    }

}

void ne_doip_unpack_entity_status_req(ne_doip_link_data_t *link_data, uint8_t pos)
{
    link_data->comm_type = NE_DOIP_SOCKET_TYPE_UDP_UNI;
    ne_doip_sync_start(global_server_manager->server->net_list_sync);
    uint8_t open_socket_num = ne_doip_list_length(global_server_manager->server->net_client_list);
    ne_doip_sync_end(global_server_manager->server->net_list_sync);
    ne_doip_pack_entity_status_res(link_data, open_socket_num);
}

void ne_doip_unpack_power_mode_req(ne_doip_link_data_t *link_data, uint8_t pos)
{
    NE_DOIP_PRINT("current power mode is %02X \n", global_server_manager->server->power_mode);
    link_data->comm_type = NE_DOIP_SOCKET_TYPE_UDP_UNI;
    ne_doip_pack_power_mode_res(link_data, global_server_manager->server->power_mode);
}

void ne_doip_unpack_diagnostic(ne_doip_link_data_t *link_data,
                               uint8_t pos, uint32_t payload_length)
{
    ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
    ne_doip_equip_tcp_table_t *equip_tcp_table = ne_doip_equip_tcp_list_find_by_fd(link_data->fd);
    ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);

    if (NULL == equip_tcp_table) {
        // diagnostic from external test equipment request
        int fd = 0;
        ne_doip_node_tcp_table_t *node_tcp_table = ne_doip_node_tcp_list_find_by_fd(link_data->fd);
        if (NULL == node_tcp_table) {
            NE_DOIP_PRINT("<unpack_diagnostic step> get tcp table is failed! by fd[%d].\n", link_data->fd);
            return;
        }
        else if (node_tcp_table->diag_data_current_pos > 0) {
            node_tcp_table->diag_data_current_pos += payload_length;
            NE_DOIP_PRINT("recv diag_data_total_length is [%u byte], diag_data_current_pos is [%u byte]..\n",
                node_tcp_table->diag_data_total_length, node_tcp_table->diag_data_current_pos);

            if (node_tcp_table->diag_data_current_pos == node_tcp_table->diag_data_total_length) {
                node_tcp_table->diag_data_fail_flag = NE_DOIP_FALSE;
                node_tcp_table->diag_data_current_pos = 0;
                node_tcp_table->diag_data_total_length = 0;
            }
            if (NE_DOIP_TRUE == node_tcp_table->diag_data_fail_flag) {
                return;
            }

            if (NE_DOIP_TRUE == global_server_manager->server->config->egw_control
                && NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server_manager->server->config->entity_type) {

                if (NE_DOIP_TRUE == node_tcp_table->functional_addressing_flag) {
                    ne_doip_egw_func_routing(link_data, pos, payload_length, (ne_doip_list_t*)node_tcp_table);
                }
                else {
                    ne_doip_egw_phys_routing(link_data, pos, payload_length, (ne_doip_list_t*)node_tcp_table);
                }
            }
            else {
                if (NE_DOIP_TRUE == node_tcp_table->functional_addressing_flag) {
                    ne_doip_node_ipc_table_t *ipc_table_t = NULL;
                    ne_doip_list_for_each(ipc_table_t, global_server_manager->node_ipc_list, base) {
                        if (ipc_table_t->ecu_logical_address != 0) {
                            ne_doip_connection_t *conn = malloc(sizeof *conn);
                            memset(conn, 0, sizeof *conn);

                            memcpy(conn->out.data, link_data->data + pos, payload_length);
                            conn->out.data_size = payload_length;
                            conn->fd = ipc_table_t->fd;
                            ne_doip_connection_write(conn);
                            free(conn);
                        }
                    }
                }
                else {
                    fd = node_tcp_table->cache_fd;
                    ne_doip_connection_t *conn = malloc(sizeof *conn);
                    memset(conn, 0, sizeof *conn);

                    memcpy(conn->out.data, link_data->data + pos, payload_length);
                    conn->out.data_size = payload_length;
                    conn->fd = fd;
                    ne_doip_connection_write(conn);
                    free(conn);
                }
            }
        }
        else {
            node_tcp_table->diag_data_current_pos += payload_length;
            uint32_t total_length_tmp = node_tcp_table->diag_data_total_length;
            NE_DOIP_PRINT("diag_data_total_length is [%u byte], diag_data_current_pos is [%u byte]..\n",
                node_tcp_table->diag_data_total_length, node_tcp_table->diag_data_current_pos);

            if (node_tcp_table->diag_data_current_pos == node_tcp_table->diag_data_total_length) {
                node_tcp_table->diag_data_total_length = 0;
                node_tcp_table->diag_data_current_pos = 0;
            }

            if (global_server_manager->server->power_mode != NE_DOIP_POWER_MODE_READY) {
                NE_DOIP_PRINT("[%s] power mode is not ready ! \n", __FUNCTION__);
                node_tcp_table->diag_data_fail_flag = NE_DOIP_TRUE;
                return;
            }

            if (payload_length <= NE_DOIP_LOGICAL_ADDRESS_LENGTH * 2) { // diagnostic payload length is at least 5 byte
                NE_DOIP_PRINT("[%s] invalid payload length ! \n", __FUNCTION__);
                ne_doip_pack_header_negative_ack(link_data, NE_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH);
                node_tcp_table->diag_data_fail_flag = NE_DOIP_TRUE;
                return;
            }

            uint8_t pos_t = pos;
            uint16_t logical_source_address = 0x0000;
            memcpy(&logical_source_address, link_data->data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            logical_source_address = ne_doip_bswap_16(logical_source_address);
            pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;

            uint16_t logical_target_address = 0x0000;
            memcpy(&logical_target_address, link_data->data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            logical_target_address = ne_doip_bswap_16(logical_target_address);
            pos = pos_t;

            node_tcp_table->functional_addressing_flag = NE_DOIP_FALSE;
            if (NE_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE == node_tcp_table->connection_state) {
                // Determine if it is functional addressing
                if (NE_DOIP_TRUE == ne_doip_is_functianal_address(global_server_manager->server->config, logical_target_address)) {
                    node_tcp_table->functional_addressing_flag = NE_DOIP_TRUE;
                }

                if (node_tcp_table->equip_logical_address != logical_source_address) {
                    NE_DOIP_PRINT("[%s] The SA is not registered on the TCP socket.\n", __FUNCTION__);
                    ne_doip_pack_diagnostic_negative_ack(link_data, logical_target_address,
                                                         logical_source_address,
                                                         NE_DOIP_DIAGNOSTIC_NACK_INVALID_SA);
                    ne_doip_server_disconnect(global_server_manager->server, NE_DOIP_SOCKET_TYPE_TCP, link_data->fd);
                    node_tcp_table->diag_data_fail_flag = NE_DOIP_TRUE;
                    return;
                }
            }
            else {
                NE_DOIP_PRINT("[%s] No route activation before using this fd diagnostic.\n", __FUNCTION__);
                ne_doip_pack_diagnostic_negative_ack(link_data, logical_target_address,
                                                     logical_source_address,
                                                     NE_DOIP_DIAGNOSTIC_NACK_TARGET_UNREACHABLE);
                node_tcp_table->diag_data_fail_flag = NE_DOIP_TRUE;
                return;
            }

            node_tcp_table->entity_logical_address = logical_target_address;

            if (NE_DOIP_TRUE == node_tcp_table->functional_addressing_flag) {
                ne_doip_sub_diagnostic_functional(link_data, pos, payload_length,
                                                  total_length_tmp, node_tcp_table->entity_logical_address,
                                                  node_tcp_table->equip_logical_address);
            }
            else {

                if (NE_DOIP_TRUE == global_server_manager->server->config->egw_control
                    && NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server_manager->server->config->entity_type) {

                    ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
                    ne_doip_node_ipc_table_t *ipc_table = ne_doip_node_ipc_list_find_by_logic_address(logical_target_address);
                    if (NULL == ipc_table) {
                        ne_doip_routing_table_t *routing_table = ne_doip_routing_list_find_by_logic_address(logical_target_address);
                        if (routing_table != NULL) {
                            NE_DOIP_PRINT("Edge gatewey routing start..to [%04X]..\n", logical_target_address);
                            ne_doip_sub_diagnostic_to_other(link_data, pos, payload_length, total_length_tmp,
                                                            routing_table, NE_DOIP_FALSE);
                        }
                        else {
                            NE_DOIP_PRINT("[%s] The TA[%04X] is unknown.\n", __FUNCTION__, logical_target_address);
                            ne_doip_pack_diagnostic_negative_ack(link_data, logical_target_address,
                                                                 node_tcp_table->equip_logical_address,
                                                                 NE_DOIP_DIAGNOSTIC_NACK_UNKNOWN_TA);
                            node_tcp_table->diag_data_fail_flag = NE_DOIP_TRUE;
                        }
                    }
                    else {
                        // Diagnose edge gatewey itself
                        node_tcp_table->cache_fd = ipc_table->fd;
                        ne_doip_sub_diagnostic_to_self(link_data, pos, payload_length, total_length_tmp, node_tcp_table->equip_logical_address, ipc_table, NE_DOIP_TA_TYPE_PHYSICAL);
                    }
                    ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
                }
                else {
                    ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
                    ne_doip_node_ipc_table_t *ipc_table = ne_doip_node_ipc_list_find_by_logic_address(logical_target_address);
                    if (NULL == ipc_table) {
                        NE_DOIP_PRINT("[%s] The TA[%04X] is unknown.\n", __FUNCTION__, logical_target_address);
                        ne_doip_pack_diagnostic_negative_ack(link_data, logical_target_address,
                                                             node_tcp_table->equip_logical_address,
                                                             NE_DOIP_DIAGNOSTIC_NACK_UNKNOWN_TA);
                        node_tcp_table->diag_data_fail_flag = NE_DOIP_TRUE;
                    }
                    else {
                        // Diagnose edge gatewey itself
                        node_tcp_table->cache_fd = ipc_table->fd;
                        ne_doip_sub_diagnostic_to_self(link_data, pos, payload_length, total_length_tmp, node_tcp_table->equip_logical_address, ipc_table, NE_DOIP_TA_TYPE_PHYSICAL);
                    }
                    ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
                }
            }
        }
    }
    else {
        // diagnostic from other doip node response
        if (payload_length <= NE_DOIP_LOGICAL_ADDRESS_LENGTH * 2) { // diagnostic payload length is at least 5 byte
            NE_DOIP_PRINT("[%s] invalid payload length ! \n", __FUNCTION__);
            return;
        }

        if (NE_DOIP_TRUE == global_server_manager->server->config->egw_control
            && NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server_manager->server->config->entity_type) {
            // Case in which the nodes in the domain are not visible
            if (equip_tcp_table->source_fd > 0) {
                int fd = equip_tcp_table->source_fd;

                uint8_t is_internal_equip_flag = NE_DOIP_FALSE;
                uint16_t equip_logical_address;
                ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
                ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(fd);
                if (equip_ipc_table != NULL) {
                    is_internal_equip_flag =  NE_DOIP_TRUE;
                    equip_logical_address = equip_ipc_table->equip_logical_address;
                }
                ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

                if (NE_DOIP_TRUE == is_internal_equip_flag) {
                    ne_doip_connection_t *conn = malloc(sizeof *conn);
                    memset(conn, 0, sizeof *conn);

                    uint8_t pos_t = 0;
                    conn->out.data[pos_t] = NE_DOIP_IN_EQUIP_DIAGNOSTIC;
                    pos_t += NE_DOIP_IN_COMMAND_LENGTH;
                    memcpy(conn->out.data + pos_t,  &payload_length, NE_DOIP_IN_DATA_LENGTH);
                    pos_t += NE_DOIP_IN_DATA_LENGTH;
                    memcpy(conn->out.data + pos_t, link_data->data + pos, payload_length);
                    pos_t += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
                    equip_logical_address = ne_doip_bswap_16(equip_logical_address);
                    memcpy(conn->out.data + pos_t, &equip_logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
                    conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
                    conn->fd = fd;
                    ne_doip_connection_write(conn);
                    free(conn);
                }
                else {
                    ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
                    ne_doip_node_tcp_table_t *node_tcp_table = ne_doip_node_tcp_list_find_by_fd(fd);
                    if (NULL == node_tcp_table) {
                        NE_DOIP_PRINT("<unpack_diagnostic step> get tcp table is failed! by fd[%d].\n", fd);
                        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
                        return;
                    }
                    uint16_t logical_address = ne_doip_bswap_16(node_tcp_table->equip_logical_address);
                    ne_doip_sync_end(global_server_manager->node_tcp_list_sync);

                    char* send_data = malloc(payload_length + NE_DOIP_HEADER_COMMON_LENGTH);

                    memcpy(send_data, link_data->data + pos - NE_DOIP_HEADER_COMMON_LENGTH, payload_length + NE_DOIP_HEADER_COMMON_LENGTH);
                    memcpy(send_data + NE_DOIP_HEADER_COMMON_LENGTH + NE_DOIP_LOGICAL_ADDRESS_LENGTH, &logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
                    
                    ne_doip_connection_write_raw(fd, send_data, payload_length + NE_DOIP_HEADER_COMMON_LENGTH);
                    free(send_data);
                }
            }
            else {
                NE_DOIP_PRINT("from other node, get routing fd is 0...\n");
            }
        }
        else {
            // Case in which the nodes in the domain are visible
            uint8_t pos_t = pos;
            uint16_t logical_source_address = 0x0000;
            memcpy(&logical_source_address, link_data->data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            logical_source_address = ne_doip_bswap_16(logical_source_address);
            pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;

            if (NE_DOIP_FALSE == global_server_manager->server->config->egw_control) {
                // For scenario 1, the information of the subnet ECU is stored in the test tcp table.
                ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
                ne_doip_equip_tcp_table_t* test_tcp_table_t = ne_doip_equip_tcp_list_find_by_logic_address(logical_source_address);
                if (NULL == test_tcp_table_t) {
                   
                    ne_doip_addr_data_t* addr_data = malloc(sizeof *addr_data);
                    memset(addr_data, 0, sizeof *addr_data);
                    if (addr_data != NULL) {
                        addr_data->fd = equip_tcp_table->fd;
                        addr_data->comm_type = NE_DOIP_SOCKET_TYPE_TEST;
                        addr_data->ip = (char*)malloc(strlen(equip_tcp_table->ip) + 1);
                        if (addr_data->ip != NULL) {
                            memcpy(addr_data->ip, equip_tcp_table->ip, strlen(equip_tcp_table->ip) + 1);
                        }
                        addr_data->port = equip_tcp_table->port;
                        ne_doip_add_connection_table(addr_data);
                        if (addr_data->ip != NULL) {
                            free(addr_data->ip);
                        }
                        free(addr_data);
                    }

                    ne_doip_list_for_each(test_tcp_table_t, global_server_manager->equip_tcp_list, base) {
                        if (test_tcp_table_t->fd == equip_tcp_table->fd && 0 == test_tcp_table_t->entity_logical_address) {
                            test_tcp_table_t->entity_logical_address = logical_source_address;
                        }
                    }
                }
                ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);
            }

            uint16_t logical_target_address = 0x0000;
            memcpy(&logical_target_address, link_data->data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            logical_target_address = ne_doip_bswap_16(logical_target_address);
            pos = pos_t;

            ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
            ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_logic_address(logical_target_address);
            if (NULL == equip_ipc_table) {
                NE_DOIP_PRINT("<unpack_routing_activation_res step> get equip ipc table is failed!\n");
                ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
                return;
            }

            int fd = equip_ipc_table->fd;
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

            ne_doip_connection_t *conn = malloc(sizeof *conn);
            memset(conn, 0, sizeof *conn);

            pos_t = 0;
            conn->out.data[pos_t] = NE_DOIP_IN_EQUIP_DIAGNOSTIC;
            pos_t += NE_DOIP_IN_COMMAND_LENGTH;
            memcpy(conn->out.data + pos_t,  &payload_length, NE_DOIP_IN_DATA_LENGTH);
            pos_t += NE_DOIP_IN_DATA_LENGTH;
            memcpy(conn->out.data + pos_t, link_data->data + pos, payload_length);
            conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
            conn->fd = fd;
            ne_doip_connection_write(conn);
            free(conn);
        }
    }
}

void ne_doip_unpack_header_negative_ack(ne_doip_link_data_t *link_data, uint8_t pos, uint32_t payload_length)
{
    NE_DOIP_PRINT("ne_doip_unpack_header_negative_ack is ignored...\n");
    return;
}

void ne_doip_unpack_annouce_or_identify_res(ne_doip_link_data_t *link_data, uint8_t pos, uint32_t payload_length)
{
    uint8_t num = ne_doip_list_length(global_server_manager->equip_ipc_list);
    if (0 == num) {
        return;
    }

    if (payload_length != NE_DOIP_ANNOUNCE_OR_IDENTITYRES_MAND_LENGTH
        && payload_length != NE_DOIP_ANNOUNCE_OR_IDENTITYRES_ALL_LENGTH) {
        NE_DOIP_PRINT("ne_doip_unpack_annouce_or_identify_res invalid payload length");
        return;
    }
    // Manage the list of announcement/vehicle identity responses
    // record ip/port/eid for subsequent lookup
    ne_doip_equip_udp_table_t* equip_udp_table = malloc(sizeof *equip_udp_table);
    memset(equip_udp_table, 0, sizeof *equip_udp_table);

    equip_udp_table->fd = link_data->fd;
    if (NULL != link_data->ip) {
        equip_udp_table->ip = (char*)malloc(strlen(link_data->ip) + 1);
        if (NULL != equip_udp_table->ip) {
            memcpy(equip_udp_table->ip, link_data->ip, strlen(link_data->ip) + 1);
        }
    }
    equip_udp_table->port = link_data->port;
    uint16_t logical_addr = 0x0000;
    memcpy(&logical_addr, link_data->data + pos + NE_DOIP_VIN_SIZE, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    equip_udp_table->entity_logical_address = ne_doip_bswap_16(logical_addr);
    memcpy(equip_udp_table->eid, link_data->data + pos + NE_DOIP_VIN_SIZE + NE_DOIP_LOGICAL_ADDRESS_LENGTH, NE_DOIP_EID_SIZE);

    ne_doip_sync_start(global_server_manager->equip_udp_list_sync);
    ne_doip_list_insert(global_server_manager->equip_udp_list->prev, (ne_doip_list_t *) equip_udp_table);
    ne_doip_sync_end(global_server_manager->equip_udp_list_sync);

    NE_DOIP_PRINT("annouce_or_identify_res Transfer to internal test equipment...\n");
    ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
    ne_doip_equip_ipc_table_t *equip_ipc_table = NULL;
    ne_doip_list_for_each(equip_ipc_table, global_server_manager->equip_ipc_list, base) {
        if (equip_ipc_table->fd > 0) {
            ne_doip_connection_t *conn = malloc(sizeof *conn);
            if (conn != NULL) {
                memset(conn, 0, sizeof *conn);
                uint8_t pos_t = 0;
                conn->out.data[pos_t] = NE_DOIP_IN_EQUIP_ANN_IDEN_RES;
                pos_t += NE_DOIP_IN_COMMAND_LENGTH;
                memcpy(conn->out.data + pos_t, &payload_length, NE_DOIP_IN_DATA_LENGTH);
                pos_t += NE_DOIP_IN_DATA_LENGTH;
                memcpy(conn->out.data + pos_t, link_data->data + pos, payload_length);
                conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
                conn->fd = equip_ipc_table->fd;
                ne_doip_connection_write(conn);
                free(conn);
            }
        }
    }
    ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
}

void ne_doip_unpack_routing_activation_res(ne_doip_link_data_t *link_data, uint8_t pos, uint32_t payload_length)
{
    ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
    ne_doip_equip_tcp_table_t* equip_tcp_table = ne_doip_equip_tcp_list_find_by_fd(link_data->fd);
    if (NULL == equip_tcp_table) {
        NE_DOIP_PRINT("<unpack_routing_activation_res step> get test tcp table is failed! by fd[%d].\n", link_data->fd);
        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);
        return;
    }

    uint16_t logical_addr = 0x0000;
    memcpy(&logical_addr, link_data->data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);

    ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
    ne_doip_node_ipc_table_t* ipc_table = ne_doip_node_ipc_list_find_by_logic_address(ne_doip_bswap_16(logical_addr));
    if (ipc_table != NULL) {
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);

        uint8_t ra_res_code = link_data->data[pos + NE_DOIP_LOGICAL_ADDRESS_LENGTH * 2];
        NE_DOIP_PRINT("routing activation res enter...res code is [%d]..\n", ra_res_code);

        if (NE_DOIP_RA_RES_ROUTING_SUCCESSFULLY_ACTIVATED == ra_res_code
            || NE_DOIP_RA_RES_CONFIRMATION_REQUIRED == ra_res_code) {

            equip_tcp_table->diag_routing_step = NE_DOIP_DIAG_ROUTING_STEP_1;
            while (ne_doip_queue_empty(equip_tcp_table->queue_manager) != NE_DOIP_TRUE) {
                ne_doip_routing_data_t* routing_data = ne_doip_front_queue(equip_tcp_table->queue_manager);
                if (routing_data != 0) {
                    ne_doip_link_data_t link_data_t;
                    memset(&link_data_t, 0, sizeof link_data_t);
                    link_data_t.fd = routing_data->fd;
                    link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TEST;
                    ne_doip_pack(&link_data_t, NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_FROM_EQUIP, routing_data->data, routing_data->data_size);
                    free(routing_data->data);
                    free(routing_data);
                }
            }
            equip_tcp_table->diag_routing_step = NE_DOIP_DIAG_ROUTING_STEP_2;
        }
        else {
            ne_doip_queue_clear(equip_tcp_table->queue_manager, link_data->fd);
        }
        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);   
    }
    else {
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);

        uint16_t equip_logical_address = ne_doip_bswap_16(logical_addr);
        memcpy(&logical_addr, link_data->data + pos + NE_DOIP_LOGICAL_ADDRESS_LENGTH, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        equip_tcp_table->entity_logical_address = equip_logical_address;

        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);

        // Transfer to internal test equipment
        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_logic_address(equip_logical_address);
        if (NULL == equip_ipc_table) {
            NE_DOIP_PRINT("<unpack_routing_activation_res step> get equip ipc table is failed!\n");
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
            return;
        }

        int fd = equip_ipc_table->fd;
        equip_ipc_table->tcp_fd = link_data->fd;
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

        ne_doip_connection_t *conn = malloc(sizeof *conn);
        memset(conn, 0, sizeof *conn);

        uint8_t pos_t = 0;
        conn->out.data[pos_t] = NE_DOIP_IN_EQUIP_ROUTING_ACTIVE;
        pos_t += NE_DOIP_IN_COMMAND_LENGTH;
        memcpy(conn->out.data + pos_t,  &payload_length, NE_DOIP_IN_DATA_LENGTH);
        pos_t += NE_DOIP_IN_DATA_LENGTH;
        memcpy(conn->out.data + pos_t, link_data->data + pos, payload_length);
        conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
        conn->fd = fd;
        ne_doip_connection_write(conn);
        free(conn);
    }
}

void ne_doip_unpack_entity_status_res(ne_doip_link_data_t *link_data, uint8_t pos, uint32_t payload_length)
{
    if (NULL == link_data->ip || 0 == link_data->port) {
        return;
    }

    ne_doip_sync_start(global_server_manager->equip_udp_list_sync);
    ne_doip_equip_udp_table_t *equip_udp_table = ne_doip_equip_udp_list_find_by_ip_port(link_data->ip, link_data->port);

    if (NULL == equip_udp_table) {
        ne_doip_sync_end(global_server_manager->equip_udp_list_sync);
        return;
    }
    else {
        int fd = equip_udp_table->source_fd;
        ne_doip_sync_end(global_server_manager->equip_udp_list_sync);

        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(fd);
        if (NULL == equip_ipc_table) {
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
            return;
        }
        else {
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
            ne_doip_connection_t *conn = malloc(sizeof *conn);
            memset(conn, 0, sizeof *conn);

            uint8_t pos_t = 0;
            conn->out.data[pos_t] = NE_DOIP_IN_EQUIP_ENTITY_STATUS;
            pos_t += NE_DOIP_IN_COMMAND_LENGTH;
            memcpy(conn->out.data + pos_t,  &payload_length, NE_DOIP_IN_DATA_LENGTH);
            pos_t += NE_DOIP_IN_DATA_LENGTH;
            memcpy(conn->out.data + pos_t, link_data->data + pos, payload_length);
            conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
            conn->fd = fd;
            ne_doip_connection_write(conn);
            free(conn);
        }
    }
}

void ne_doip_unpack_power_mode_res(ne_doip_link_data_t *link_data, uint8_t pos, uint32_t payload_length)
{
    if (NULL == link_data->ip || 0 == link_data->port) {
        return;
    }

    ne_doip_sync_start(global_server_manager->equip_udp_list_sync);
    ne_doip_equip_udp_table_t *equip_udp_table = ne_doip_equip_udp_list_find_by_ip_port(link_data->ip, link_data->port);

    if (NULL == equip_udp_table) {
        ne_doip_sync_end(global_server_manager->equip_udp_list_sync);
        return;
    }
    else {
        int fd = equip_udp_table->source_fd;
        ne_doip_sync_end(global_server_manager->equip_udp_list_sync);

        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(fd);
        if (NULL == equip_ipc_table) {
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
            return;
        }
        else {
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
            ne_doip_connection_t *conn = malloc(sizeof *conn);
            memset(conn, 0, sizeof *conn);

            uint8_t pos_t = 0;
            conn->out.data[pos_t] = NE_DOIP_IN_EQUIP_POWER_MODE;
            pos_t += NE_DOIP_IN_COMMAND_LENGTH;
            memcpy(conn->out.data + pos_t,  &payload_length, NE_DOIP_IN_DATA_LENGTH);
            pos_t += NE_DOIP_IN_DATA_LENGTH;
            memcpy(conn->out.data + pos_t, link_data->data + pos, payload_length);
            conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
            conn->fd = fd;
            ne_doip_connection_write(conn);
            free(conn);
        }
    }
}

void ne_doip_unpack_daig_positive_ack(ne_doip_link_data_t *link_data, uint8_t pos, uint32_t payload_length)
{
    ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
    ne_doip_equip_tcp_table_t* equip_tcp_table = ne_doip_equip_tcp_list_find_by_fd(link_data->fd);
    if (NULL == equip_tcp_table) {
        NE_DOIP_PRINT("<unpack_routing_activation_res step> get test tcp table is failed! by fd[%d].\n", link_data->fd);
        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);
        return;
    }

    uint16_t logical_addr = 0x0000;
    memcpy(&logical_addr, link_data->data + pos + NE_DOIP_LOGICAL_ADDRESS_LENGTH, NE_DOIP_LOGICAL_ADDRESS_LENGTH);

    ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
    ne_doip_node_ipc_table_t* ipc_table = ne_doip_node_ipc_list_find_by_logic_address(ne_doip_bswap_16(logical_addr));
    ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
    if (ipc_table != NULL) {
        int fd = equip_tcp_table->source_fd;
        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);

        uint8_t is_internal_equip_flag = NE_DOIP_FALSE;
        uint16_t entity_logical_address;
        uint16_t equip_logical_address;
        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(fd);
        if (equip_ipc_table != NULL) {
            is_internal_equip_flag =  NE_DOIP_TRUE;
            entity_logical_address = equip_tcp_table->entity_logical_address;
            equip_logical_address = equip_ipc_table->equip_logical_address;
        }
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

        if (NE_DOIP_TRUE == is_internal_equip_flag) {
            ne_doip_link_data_t link_data_t;
            memset(&link_data_t, 0, sizeof link_data_t);
            link_data_t.fd = fd;
            link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_IPC;
            ne_doip_pack_diagnostic_positive_ack(&link_data_t, entity_logical_address, equip_logical_address);
        }
        else {
            ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
            ne_doip_node_tcp_table_t *tcp_table = ne_doip_node_tcp_list_find_by_fd(fd);
            if (NULL == tcp_table) {
                NE_DOIP_PRINT("<unpack_daig_positive_ack step> get tcp table is failed! by fd[%d].\n", fd);
                ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
                return;
            }
            uint16_t logical_address = ne_doip_bswap_16(tcp_table->equip_logical_address);
            ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
            char* send_data = malloc(payload_length + NE_DOIP_HEADER_COMMON_LENGTH);
            memset(send_data, 0, payload_length + NE_DOIP_HEADER_COMMON_LENGTH);

            memcpy(send_data, link_data->data + pos - NE_DOIP_HEADER_COMMON_LENGTH, payload_length + NE_DOIP_HEADER_COMMON_LENGTH);
            memcpy(send_data + NE_DOIP_HEADER_COMMON_LENGTH + NE_DOIP_LOGICAL_ADDRESS_LENGTH, &logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            
            ne_doip_connection_write_raw(fd, send_data, payload_length + NE_DOIP_HEADER_COMMON_LENGTH);
            free(send_data);
        }
    }
    else {
        // Transfer to internal test equipment
        int fd = equip_tcp_table->source_fd;
        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);

        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(fd);
        if (NULL == equip_ipc_table) {
            NE_DOIP_PRINT("internal test equipment has been released... do not send alive check response...\n");
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
        }
        else {
            int fd = equip_ipc_table->fd;
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

            ne_doip_connection_t *conn = malloc(sizeof *conn);
            memset(conn, 0, sizeof *conn);

            uint8_t pos_t = 0;
            conn->out.data[pos_t] = NE_DOIP_IN_EQUIP_DIAGNOSTIC_PACK;
            pos_t += NE_DOIP_IN_COMMAND_LENGTH;
            memcpy(conn->out.data + pos_t,  &payload_length, NE_DOIP_IN_DATA_LENGTH);
            pos_t += NE_DOIP_IN_DATA_LENGTH;
            memcpy(conn->out.data + pos_t, link_data->data + pos, payload_length);
            conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
            conn->fd = fd;
            ne_doip_connection_write(conn);
            free(conn);
        }

    }
}

void ne_doip_unpack_daig_negative_ack(ne_doip_link_data_t *link_data, uint8_t pos, uint32_t payload_length)
{
    ne_doip_sync_start(global_server_manager->equip_tcp_list_sync);
    ne_doip_equip_tcp_table_t* equip_tcp_table = ne_doip_equip_tcp_list_find_by_fd(link_data->fd);
    if (NULL == equip_tcp_table) {
        NE_DOIP_PRINT("<unpack_routing_activation_res step> get test tcp table is failed! by fd[%d].\n", link_data->fd);
        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);
        return;
    }

    uint16_t logical_addr = 0x0000;
    memcpy(&logical_addr, link_data->data + pos + NE_DOIP_LOGICAL_ADDRESS_LENGTH, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    uint8_t nack_code = 0x00;
    memcpy(&nack_code, link_data->data + pos + NE_DOIP_LOGICAL_ADDRESS_LENGTH * 2, NE_DOIP_NACK_CODE_LENGTH);

    ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
    ne_doip_node_ipc_table_t* ipc_table = ne_doip_node_ipc_list_find_by_logic_address(ne_doip_bswap_16(logical_addr));
    ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
    if (ipc_table != NULL) {

        int fd = equip_tcp_table->source_fd;
        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);

        uint8_t is_internal_equip_flag = NE_DOIP_FALSE;
        uint16_t entity_logical_address;
        uint16_t equip_logical_address;
        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(fd);
        if (equip_ipc_table != NULL) {
            is_internal_equip_flag =  NE_DOIP_TRUE;
            entity_logical_address = equip_tcp_table->entity_logical_address;
            equip_logical_address = equip_ipc_table->equip_logical_address;
        }
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

        if (NE_DOIP_TRUE == is_internal_equip_flag) {
            ne_doip_link_data_t link_data_t;
            memset(&link_data_t, 0, sizeof link_data_t);
            link_data_t.fd = fd;
            link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_IPC;
            ne_doip_pack_diagnostic_negative_ack(&link_data_t, entity_logical_address,
                                                 equip_logical_address, nack_code);
        }
        else {
            ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
            ne_doip_node_tcp_table_t *tcp_table = ne_doip_node_tcp_list_find_by_fd(fd);
            if (NULL == tcp_table) {
                NE_DOIP_PRINT("<unpack_daig_negative_ack step> get tcp table is failed! by fd[%d].\n", fd);
                ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
                return;
            }
            uint16_t logical_address = ne_doip_bswap_16(tcp_table->equip_logical_address);
            ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
            char* send_data = malloc(payload_length + NE_DOIP_HEADER_COMMON_LENGTH);
            memset(send_data, 0, payload_length + NE_DOIP_HEADER_COMMON_LENGTH);

            memcpy(send_data, link_data->data + pos - NE_DOIP_HEADER_COMMON_LENGTH, payload_length + NE_DOIP_HEADER_COMMON_LENGTH);
            memcpy(send_data + NE_DOIP_HEADER_COMMON_LENGTH + NE_DOIP_LOGICAL_ADDRESS_LENGTH, &logical_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            
            ne_doip_connection_write_raw(fd, send_data, payload_length + NE_DOIP_HEADER_COMMON_LENGTH);
            free(send_data);
        }
    }
    else {
        // Transfer to internal test equipment
        int fd = equip_tcp_table->source_fd;
        ne_doip_sync_end(global_server_manager->equip_tcp_list_sync);

        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(fd);
        if (NULL == equip_ipc_table) {
            NE_DOIP_PRINT("internal test equipment has been released... do not send alive check response...\n");
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
        }
        else {
            int fd = equip_ipc_table->fd;
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

            ne_doip_connection_t *conn = malloc(sizeof *conn);
            memset(conn, 0, sizeof *conn);

            int8_t pos_t = 0;
            conn->out.data[pos_t] = NE_DOIP_IN_EQUIP_DIAGNOSTIC_NACK;
            pos_t += NE_DOIP_IN_COMMAND_LENGTH;
            memcpy(conn->out.data + pos_t,  &payload_length, NE_DOIP_IN_DATA_LENGTH);
            pos_t += NE_DOIP_IN_DATA_LENGTH;
            memcpy(conn->out.data + pos_t, link_data->data + pos, payload_length);
            conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
            conn->fd = fd;
            ne_doip_connection_write(conn);
            free(conn);
        }
    }
}

void ne_doip_unpack_activation_line_broadcast()
{
    ne_doip_server_disconnect_all(global_server_manager->server, NE_DOIP_SOCKET_TYPE_TCP);
}

void ne_doip_pack(ne_doip_link_data_t *link_data, uint8_t type, char* data, uint32_t length)
{
    NE_DOIP_PRINT("[%s] start....\n", __FUNCTION__);
#ifdef NE_DOIP_DEBUG
    printf("[length: %u]pack data is ", length);
    uint32_t i;
    for (i = 0; i < length; ++i) {  
        printf("%02X", (unsigned char)data[i]);
    }
    printf("\n");
#endif

    if (link_data->fd < 0) {
        NE_DOIP_PRINT("[%s] fd is ivalid! doip pack is stoped...\n", __FUNCTION__);
        return;
    }

    if (NE_DOIP_SOCKET_TYPE_TCP == link_data->comm_type) {
        ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
        ne_doip_node_tcp_table_t* node_tcp_table = ne_doip_node_tcp_list_find_by_fd(link_data->fd);
        if (NULL == node_tcp_table) {
            NE_DOIP_PRINT("<doip pack step> get tcp_table is failed! by fd[%d].\n", link_data->fd);
            ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
            return;
        }

        if (node_tcp_table->tcp_generral_inactivity_timeid != 0) {
            // Send tcp data to reset the general inactivity timer
            ne_doip_timer_restart(global_server_manager->timer_manager, -1,
                global_server_manager->server->config->general_inactivity_time,
                node_tcp_table->tcp_generral_inactivity_timeid);
        }
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
    }

    ne_doip_connection_t *conn = malloc(sizeof *conn);
    memset(conn, 0, sizeof *conn);

    memcpy(conn->out.data, data, length);
    conn->out.data_size = length;
    conn->fd = link_data->fd;

    switch (type) {
    case NE_DOIP_PAYLOAD_TYPE_HEADER_NEGATIVE_ACK:
    {
        if (NE_DOIP_SOCKET_TYPE_TCP == link_data->comm_type) {
            ne_doip_connection_write(conn);
        }
        else if (NE_DOIP_SOCKET_TYPE_UDP_MOUTI == link_data->comm_type
            || NE_DOIP_SOCKET_TYPE_UDP_UNI == link_data->comm_type
            || NE_DOIP_SOCKET_TYPE_UNKNOWN == link_data->comm_type) {
            if (NE_DOIP_NET_TYPE_IPV6 == global_server_manager->server->config->net_type) {
                ne_doip_udp6_send(conn, link_data->ip, link_data->port);
            }
            else {
                ne_doip_udp_send(conn, link_data->ip, link_data->port);
            }
        }
        break;
    }
    case NE_DOIP_PAYLOAD_TYPE_VEHICLE_ANNOUNCEMENT:
    {
        if (NE_DOIP_NET_TYPE_IPV6 == global_server_manager->server->config->net_type) {
            conn->fd = global_server_manager->server->ipv6_udp_socket->fd;
            ne_doip_udp6_send(conn, NE_DOIP_UDP6_BROADCAST_IP, NE_DOIP_UDP_DISCOVERY_PORT);
        }
        else {
            conn->fd = global_server_manager->server->ipv4_udp_socket->fd;
            ne_doip_udp_send(conn, NE_DOIP_UDP_BROADCAST_IP, NE_DOIP_UDP_DISCOVERY_PORT);
        }

        ne_doip_net_source_t *net_source = NULL;
        ne_doip_list_for_each(net_source, global_server_manager->server->config->net_source_list, base) {
            if (NE_DOIP_IP_CONFIGURED_ANNOUNCED == net_source->announce_state) {
                net_source->announce_count += 1;
                if (1 == net_source->announce_count) {
                    net_source->announce_interval_timeid = ne_doip_timer_start(global_server_manager->timer_manager,
                                                                              1, global_server_manager->server->config->announce_interval_time,
                                                                              ne_doip_vehicle_announce_interval_timer_callback);
                }
                else if (3 == net_source->announce_count) {
                    net_source->announce_count = 0;
                }
            }
        }
        break;
    }
    case NE_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_RESPONSE:
    case NE_DOIP_PAYLOAD_TYPE_ENTITY_STATUS_RESPONSE:
    case NE_DOIP_PAYLOAD_TYPE_POWER_MODE_INFO_RESPONSE:
    case NE_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST_EID:
    case NE_DOIP_PAYLOAD_TYPE_ENTITY_STATUS_REQUEST:
    case NE_DOIP_PAYLOAD_TYPE_POWER_MODE_INFO_REQUEST:
    {
        if (NE_DOIP_NET_TYPE_IPV6 == global_server_manager->server->config->net_type) {
            ne_doip_udp6_send(conn, link_data->ip, link_data->port);
        }
        else {
            ne_doip_udp_send(conn, link_data->ip, link_data->port);
        }
        break;
    }
    case NE_DOIP_PAYLOAD_TYPE_ROUTING_ACTIVE_RESPONSE:
    case NE_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_REQUEST:
    case NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_POSITIVE_ACK:
    case NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_NEGATIVE_ACK:
    case NE_DOIP_PAYLOAD_TYPE_ROUTING_ACTIVE_REQUEST:
    case NE_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_RESPONSE:
    case NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_FROM_EQUIP:
    case NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_FROM_ENTITY:
    {
        ne_doip_connection_write(conn);
        break;
    }
    case NE_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST:
    case NE_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST_VIN:
    {
        if (NE_DOIP_NET_TYPE_IPV6 == global_server_manager->server->config->net_type) {
            conn->fd = global_server_manager->server->ipv6_udp_socket->fd;
            ne_doip_udp6_send(conn, NE_DOIP_UDP6_BROADCAST_IP, NE_DOIP_UDP_DISCOVERY_PORT);
        }
        else {
            conn->fd = global_server_manager->server->ipv4_udp_socket->fd;
            ne_doip_udp_send(conn, NE_DOIP_UDP_BROADCAST_IP, NE_DOIP_UDP_DISCOVERY_PORT);
        }
        break;
    }
    default :
        break;
    }
    free(conn);
}

uint32_t ne_doip_net_unpack_exec(ne_doip_link_data_t *link_data, uint32_t pos)
{
    if (NE_DOIP_SOCKET_TYPE_TCP == link_data->comm_type) {
        // Check if the diagnostic data has been received completely
        ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
        ne_doip_node_tcp_table_t *node_tcp_table = ne_doip_node_tcp_list_find_by_fd(link_data->fd);
        if (NULL == node_tcp_table) {
            NE_DOIP_PRINT("<net unpack step> get tcp_table is failed! by fd[%d].\n", link_data->fd);
            ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
            return 0;
        }
        else if (node_tcp_table->diag_data_total_length - node_tcp_table->diag_data_current_pos > 0) {
            // Case that exceeds buffer size
            uint32_t rest_length = node_tcp_table->diag_data_total_length - node_tcp_table->diag_data_current_pos;
            ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
            if (rest_length >= link_data->data_size) {
                ne_doip_integrate_thread_task(NE_DOIP_DIAG_SOURCE_TYPE_EXTERNAL_EQUIP, link_data, pos, link_data->data_size);
                // ne_doip_unpack_diagnostic(link_data, pos, link_data->data_size);
                return link_data->data_size;
            }
            else {
                ne_doip_integrate_thread_task(NE_DOIP_DIAG_SOURCE_TYPE_EXTERNAL_EQUIP, link_data, pos, rest_length);
                // ne_doip_unpack_diagnostic(link_data, pos, rest_length);
                return rest_length;
            }
        }
        else {
            // The case of the first diagnosis request, and the case of the new request after the diagnosis is completed.
            node_tcp_table->diag_data_total_length = 0;
            node_tcp_table->diag_data_current_pos = 0;
            ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
        }
    }

    uint8_t protocol_version = link_data->data[pos];
    pos += NE_DOIP_PROTOCOL_VERSION_LENGTH;
    uint8_t inverse_protocol_version = link_data->data[pos];
    pos += NE_DOIP_INVERSE_PROTOCOL_VERSION_LENGTH;

    inverse_protocol_version = ~inverse_protocol_version;

    uint16_t payload_type = 0x0000;
    uint32_t payload_length = 0;

    uint8_t config_protocol_version = global_server_manager->server->config->protocol_version;
    if (protocol_version != config_protocol_version && protocol_version != NE_DOIP_DEFAULT_VERSION) {
        NE_DOIP_PRINT("[%s] protocol_version[%02X] is not match and is not default !\n", __FUNCTION__, protocol_version);
        ne_doip_pack_header_negative_ack(link_data, NE_DOIP_HEADER_NACK_INCORRECT_PATTERN_FORMAT);
        return 0;
    }
    else if (inverse_protocol_version != protocol_version) {
        NE_DOIP_PRINT("[%s] incorrect pattern format! protocol_version is [%02X], inverse_protocol_version is [%02X]\n",
               __FUNCTION__, protocol_version, inverse_protocol_version);
        ne_doip_pack_header_negative_ack(link_data, NE_DOIP_HEADER_NACK_INCORRECT_PATTERN_FORMAT);
        return 0;
    }
    else {
        memcpy(&payload_type, link_data->data + pos, NE_DOIP_PAYLOAD_TYPE_LENGTH);
        payload_type = ne_doip_bswap_16(payload_type);
        pos += NE_DOIP_PAYLOAD_TYPE_LENGTH;

        memcpy(&payload_length, link_data->data + pos, NE_DOIP_PAYLOAD_LENGTH_LENGTH);
        payload_length = ne_doip_bswap_32(payload_length);
        pos += NE_DOIP_PAYLOAD_LENGTH_LENGTH;

        if (payload_type != NE_DOIP_ST_PAYLOADTYPE_VEHICLE_INDENTIFY
            && payload_type != NE_DOIP_ST_PAYLOADTYPE_VEHICLE_INDENTIFY_EID
            && payload_type != NE_DOIP_ST_PAYLOADTYPE_VEHICLE_INDENTIFY_VIN
            && payload_type != NE_DOIP_ST_PAYLOADTYPE_ROUTING_ACTIVE_REQ
            && payload_type != NE_DOIP_ST_PAYLOADTYPE_ALIVE_CHECK_RES
            && payload_type != NE_DOIP_ST_PAYLOADTYPE_ENTINTY_STATUS_REQ
            && payload_type != NE_DOIP_ST_PAYLOADTYPE_POWER_MODE_REQ
            && payload_type != NE_DOIP_ST_PAYLOADTYPE_DIAGNOSTIC_REQ
            && payload_type != NE_DOIP_ST_PAYLOADTYPE_HEADER_NEGTIVE_ACK
            && payload_type != NE_DOIP_ST_PAYLOADTYPE_ANNOUNCE_OR_IDENTIFYRES
            && payload_type != NE_DOIP_ST_PAYLOADTYPE_ALIVE_CHECK_REQ
            && payload_type != NE_DOIP_ST_PAYLOADTYPE_ROUTING_ACTIVE_RES
            && payload_type != NE_DOIP_ST_PAYLOADTYPE_ENTINTY_STATUS_RES
            && payload_type != NE_DOIP_ST_PAYLOADTYPE_POWER_MODE_RES
            && payload_type != NE_DOIP_ST_PAYLOADTYPE_DIAG_POSITIVE_ACK
            && payload_type != NE_DOIP_ST_PAYLOADTYPE_DIAG_NEGATIVE_ACK
            && payload_type != NE_DOIP_OUT_VIN_GID_SYNC
            && payload_type != NE_DOIP_OUT_ACTIVATION_DEACTIVE) {
            ne_doip_pack_header_negative_ack(link_data, NE_DOIP_HEADER_NACK_UNKNOWN_PAYLOAD_TYPE);
            if (0 == payload_length) {
                return 0;
            }
            else if (payload_length + NE_DOIP_HEADER_COMMON_LENGTH >= link_data->data_size - pos) {
                return link_data->data_size;
            }
            else {
                return pos + payload_length;
            }
        }

        NE_DOIP_PRINT("[%s] payload_length is %d byte \n", __FUNCTION__, payload_length);
        if (payload_length > global_server_manager->server->config->mds) {
            NE_DOIP_PRINT("[%s] message too large !\n", __FUNCTION__);
            ne_doip_pack_header_negative_ack(link_data, NE_DOIP_HEADER_NACK_MESSAGE_TOO_LARGE);
            if (payload_length + NE_DOIP_HEADER_COMMON_LENGTH >= link_data->data_size - pos) {
                return link_data->data_size;
            }
            else {
                return pos + payload_length;
            }
        }
    }

    switch (payload_type) {
    case NE_DOIP_ST_PAYLOADTYPE_VEHICLE_INDENTIFY:
    {
        if (NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server_manager->server->config->entity_type
            || NE_DOIP_FALSE == global_server_manager->server->config->egw_control) {
            NE_DOIP_PRINT("[%s] payload_type is [Vehicle Identification request message] \n", __FUNCTION__);
            uint8_t num = ne_doip_list_length(global_server_manager->equip_ipc_list);
            if (0 == num) {
                ne_doip_unpack_vehicle_identification_req(link_data, pos);
            }
            else {
                NE_DOIP_PRINT("[%s] This is an internal broadcast message, ignored  \n", __FUNCTION__);
            }
        }
        break;
    }
    case NE_DOIP_ST_PAYLOADTYPE_VEHICLE_INDENTIFY_EID:
    {
        NE_DOIP_PRINT("[%s] payload_type is [Vehicle identification request message with EID] \n", __FUNCTION__);
        ne_doip_unpack_vehicle_identification_req_eid(link_data, pos, payload_length);
        break;
    }
    case NE_DOIP_ST_PAYLOADTYPE_VEHICLE_INDENTIFY_VIN:
    {
        if (NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server_manager->server->config->entity_type
            || NE_DOIP_FALSE == global_server_manager->server->config->egw_control) {
            NE_DOIP_PRINT("[%s] payload_type is [Vehicle identification request message with VIN] \n", __FUNCTION__);
            uint8_t num = ne_doip_list_length(global_server_manager->equip_ipc_list);
            if (0 == num) {
                ne_doip_unpack_vehicle_identification_req_vin(link_data, pos, payload_length);
            }
            else {
                NE_DOIP_PRINT("[%s] This is an internal broadcast message, ignored  \n", __FUNCTION__);
            }
        }
        break;
    }
    case NE_DOIP_ST_PAYLOADTYPE_ROUTING_ACTIVE_REQ:
    {
        NE_DOIP_PRINT("[%s] payload_type is [Routing activation request] \n", __FUNCTION__);
        ne_doip_unpack_routing_activation_req(link_data, pos, payload_length);
        break;
    }
    case NE_DOIP_ST_PAYLOADTYPE_ALIVE_CHECK_RES:
    {
        NE_DOIP_PRINT("[%s] payload_type is [Alive Check response] \n", __FUNCTION__);
        ne_doip_unpack_alive_check_res(link_data, pos, payload_length);
        break;
    }
    case NE_DOIP_ST_PAYLOADTYPE_ENTINTY_STATUS_REQ:
    {
        NE_DOIP_PRINT("[%s] payload_type is [DoIP entity status request] \n", __FUNCTION__);
        ne_doip_unpack_entity_status_req(link_data, pos);
        break;
    }
    case NE_DOIP_ST_PAYLOADTYPE_POWER_MODE_REQ:
    {
        NE_DOIP_PRINT("[%s] payload_type is [Diagnostic power mode information request] \n", __FUNCTION__);
        ne_doip_unpack_power_mode_req(link_data, pos);
        break;
    }
    case NE_DOIP_ST_PAYLOADTYPE_DIAGNOSTIC_REQ:
    {
        NE_DOIP_PRINT("[%s] payload_type is [Diagnostic message] \n", __FUNCTION__);
        ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
        ne_doip_node_tcp_table_t *node_tcp_table = ne_doip_node_tcp_list_find_by_fd(link_data->fd);
        if (node_tcp_table != NULL) {
            node_tcp_table->diag_data_total_length = payload_length;
        }
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);

        if (payload_length > link_data->data_size - pos) {
            if (++g_task_id > 255) {
                g_task_id = 0;
            }
            ne_doip_unpack_diagnostic(link_data, pos, link_data->data_size - pos);
        }
        else {
            ne_doip_unpack_diagnostic(link_data, pos, payload_length);
        }
        break;
    }
    case NE_DOIP_ST_PAYLOADTYPE_HEADER_NEGTIVE_ACK:
    {
        NE_DOIP_PRINT("[%s] payload_type is [Generic DoIPheader negative acknowledge] \n", __FUNCTION__);
        ne_doip_unpack_header_negative_ack(link_data, pos, payload_length);
        break;
    }
    case NE_DOIP_ST_PAYLOADTYPE_ANNOUNCE_OR_IDENTIFYRES:
    {
        char eid[NE_DOIP_EID_SIZE] = { 0 };
        memcpy(eid, link_data->data + pos + NE_DOIP_VIN_SIZE + NE_DOIP_LOGICAL_ADDRESS_LENGTH, NE_DOIP_EID_SIZE);
        uint8_t result = strncmp(global_server_manager->server->config->eid, eid, NE_DOIP_EID_SIZE);

        if (0 == result) {
            NE_DOIP_PRINT("[%s] This is an internal broadcast message, ignored  \n", __FUNCTION__);
        }
        else {
            if (NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server_manager->server->config->entity_type
                && NE_DOIP_ANNOUNCE_OR_IDENTITYRES_ALL_LENGTH == payload_length
                && NE_DOIP_TRUE == global_server_manager->server->config->need_vin_gid_sync) {
                // Edge gateway receives GID/VIN unsynchronized message
                uint8_t vin_gid_sync = 0;
                memcpy(&vin_gid_sync, link_data->data + pos + NE_DOIP_ANNOUNCE_OR_IDENTITYRES_MAND_LENGTH, 1);
                if (NE_DOIP_VIN_GID_NOT_SYNCHRONIZED == vin_gid_sync) {
                    ne_doip_pack_vin_gid_sync_boadcast();
                }
            }

            ne_doip_unpack_annouce_or_identify_res(link_data, pos, payload_length);
        }
        break;
    }
    case NE_DOIP_ST_PAYLOADTYPE_ALIVE_CHECK_REQ:
    {
        NE_DOIP_PRINT("[%s] payload_type is [Alive check request] \n", __FUNCTION__);
        ne_doip_pack_alive_check_res(link_data, pos);
        break;
    }
    case NE_DOIP_ST_PAYLOADTYPE_ROUTING_ACTIVE_RES:
    {
        NE_DOIP_PRINT("[%s] payload_type is [Routing Actiocation response] \n", __FUNCTION__);
        ne_doip_unpack_routing_activation_res(link_data, pos, payload_length);
        break;
    }
    case NE_DOIP_ST_PAYLOADTYPE_ENTINTY_STATUS_RES:
    {
        NE_DOIP_PRINT("[%s] payload_type is [Entity status response] \n", __FUNCTION__);
        ne_doip_unpack_entity_status_res(link_data, pos, payload_length);
        break;
    }
    case NE_DOIP_ST_PAYLOADTYPE_POWER_MODE_RES:
    {
        NE_DOIP_PRINT("[%s] payload_type is [Power mode response] \n", __FUNCTION__);
        ne_doip_unpack_power_mode_res(link_data, pos, payload_length);
        break;
    }
    case NE_DOIP_ST_PAYLOADTYPE_DIAG_POSITIVE_ACK:
    {
        NE_DOIP_PRINT("[%s] payload_type is [Diagnostic positive acknowledge] \n", __FUNCTION__);
        ne_doip_unpack_daig_positive_ack(link_data, pos, payload_length);
        break;
    }
    case NE_DOIP_ST_PAYLOADTYPE_DIAG_NEGATIVE_ACK:
    {
        NE_DOIP_PRINT("[%s] payload_type is [Diagnostic negative acknowledge] \n", __FUNCTION__);
        ne_doip_unpack_daig_negative_ack(link_data, pos, payload_length);
        break;
    }
    case NE_DOIP_OUT_VIN_GID_SYNC:
    {
        NE_DOIP_PRINT("[%s] payload_type is [vin/gid sync broadcast] \n", __FUNCTION__);
        ne_doip_unpack_vin_gid_sync_broadcast(link_data, pos);
        break;
    }
    case NE_DOIP_OUT_ACTIVATION_DEACTIVE:
    {
        if (global_server_manager->server->config->entity_type != NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY) {
            NE_DOIP_PRINT("[%s] payload_type is [activation line deactive] \n", __FUNCTION__);
            ne_doip_unpack_activation_line_broadcast();
        }
        else {
            NE_DOIP_PRINT("[%s] This is an internal broadcast message, ignored \n", __FUNCTION__);
        }
        break;
    }
    default:
        NE_DOIP_PRINT("[%s] unknown payload type !\n", __FUNCTION__);
        break;
    }

    if (payload_length + pos >= link_data->data_size) {
        return link_data->data_size;
    }
    else {
        pos += payload_length;
        return pos;
    }
}

void ne_doip_net_unpack(ne_doip_link_data_t *link_data)
{
    NE_DOIP_PRINT("[%s] start....\n", __FUNCTION__);
    if (NULL == link_data) {
        NE_DOIP_PRINT("[%s]link_data is null! net unpack is stoped...\n", __FUNCTION__);
        return;
    }
#ifdef NE_DOIP_DEBUG
    printf("recv data is ");
    uint32_t i;
    for (i = 0; i < link_data->data_size; ++i) {
        printf("%02X", (unsigned char)link_data->data[i]);
    }
    printf("\n");
#endif

    if (NE_DOIP_SOCKET_TYPE_TCP == link_data->comm_type) {
        ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
        ne_doip_node_tcp_table_t* node_tcp_table = ne_doip_node_tcp_list_find_by_fd(link_data->fd);
        if (NULL == node_tcp_table) {
            ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
            NE_DOIP_PRINT("<net unpack step> get tcp_table is failed! by fd[%d].\n", link_data->fd);
            return;
        }
        node_tcp_table->comm_type = NE_DOIP_SOCKET_TYPE_TCP;
        if (node_tcp_table->tcp_generral_inactivity_timeid != 0) {
            // Receive tcp data, reset general inactivity timer
            ne_doip_timer_restart(global_server_manager->timer_manager, -1,
                global_server_manager->server->config->general_inactivity_time,
                node_tcp_table->tcp_generral_inactivity_timeid);
        }
        else {
            // Receive tcp data for the first time, start the general inactivity timer
            node_tcp_table->tcp_generral_inactivity_timeid = ne_doip_timer_start(global_server_manager->timer_manager, -1,
                                                            global_server_manager->server->config->general_inactivity_time,
                                                            ne_doip_general_timer_callback);
        }
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
    }

    if (link_data->data_size < NE_DOIP_HEADER_COMMON_LENGTH) {
        NE_DOIP_PRINT("[%s] DoIP message is invalid! [Data size is less than 8 bytes].\n", __FUNCTION__);
        return;
    }

    uint32_t pos = 0;
    do {
        pos = ne_doip_net_unpack_exec(link_data, pos);
        if (0 == pos) {
            break;
        }
    } while (link_data->data_size - pos > 0);
}

uint32_t ne_doip_ipc_unpack_execute(ne_doip_link_data_t *link_data, uint32_t pos)
{
    ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
    ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(link_data->fd);
    if (equip_ipc_table != NULL) {
        if (equip_ipc_table->diag_data_total_length - equip_ipc_table->diag_data_current_pos > 0) {
            uint32_t rest_length = equip_ipc_table->diag_data_total_length - equip_ipc_table->diag_data_current_pos;
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
            if (rest_length >= link_data->data_size) {
                ne_doip_integrate_thread_task(NE_DOIP_DIAG_SOURCE_TYPE_INTERNAL_EQUIP, link_data, pos, link_data->data_size);
                // ne_doip_pack_diagnostic_from_internal_equip(link_data, pos, link_data->data_size);
                return link_data->data_size;
            }
            else {
                ne_doip_integrate_thread_task(NE_DOIP_DIAG_SOURCE_TYPE_INTERNAL_EQUIP, link_data, pos, rest_length);
                // ne_doip_pack_diagnostic_from_internal_equip(link_data, pos, rest_length);
                return rest_length;
            }
        }
        else {
            equip_ipc_table->diag_data_total_length = 0;
            equip_ipc_table->diag_data_current_pos = 0;
            ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
        }
    }
    else {
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
    }

    unsigned char request_type = link_data->data[pos];
    pos += NE_DOIP_IN_COMMAND_LENGTH;
    uint32_t payload_data_length = 0;
    memcpy(&payload_data_length, link_data->data + pos, NE_DOIP_IN_DATA_LENGTH);
    pos += NE_DOIP_IN_DATA_LENGTH;

    switch (request_type) {
    case NE_DOIP_IN_PAYLOADTYPE_NODE_RGIST:
    {
        NE_DOIP_PRINT("request_type is NE_DOIP_IN_PAYLOADTYPE_NODE_RGIST\n");
        uint8_t pos_t = pos;
        uint16_t ecu_logical_address = 0x0000;
        memcpy(&ecu_logical_address, link_data->data + pos_t, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        ecu_logical_address = ne_doip_bswap_16(ecu_logical_address);
        pos_t += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        ne_doip_instence_type_t doip_instence_type = NE_DOIP_INSTANCE_TYPE_UNKOWN;
        memcpy(&doip_instence_type, link_data->data + pos_t, NE_DOIP_INSTANCE_TYPE_LENGTH);

        /***********Registered node information is added to ipc list***********/
        ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
        ne_doip_node_ipc_table_t* node_ipc_table = malloc(sizeof *node_ipc_table);
        memset(node_ipc_table, 0, sizeof *node_ipc_table);

        node_ipc_table->fd = link_data->fd;
        node_ipc_table->comm_type = NE_DOIP_SOCKET_TYPE_IPC;
        node_ipc_table->ecu_logical_address = ecu_logical_address;
        if (NE_DOIP_INSTANCE_TYPE_ENTITY == doip_instence_type) {
            node_ipc_table->entity_logical_address = ecu_logical_address;
        }
        ne_doip_list_insert(global_server_manager->node_ipc_list->prev, (ne_doip_list_t *) node_ipc_table);
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
        /**********************************************************************/

        NE_DOIP_PRINT("registed node address is %04X \n", ecu_logical_address);

        if (NE_DOIP_INSTANCE_TYPE_ENTITY == doip_instence_type) {
            ne_doip_net_source_t *net_source = NULL;
            ne_doip_list_for_each(net_source, global_server_manager->server->config->net_source_list, base) {
                if (NE_DOIP_IP_CONFIGURED_NOT_ANNOUNCED == net_source->announce_state) {
                    uint32_t random_time_value = ne_doip_get_random_value(NE_DOIP_A_DOIP_ANNOUNCE_WAIT_MAX);
                    net_source->announce_wait_timeid = ne_doip_timer_start(global_server_manager->timer_manager,
                                                          -1, random_time_value, ne_doip_vehicle_announce_wait_timer_callback);
                }
            }
        }
        break;
    }
    case NE_DOIP_IN_PAYLOADTYPE_REGIST_USER_CONF:
    {
        NE_DOIP_PRINT("request_type is NE_DOIP_IN_PAYLOADTYPE_REGIST_USER_CONF\n");
        ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
        ne_doip_node_ipc_table_t* node_ipc_table = ne_doip_node_ipc_list_find_by_fd(link_data->fd);
        if (node_ipc_table != NULL) {
            node_ipc_table->confirmation_flag = NE_DOIP_TRUE;
        }
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
        break;
    }
    case NE_DOIP_IN_PAYLOADTYPE_USER_CONF_RESULT:
    {
        NE_DOIP_PRINT("request_type is NE_DOIP_IN_PAYLOADTYPE_USER_CONF_RESULT\n");
        uint8_t pos_t = pos;
        uint16_t entity_logical_address = 0x0000;
        memcpy(&entity_logical_address, link_data->data + pos_t, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        entity_logical_address = ne_doip_bswap_16(entity_logical_address);
        pos_t += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        uint16_t equip_logical_address = 0x0000;
        memcpy(&equip_logical_address, link_data->data + pos_t, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        equip_logical_address = ne_doip_bswap_16(equip_logical_address);
        pos_t += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        ne_doip_user_con_result_t confirm_result = NE_DOIP_USER_CON_RESLUT_REJECT;
        memcpy(&confirm_result, link_data->data + pos_t, NE_DOIP_CONF_RESULT_LENGTH);

        ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
        ne_doip_node_ipc_table_t* ipc_entity_table = ne_doip_node_ipc_list_find_entity();

        ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
        ne_doip_node_tcp_table_t* node_tcp_table = ne_doip_node_tcp_list_find_by_logic_address(equip_logical_address);

        if (ipc_entity_table != NULL && node_tcp_table != NULL) {
            if (NE_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_CONF == node_tcp_table->connection_state) {
                if (NE_DOIP_USER_CON_RESLUT_PASS == confirm_result) {
                    node_tcp_table->connection_state = NE_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE;
                    node_tcp_table->fd_regist_flag = NE_DOIP_TRUE;
                    ne_doip_link_data_t link_data_t;
                    memset(&link_data_t, 0, sizeof link_data_t);
                    link_data_t.fd = node_tcp_table->fd;
                    link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TCP;
                    ne_doip_pack_routing_activation_res(&link_data_t, equip_logical_address,
                                                        ipc_entity_table->entity_logical_address,
                                                        NE_DOIP_RA_RES_CONFIRMATION_REQUIRED,
                                                        0x00000000);
                }
                else if (NE_DOIP_USER_CON_RESLUT_NO_CON == confirm_result) {
                    node_tcp_table->connection_state = NE_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE;
                    node_tcp_table->fd_regist_flag = NE_DOIP_TRUE;
                    ne_doip_link_data_t link_data_t;
                    memset(&link_data_t, 0, sizeof link_data_t);
                    link_data_t.fd = node_tcp_table->fd;
                    link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TCP;
                    ne_doip_pack_routing_activation_res(&link_data_t, equip_logical_address,
                                                        ipc_entity_table->entity_logical_address,
                                                        NE_DOIP_RA_RES_ROUTING_SUCCESSFULLY_ACTIVATED,
                                                        0x00000000);
                }
                else {
                    ne_doip_link_data_t link_data_t;
                    memset(&link_data_t, 0, sizeof link_data_t);
                    link_data_t.fd = node_tcp_table->fd;
                    link_data_t.comm_type = NE_DOIP_SOCKET_TYPE_TCP;
                    ne_doip_pack_routing_activation_res(&link_data_t, equip_logical_address,
                                                        ipc_entity_table->entity_logical_address,
                                                        NE_DOIP_RA_RES_REJECT_CONFIRMATION,
                                                        0x00000000);
                }
            }
            else {
                NE_DOIP_PRINT("connection_state is not NE_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_CONF\n");
            }
        }
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
        break;
    }
    case NE_DOIP_IN_PAYLOADTYPE_DIAG_REQUEST:
    {
        NE_DOIP_PRINT("request_type is NE_DOIP_IN_PAYLOADTYPE_DIAG_REQUEST\n");
        ne_doip_pack_diagnostic_from_entity(link_data, pos, payload_data_length);
        break;
    }
    case NE_DOIP_IN_EQUIP_RGIST:
    {
        NE_DOIP_PRINT("request_type is NE_DOIP_IN_EQUIP_RGIST\n");
        uint16_t equip_logical_address = 0x0000;
        memcpy(&equip_logical_address, link_data->data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        equip_logical_address = ne_doip_bswap_16(equip_logical_address);

        /***********Registered equip information is added to equip ipc list***********/
        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t* equip_ipc_table = malloc(sizeof *equip_ipc_table);
        memset(equip_ipc_table, 0, sizeof *equip_ipc_table);

        equip_ipc_table->fd = link_data->fd;
        equip_ipc_table->comm_type = NE_DOIP_SOCKET_TYPE_IPC;
        equip_ipc_table->equip_logical_address = equip_logical_address;
        ne_doip_list_insert(global_server_manager->equip_ipc_list->prev, (ne_doip_list_t *) equip_ipc_table);
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);
        /**********************************************************************/

        global_server_manager->server->config->activation_line_flag = NE_DOIP_TRUE;

        NE_DOIP_PRINT("registed equip address is %04X \n", equip_logical_address);
        break;
    }
    case NE_DOIP_IN_EQUIP_IDENTITY_REQ:
    {
        NE_DOIP_PRINT("request_type is NE_DOIP_IN_EQUIP_IDENTITY_REQ\n");
        ne_doip_pack_vehicle_identification_req(link_data);
        break;
    }
    case NE_DOIP_IN_EQUIP_IDENTITY_REQ_EID:
    {
        NE_DOIP_PRINT("request_type is NE_DOIP_IN_EQUIP_IDENTITY_REQ_EID\n");
        ne_doip_pack_vehicle_identification_req_eid(link_data, pos);
        break;
    }
    case NE_DOIP_IN_EQUIP_IDENTITY_REQ_VIN:
    {
        NE_DOIP_PRINT("request_type is NE_DOIP_IN_EQUIP_IDENTITY_REQ_VIN\n");
        ne_doip_pack_vehicle_identification_req_vin(link_data, pos);
        break;
    }
    case NE_DOIP_IN_EQUIP_ROUTING_ACTIVE:
    {
        NE_DOIP_PRINT("request_type is NE_DOIP_IN_EQUIP_ROUTING_ACTIVE\n");
        ne_doip_pack_routing_activation_req(link_data, pos, payload_data_length);
        break;
    }
    case NE_DOIP_IN_EQUIP_ALIVE_CHECK:
    {
        NE_DOIP_PRINT("request_type is NE_DOIP_IN_EQUIP_ALIVE_CHECK\n");
        ne_doip_pack_alive_check_res(link_data, pos);
        break;
    }
    case NE_DOIP_IN_EQUIP_ENTITY_STATUS:
    {
        NE_DOIP_PRINT("request_type is NE_DOIP_IN_EQUIP_ENTITY_STATUS\n");
        ne_doip_pack_entity_status_req(link_data, pos);
        break;
    }
    case NE_DOIP_IN_EQUIP_POWER_MODE:
    {
        NE_DOIP_PRINT("request_type is NE_DOIP_IN_EQUIP_POWER_MODE\n");
        ne_doip_pack_power_mode_req(link_data, pos);
        break;
    }
    case NE_DOIP_IN_EQUIP_DIAGNOSTIC:
    {
        NE_DOIP_PRINT("request_type is NE_DOIP_IN_EQUIP_DIAGNOSTIC\n");
        ne_doip_sync_start(global_server_manager->equip_ipc_list_sync);
        ne_doip_equip_ipc_table_t *equip_ipc_table = ne_doip_equip_ipc_list_find_by_fd(link_data->fd);
        if (equip_ipc_table != NULL) {
            equip_ipc_table->diag_data_total_length = payload_data_length;
        }
        ne_doip_sync_end(global_server_manager->equip_ipc_list_sync);

        if (payload_data_length > link_data->data_size - pos - NE_DOIP_BLANK_2_LENGTH - NE_DOIP_TA_TYPE_LENGTH) {
            // Big data
            ++g_task_id;
            ne_doip_pack_diagnostic_from_internal_equip(link_data, pos + NE_DOIP_BLANK_2_LENGTH,
                                                        link_data->data_size - pos - NE_DOIP_BLANK_2_LENGTH - NE_DOIP_TA_TYPE_LENGTH);
        }
        else {
            // Small data
            ne_doip_pack_diagnostic_from_internal_equip(link_data, pos + NE_DOIP_BLANK_2_LENGTH, payload_data_length);
        }

        if (payload_data_length + NE_DOIP_BLANK_2_LENGTH + NE_DOIP_TA_TYPE_LENGTH + pos >= link_data->data_size) {
            return link_data->data_size;
        }
        else {
            pos += payload_data_length;
            return pos;
        }
    }
    default:
        break;
    }

    if (payload_data_length + pos >= link_data->data_size) {
        return link_data->data_size;
    }
    else {
        pos += payload_data_length;
        return pos;
    }
}

void ne_doip_ipc_unpack(ne_doip_link_data_t *link_data)
{
    NE_DOIP_PRINT("[%s] start....\n", __FUNCTION__);
    if (NULL == link_data) {
        return;
    }

#ifdef NE_DOIP_DEBUG
    printf("recv data is ");
    uint32_t i;
    for (i = 0; i < link_data->data_size; ++i) {
        printf("%02X", (unsigned char)link_data->data[i]);
    }
    printf("\n");
#endif

    uint32_t pos = 0;
    do {
        pos = ne_doip_ipc_unpack_execute(link_data, pos);
        if (0 == pos) {
            break;
        }
    } while (link_data->data_size - pos > 0);
}

void ne_doip_vehicle_announce(const char* if_name)
{
    ne_doip_net_source_t* net_source = ne_doip_net_source_list_find_by_ifname(global_server_manager->server->config, if_name);
    if (NULL == net_source) {
        return;
    }

    net_source->announce_state = NE_DOIP_IP_CONFIGURED_NOT_ANNOUNCED;
    ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
    ne_doip_node_ipc_table_t * node_ipc_table = ne_doip_node_ipc_list_find_entity();
    if (NULL == node_ipc_table) {
        NE_DOIP_PRINT("do not announce,wait doip node regist! \n");
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
        return;
    }

    if (NE_DOIP_IP_CONFIGURED_ANNOUNCED == net_source->announce_state) {
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
        return;
    }

    uint32_t random_time_value = ne_doip_get_random_value(NE_DOIP_A_DOIP_ANNOUNCE_WAIT_MAX);
    net_source->announce_wait_timeid = ne_doip_timer_start(global_server_manager->timer_manager,
                                          -1, random_time_value, ne_doip_vehicle_announce_wait_timer_callback);

    net_source->announce_state = NE_DOIP_IP_CONFIGURED_ANNOUNCED;
    ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
}

void ne_doip_vehicle_announce_state_reset(char* if_name)
{
    ne_doip_net_source_t* net_source = ne_doip_net_source_list_find_by_ifname(global_server_manager->server->config, if_name);
    if (NULL == net_source) {
        return;
    }
    net_source->announce_state = NE_DOIP_IP_NOT_CONFIGURED_NOT_ANNOUNCED;
}

void ne_doip_broadcast_activation_line_deactive()
{
    NE_DOIP_PRINT("ne_doip_broadcast_activation_line_deactive is enter..\n");
    uint32_t data_size = NE_DOIP_HEADER_COMMON_LENGTH;
    char* send_data = malloc(data_size);
    memset(send_data, 0, data_size);

    uint32_t payload_length = 0;
    uint16_t payload_type = NE_DOIP_OUT_ACTIVATION_DEACTIVE;

    uint8_t pos = 0;
    send_data[pos] = global_server_manager->server->config->protocol_version;
    pos += NE_DOIP_PROTOCOL_VERSION_LENGTH;
    send_data[pos] = ~global_server_manager->server->config->protocol_version;
    pos += NE_DOIP_INVERSE_PROTOCOL_VERSION_LENGTH;
    memcpy(send_data + pos, &payload_type, NE_DOIP_PAYLOAD_TYPE_LENGTH);
    pos += NE_DOIP_PAYLOAD_TYPE_LENGTH;
    memcpy(send_data + pos, &payload_length, NE_DOIP_PAYLOAD_LENGTH_LENGTH);

    ne_doip_connection_t *conn = malloc(sizeof *conn);
    memset(conn, 0, sizeof *conn);

    memcpy(conn->out.data, send_data, data_size);
    conn->out.data_size = data_size;

    if (NE_DOIP_NET_TYPE_IPV6 == global_server_manager->server->config->net_type) {
        conn->fd = global_server_manager->server->ipv6_udp_socket->fd;
        ne_doip_udp6_send(conn, NE_DOIP_UDP6_BROADCAST_IP, NE_DOIP_UDP_DISCOVERY_PORT);
    }
    else {
        conn->fd = global_server_manager->server->ipv4_udp_socket->fd;
        ne_doip_udp_send(conn, NE_DOIP_UDP_BROADCAST_IP, NE_DOIP_UDP_DISCOVERY_PORT);
    }

    free(send_data);
    free(conn);
}

void ne_doip_vehicle_announce_wait_timer_callback(void* arg1)
{
    ne_doip_timer_t* timer = (ne_doip_timer_t*)arg1;
    NE_DOIP_PRINT("ne_doip_vehicle_announce_wait_timer_callback is start...timeid[%d] \n", timer->timeid);

    ne_doip_net_source_t* net_source = ne_doip_net_source_list_find_by_announce_wait_timeid(global_server_manager->server->config, timer->timeid);
    if (NULL == net_source) {
        return;
    }
    net_source->announce_state = NE_DOIP_IP_CONFIGURED_ANNOUNCED;

    ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
    ne_doip_node_ipc_table_t* node_ipc_table = ne_doip_node_ipc_list_find_entity();
    if (NULL == node_ipc_table) {
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
        return;
    }

    ne_doip_link_data_t* link_data = malloc(sizeof *link_data);
    memset(link_data, 0, sizeof *link_data);

    link_data->fd = node_ipc_table->fd;
    link_data->comm_type = NE_DOIP_SOCKET_TYPE_UDP_MOUTI;
    uint16_t entity_logical_address = node_ipc_table->entity_logical_address;
    ne_doip_sync_end(global_server_manager->node_ipc_list_sync);

    if (global_server_manager->server->config->authen_info != 0
        || NE_DOIP_TRUE == node_ipc_table->confirmation_flag) {
        ne_doip_pack_announce_or_identityresponse(link_data, entity_logical_address,
                                                  NE_DOIP_FURTHER_ACTION_REQUIRED);
    }
    else {
        ne_doip_pack_announce_or_identityresponse(link_data, entity_logical_address,
                                                  NE_DOIP_NO_FURTHER_ACTION_REQUIRED);
    }
    free(link_data);
}

void ne_doip_vehicle_announce_interval_timer_callback(void* arg1)
{
    ne_doip_timer_t* timer = (ne_doip_timer_t*)arg1;
    NE_DOIP_PRINT("ne_doip_vehicle_announce_interval_timer_callback is start...timeid[%d] \n", timer->timeid);

    ne_doip_net_source_t* net_source = ne_doip_net_source_list_find_by_announce_interval_timeid(global_server_manager->server->config, timer->timeid);
    if (NULL == net_source) {
        return;
    }

    ne_doip_link_data_t* link_data = malloc(sizeof *link_data);
    memset(link_data, 0, sizeof *link_data);

    if (2 == net_source->announce_count) {
        net_source->announce_interval_timeid = 0;
        timer->m_bIterate = 0;
        ne_doip_timer_stop(global_server_manager->timer_manager, timer->timeid);
    }

    ne_doip_sync_start(global_server_manager->node_ipc_list_sync);
    ne_doip_node_ipc_table_t* node_ipc_table = ne_doip_node_ipc_list_find_entity();
    if (NULL == node_ipc_table) {
        ne_doip_sync_end(global_server_manager->node_ipc_list_sync);
        free(link_data);
        return;
    }

    link_data->fd = node_ipc_table->fd;
    link_data->comm_type = NE_DOIP_SOCKET_TYPE_UDP_MOUTI;
    uint16_t entity_logical_address = node_ipc_table->entity_logical_address;
    ne_doip_sync_end(global_server_manager->node_ipc_list_sync);

    if (global_server_manager->server->config->authen_info != 0
        || NE_DOIP_TRUE == node_ipc_table->confirmation_flag) {
        ne_doip_pack_announce_or_identityresponse(link_data, entity_logical_address,
                                                  NE_DOIP_FURTHER_ACTION_REQUIRED);
    }
    else {
        ne_doip_pack_announce_or_identityresponse(link_data, entity_logical_address,
                                                  NE_DOIP_NO_FURTHER_ACTION_REQUIRED);
    }
    free(link_data);
}

void ne_doip_vehicle_indentify_wait_timer_callback(void* arg1)
{
    ne_doip_timer_t* timer = (ne_doip_timer_t*)arg1;
    NE_DOIP_PRINT("ne_doip_vehicle_indentify_wait_timer_callback is start...timeid[%d] \n", timer->timeid);
    ne_doip_sync_start(global_server_manager->node_udp_list_sync);
    ne_doip_node_udp_table_t* udp_table = ne_doip_node_udp_list_find_by_timerid(timer->timeid);
    ne_doip_timer_stop(global_server_manager->timer_manager, timer->timeid);
    if (NULL == udp_table) {
        ne_doip_sync_end(global_server_manager->node_udp_list_sync);
        return;
    }
    udp_table->vehicle_identify_wait_timeid = 0;
    ne_doip_link_data_t* link_data = malloc(sizeof *link_data);
    memset(link_data, 0, sizeof *link_data);

    link_data->fd = udp_table->fd;
    link_data->ip = (char*)malloc(strlen(udp_table->ip) + 1);
    if (NULL != link_data->ip) {
        memcpy(link_data->ip, udp_table->ip, strlen(udp_table->ip) + 1);
    }
    link_data->port = udp_table->port;
    link_data->comm_type = NE_DOIP_SOCKET_TYPE_UDP_UNI;
    ne_doip_sync_end(global_server_manager->node_udp_list_sync);

    ne_doip_node_ipc_table_t* ipc_entity_table = ne_doip_node_ipc_list_find_entity();
    if (ipc_entity_table != NULL) {
        if (global_server_manager->server->config->authen_info != 0
            || NE_DOIP_TRUE == ipc_entity_table->confirmation_flag) {
            ne_doip_pack_announce_or_identityresponse(link_data, ipc_entity_table->entity_logical_address,
                                                      NE_DOIP_FURTHER_ACTION_REQUIRED);
        }
        else {
            ne_doip_pack_announce_or_identityresponse(link_data, ipc_entity_table->entity_logical_address,
                                                      NE_DOIP_NO_FURTHER_ACTION_REQUIRED);
        }
    }

    if (link_data->ip != NULL) {
        free(link_data->ip);
    }
    free(link_data);
}

void ne_doip_initial_timer_callback(void* arg1)
{
    ne_doip_timer_t* timer = (ne_doip_timer_t*)arg1;
    NE_DOIP_PRINT("ne_doip_initial_timer_callback is start...timeid[%d] \n", timer->timeid);
    ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
    ne_doip_node_tcp_table_t* node_tcp_table = ne_doip_node_tcp_list_find_by_timerid(timer->timeid);
    ne_doip_timer_stop(global_server_manager->timer_manager, timer->timeid);
    if (NULL == node_tcp_table) {
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
        return;
    }
    int fd = node_tcp_table->fd;
    ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
    // Close the socket and remove it from the list
    ne_doip_server_disconnect(global_server_manager->server, NE_DOIP_SOCKET_TYPE_TCP, fd);
}

void ne_doip_general_timer_callback(void* arg1)
{
    ne_doip_timer_t* timer = (ne_doip_timer_t*)arg1;
    NE_DOIP_PRINT("ne_doip_general_timer_callback is start...timeid[%d] \n", timer->timeid);
    ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
    ne_doip_node_tcp_table_t* node_tcp_table = ne_doip_node_tcp_list_find_by_timerid(timer->timeid);
    ne_doip_timer_stop(global_server_manager->timer_manager, timer->timeid);
    if (NULL == node_tcp_table) {
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
        return;
    }
    int fd = node_tcp_table->fd;
    ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
    // Close the socket and remove it from the list
    ne_doip_server_disconnect(global_server_manager->server, NE_DOIP_SOCKET_TYPE_TCP, fd);
}

void ne_doip_alive_check_timer_callback(void* arg1)
{
    ne_doip_timer_t* timer = (ne_doip_timer_t*)arg1;
    NE_DOIP_PRINT("ne_doip_alive_check_timer_callback is start...timeid[%d] \n", timer->timeid);
    ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
    ne_doip_node_tcp_table_t* node_tcp_table = ne_doip_node_tcp_list_find_by_timerid(timer->timeid);
    ne_doip_timer_stop(global_server_manager->timer_manager, timer->timeid);
    if (NULL == node_tcp_table) {
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
        return;
    }
    if (NE_DOIP_TRUE == node_tcp_table->single_alive_check_flag) {
        ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
        ne_doip_node_tcp_table_t* routing_tcp_table = ne_doip_node_tcp_list_find_by_fd(node_tcp_table->routing_fd);
        if (routing_tcp_table != NULL) {
            /* The registered tcp socket does not receive an alive check response,
               assigning the SA to the newly requested tcp socket */

            routing_tcp_table->equip_logical_address = node_tcp_table->equip_logical_address;
            routing_tcp_table->fd_regist_flag = NE_DOIP_TRUE;
            routing_tcp_table->connection_state = NE_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE;

            ne_doip_link_data_t link_data;
            link_data.fd = routing_tcp_table->fd;
            link_data.comm_type = NE_DOIP_SOCKET_TYPE_TCP;

            ne_doip_routing_activation_check_authentication(&link_data, routing_tcp_table->authen_info);
        }
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
    }
    else {
        if (++global_server_manager->alive_check_ncts == global_server_manager->server->config->mcts) {
            global_server_manager->alive_check_ncts = 0;
        }
        uint16_t logical_routing_address = node_tcp_table->logical_routing_address;
        ne_doip_sync_start(global_server_manager->node_tcp_list_sync);
        ne_doip_node_tcp_table_t *routing_tcp_table = ne_doip_node_tcp_list_find_by_logic_address(logical_routing_address);
        if (routing_tcp_table != NULL) {
            // All alive check found that the socket is invalid
            if (NE_DOIP_FALSE == routing_tcp_table->fd_regist_flag) {
                // assigning the SA to the newly requested tcp socket
                routing_tcp_table->equip_logical_address = logical_routing_address;
                routing_tcp_table->fd_regist_flag = NE_DOIP_TRUE;
                routing_tcp_table->connection_state = NE_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE;

                ne_doip_link_data_t link_data;
                link_data.fd = routing_tcp_table->fd;
                link_data.comm_type = NE_DOIP_SOCKET_TYPE_TCP;

                ne_doip_routing_activation_check_authentication(&link_data, routing_tcp_table->authen_info);
            }
        }
        ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
    }
    ne_doip_sync_end(global_server_manager->node_tcp_list_sync);
    // Close the socket and remove it from the list
    ne_doip_server_disconnect(global_server_manager->server, NE_DOIP_SOCKET_TYPE_TCP, node_tcp_table->fd);
}

void ne_doip_integrate_thread_task(ne_doip_diag_source_type_t diag_source_type,
                                   ne_doip_link_data_t *link_data,
                                   uint32_t pos, uint32_t payload_length)
{
    ne_doip_task_t *task = malloc(sizeof *task);
    memset(task, 0, sizeof *task);
    task->id = g_task_id;

    ne_doip_integrate_task_t *integrate_task = malloc(sizeof(ne_doip_integrate_task_t));
    memset(integrate_task, 0, sizeof(ne_doip_integrate_task_t));

    integrate_task->diag_source_type = diag_source_type;
    integrate_task->link_data = malloc(sizeof(ne_doip_link_data_t));
    memset(integrate_task->link_data, 0, sizeof(ne_doip_link_data_t));
    memcpy(integrate_task->link_data, link_data, sizeof(ne_doip_link_data_t));
    integrate_task->link_data->data = malloc(link_data->data_size);
    memset(integrate_task->link_data->data, 0, link_data->data_size);
    memcpy(integrate_task->link_data->data, link_data->data, link_data->data_size);
    integrate_task->pos = pos;
    integrate_task->payload_length = payload_length;
    task->data = (void*)integrate_task;
    

    int res = ne_doip_threadpool_push(global_server_manager->threadpool, task);
    if (res >= 0) {
        NE_DOIP_PRINT("ne_doip_integrate_thread_task push succeed...task id is %d\n", g_task_id);
    }
    else {
        NE_DOIP_PRINT("ne_doip_integrate_thread_task push error...task id is %d\n", g_task_id);
    }
}

void ne_doip_thread_callback_func(void *data, void *user_data)
{
    ne_doip_task_t *task = (ne_doip_task_t *)data;
    if (NULL == task) {
        return;
    }
    NE_DOIP_PRINT("ne_doip_thread_callback_func is enter...task id is is %d\n", task->id);

    ne_doip_integrate_task_t *integrate_task = (ne_doip_integrate_task_t*)task->data;
    if (NULL == integrate_task) {
        return;
    }

    if (NE_DOIP_DIAG_SOURCE_TYPE_INTERNAL_EQUIP == integrate_task->diag_source_type) {
        NE_DOIP_PRINT("excute [ne_doip_pack_diagnostic_from_internal_equip].. \n");
        ne_doip_pack_diagnostic_from_internal_equip(integrate_task->link_data, integrate_task->pos, integrate_task->payload_length);
    }
    else if (NE_DOIP_DIAG_SOURCE_TYPE_EXTERNAL_EQUIP == integrate_task->diag_source_type) {
        NE_DOIP_PRINT("excute [ne_doip_unpack_diagnostic].. \n");
        ne_doip_unpack_diagnostic(integrate_task->link_data, integrate_task->pos, integrate_task->payload_length);
    }

    free(integrate_task->link_data->data);
    free(integrate_task->link_data);
    free(integrate_task);
}
/* EOF */