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
#include <sys/un.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <net/if.h>
#include <fcntl.h>

#include "ne_doip_server.h"
#include "ne_doip_def.h"
#include "ne_doip_os.h"
#include "ne_doip_config.h"
#include "ne_doip_server_manager.h"
#include "ne_doip_util.h"
#include "ne_doip_select.h"

// ********************************************************************
// date definition
// ********************************************************************

/// define client type
enum NE_DOIP_CLIENT_TYPE
{
    NE_DOIP_CLIENT_TYPE_IPC  = 0x01,    ///< ipc lient type
    NE_DOIP_CLIENT_TYPE_NET  = 0x02,    ///< net lient type
    NE_DOIP_CLIENT_TYPE_TEST = 0x03,    ///< test lient type
    NE_DOIP_CLIENT_TYPE_FREE = 0x0F     ///< free lient type
};

#define NE_DOIP_IPC_PATH            "/tmp/"
#define NE_DOIP_SERVER_NAME         "doip_server"
#define NE_DOIP_EPOLL_IPC_THREAD    "NE_DOIP_EPOLL_IPC_THREAD"
#define NE_DOIP_EPOLL_NET_THREAD    "NE_DOIP_EPOLL_NET_THREAD"

#define NE_DOIP_TYPE_IPC    1
#define NE_DOIP_TYPE_NET    2

static ne_doip_server_t *global_server = NULL;

// ********************************************************************
// internal interface
// ********************************************************************

static ne_doip_server_t *
ne_doip_server_create(void)
{
    ne_doip_server_t *server;

    server = malloc(sizeof *server);
    memset(server, 0, sizeof *server);

    server->sel_ipc = ne_doip_select_create();
    if (NULL == server->sel_ipc) {
        NE_DOIP_PRINT("ne_doip_server_create sel_ipc error\n");
        free(server);
        return NULL;
    }

    server->sel_net = ne_doip_select_create();
    if (NULL == server->sel_net) {
        NE_DOIP_PRINT("ne_doip_server_create sel_net error\n");
        ne_doip_select_destroy(server->sel_ipc);
        free(server);
        return NULL;
    }

    server->ipc_list_sync = ne_doip_sync_create();
    server->net_list_sync = ne_doip_sync_create();
    server->test_list_sync = ne_doip_sync_create();
    server->net_list_free_sync = ne_doip_sync_create();

    // create ipc client list
    ne_doip_client_t *list;
    list = malloc(sizeof *list);
    memset(list, 0, sizeof *list);
    list->client_type = NE_DOIP_CLIENT_TYPE_IPC;
    ne_doip_list_init((ne_doip_list_t *)list);
    server->ipc_client_list = (ne_doip_list_t *)list;

    // create net client list
    list = malloc(sizeof *list);
    memset(list, 0, sizeof *list);
    list->client_type = NE_DOIP_CLIENT_TYPE_NET;
    ne_doip_list_init((ne_doip_list_t *)list);
    server->net_client_list = (ne_doip_list_t *)list;

    // create test client list
    list = malloc(sizeof *list);
    memset(list, 0, sizeof *list);
    list->client_type = NE_DOIP_CLIENT_TYPE_TEST;
    ne_doip_list_init((ne_doip_list_t *)list);
    server->test_client_list = (ne_doip_list_t *)list;

    // create client free list
    list = malloc(sizeof *list);
    memset(list, 0, sizeof *list);
    list->client_type = NE_DOIP_CLIENT_TYPE_FREE;
    ne_doip_list_init((ne_doip_list_t *)list);
    server->client_list_free = (ne_doip_list_t *)list;

    return server;
}

static ne_doip_sync_t *
ne_doip_get_sync(ne_doip_server_t *server, int type)
{
    ne_doip_sync_t *sync = NULL;

    switch (type) {
    case NE_DOIP_CLIENT_TYPE_IPC:
        sync = server->ipc_list_sync;
        break;
    case NE_DOIP_CLIENT_TYPE_NET:
        sync = server->net_list_sync;
        break;
    case NE_DOIP_CLIENT_TYPE_TEST:
        sync = server->test_list_sync;
        break;
    default:
        break;
    }

    return sync;
}

static void
ne_doip_free_client(ne_doip_server_t *server, int type)
{
    ne_doip_sync_start(server->net_list_free_sync);

    int cnt = ne_doip_list_length(server->client_list_free);
    if (cnt == 0) {
        ne_doip_sync_end(server->net_list_free_sync);
        return;
    }
    NE_DOIP_PRINT("ne_doip_free_client start cnt:[%d]\n", cnt);

    ne_doip_client_t *client;
    ne_doip_client_t *tmp;

    ne_doip_list_for_each_safe(client, tmp, server->client_list_free, base) {
        if (type != NE_DOIP_CLIENT_TYPE_FREE) {
            if (type == NE_DOIP_CLIENT_TYPE_IPC) {
                if (client->client_type != type) {
                    // when type is ipc, skip other type client, only free ipc client[in ipc epoll thread]
                    continue;
                }
            }
            else {
                if (client->client_type == NE_DOIP_CLIENT_TYPE_IPC) {
                    // when type is not ipc, skip ipc client, free all other client(tcp. udp. test)[in net epoll thread]
                    continue;
                }
            }
        }

        free(client->source);
        client->source = NULL;

        // fd has been closed during source remove
        free(client->connection);
        client->connection = NULL;

        ne_doip_list_remove((ne_doip_list_t *) client);
        free(client);
    }

    cnt = ne_doip_list_length(server->client_list_free);
    NE_DOIP_PRINT("ne_doip_free_client end cnt:[%d]\n", cnt);

    ne_doip_sync_end(server->net_list_free_sync);
}

static void
ne_doip_client_list_destroy(ne_doip_server_t *server, ne_doip_list_t *list)
{
    if (!list) {
        return;
    }

    ne_doip_client_t *client_list = (ne_doip_client_t *)list;
    ne_doip_sync_t *sync = ne_doip_get_sync(server, client_list->client_type);
    if (!sync) {
        return;
    }

    ne_doip_sync_start(sync);

    ne_doip_client_t *client;
    ne_doip_client_t *tmp;

    ne_doip_list_for_each_safe(client, tmp, list, base) {
        ne_doip_event_source_remove(client->source);

        ne_doip_list_t *list_node = (ne_doip_list_t *) client;
        ne_doip_list_remove(list_node);

        ne_doip_sync_start(server->net_list_free_sync);
        ne_doip_list_init(list_node);
        ne_doip_list_insert(server->client_list_free, list_node);
        ne_doip_sync_end(server->net_list_free_sync);
    }

    free(list);

    ne_doip_sync_end(sync);
}

static void
ne_doip_server_destroy(ne_doip_server_t *server)
{
    if (server == NULL) {
        return;
    }

    ne_doip_server_manager_destroy();
    ne_doip_unload_config(server->config);

    ne_doip_client_list_destroy(server, server->ipc_client_list);
    ne_doip_client_list_destroy(server, server->net_client_list);
    ne_doip_client_list_destroy(server, server->test_client_list);
    ne_doip_free_client(server, NE_DOIP_CLIENT_TYPE_FREE);
    NE_DOIP_PRINT("ne_doip_server_destroy client_list_free length:[%d]\n", ne_doip_list_length(server->client_list_free));

    ne_doip_sync_destroy(server->ipc_list_sync);
    ne_doip_sync_destroy(server->net_list_sync);
    ne_doip_sync_destroy(server->test_list_sync);
    ne_doip_sync_destroy(server->net_list_free_sync);

    ne_doip_select_destroy(server->sel_ipc);
    ne_doip_select_destroy(server->sel_net);

    free(server);
}

static void
ne_doip_server_stop(ne_doip_server_t *server)
{
    server->ipc_run = 0;
    server->net_run = 0;

    // ne_doip_notify epoll_wait to continue and quit thread
    int ret = ne_doip_select_notify(server->sel_ipc);
    if (ret < 0) {
        NE_DOIP_PRINT("server_stop failed to notify sel_ipc\n");
    }

    ret = ne_doip_select_notify(server->sel_net);
    if (ret < 0) {
        NE_DOIP_PRINT("server_stop failed to notify sel_net\n");
    }

    // stop ipc epoll thread
    if (server->epoll_ipc_thread != NULL) {
        ne_doip_thread_stop(server->epoll_ipc_thread);
        server->epoll_ipc_thread = NULL;
    }

    // stop net epoll thread
    if (server->epoll_net_thread != NULL) {
        ne_doip_thread_stop(server->epoll_net_thread);
        server->epoll_net_thread = NULL;
    }
}

static void
ne_doip_client_destroy(ne_doip_client_t *client)
{
    ne_doip_sync_t *sync = ne_doip_get_sync(client->server, client->client_type);
    if (!sync) {
        return;
    }

    ne_doip_sync_start(sync);
    ne_doip_event_source_remove(client->source);

    ne_doip_list_t *list_node = (ne_doip_list_t *)client;
    ne_doip_list_remove(list_node);

    ne_doip_sync_start(client->server->net_list_free_sync);
    ne_doip_list_init(list_node);
    ne_doip_list_insert(client->server->client_list_free, list_node);
    ne_doip_sync_end(client->server->net_list_free_sync);

    ne_doip_remove_connection_table(client->connection->fd, client->client_type);
    ne_doip_sync_end(sync);
}


static int
ne_doip_client_connection(int fd, uint32_t mask, void *data)
{
    int len;
    ne_doip_client_t *client = data;
    ne_doip_connection_t *conn = client->connection;
    NE_DOIP_PRINT("ne_doip_client_connection fd:[%d], fd_s:[%d], fd_c:[%d]\n", fd, client->source->fd, conn->fd);

    if (mask & NE_DOIP_EVENT_ERROR) {
        NE_DOIP_PRINT("ne_doip_client_connection NE_DOIP_EVENT_ERROR! code:%d, message:%s\n", errno, strerror(errno));
        ne_doip_client_destroy(client);
        return -1;
    }

    if (mask & NE_DOIP_EVENT_HANGUP) {
        NE_DOIP_PRINT("ne_doip_client_connection NE_DOIP_EVENT_HANGUP! code:%d, message:%s\n", errno, strerror(errno));
        ne_doip_client_destroy(client);
        return -1;
    }

    if (mask & NE_DOIP_EVENT_WRITABLE) {
        errno = 0;
        len = ne_doip_connection_write(conn);
        if (len < 0 && errno != EAGAIN) {
            NE_DOIP_PRINT("ne_doip_client_connection write error! code:%d, message:%s\n", errno, strerror(errno));
            ne_doip_client_destroy(client);
            return -1;
        }
        else if (len >= 0) {
            ne_doip_event_source_fd_update(client->source, NE_DOIP_EVENT_READABLE);
        }
    }

    len = 0;
    if (mask & NE_DOIP_EVENT_READABLE) {
        errno = 0;
        len = ne_doip_connection_read(conn);
        if (len < 0 && errno != EAGAIN) {
            NE_DOIP_PRINT("ne_doip_client_connection read error! code:%d, message:%s\n", errno, strerror(errno));
            ne_doip_client_destroy(client);
            return -1;
        }
        else if (len == 0 && errno != EAGAIN) {
            NE_DOIP_PRINT("ne_doip_client_connection read EOF, socket closed\n");
            ne_doip_client_destroy(client);
            return -1;
        }

        // Parsing the protocol through the manager interface
        ne_doip_link_data_t* link_data = malloc(sizeof *link_data);
        memset(link_data, 0, sizeof *link_data);

        link_data->fd = conn->fd;
        link_data->data_size = conn->in.data_size;

        link_data->data = malloc(conn->in.data_size);
        memset(link_data->data, 0, conn->in.data_size);

        memcpy(link_data->data, conn->in.data, conn->in.data_size);
        link_data->comm_type = client->client_type;

        if (NE_DOIP_CLIENT_TYPE_IPC == client->client_type) {
            ne_doip_ipc_unpack(link_data);
        }
        else {
            ne_doip_net_unpack(link_data);
        }
        free(link_data->data);
        free(link_data);
    }

    return 0;
}

static ne_doip_client_t *
ne_doip_client_create(ne_doip_server_t *server, ne_doip_select_t *sel_ptr, int fd)
{
    ne_doip_client_t *client;

    client = malloc(sizeof *client);
    memset(client, 0, sizeof *client);

    ne_doip_list_init((ne_doip_list_t *)client);

    client->connection = ne_doip_connection_create(fd);
    if (client->connection == NULL) {
        NE_DOIP_PRINT("client_create connection error!\n");
        free(client);
        return NULL;
    }

    client->server = server;
    client->source = ne_doip_event_source_create(sel_ptr, fd,
                          ne_doip_client_connection, client);

    if (!client->source) {
        NE_DOIP_PRINT("client_create source error!\n");
        free(client->connection);
        free(client);
        return NULL;
    }

    return client;
}

static int
ne_doip_ipc_socket_data(int fd, uint32_t mask, void *data)
{
    NE_DOIP_PRINT("ne_doip_ipc_socket_data enter\n");

    ne_doip_server_t *server = (ne_doip_server_t *)data;
    struct sockaddr_un addr;
    socklen_t length;
    int client_fd;

    length = sizeof addr;
    client_fd = ne_doip_os_accept(fd, (struct sockaddr *) &addr, &length);
    if (client_fd < 0) {
        NE_DOIP_PRINT("ne_doip_ipc_socket_data client_fd error!\n");
        return -1;
    }

    if (global_server->config == NULL) {
        return -1;
    }

    NE_DOIP_PRINT("ne_doip_ipc_socket_data accept fd:%d, sun_path:%s\n", client_fd, addr.sun_path);

    ne_doip_client_t *client;
    client = ne_doip_client_create(server, server->sel_ipc, client_fd);
    if (!client) {
        close(client_fd);
        return -1;
    }

    client->client_type = NE_DOIP_CLIENT_TYPE_IPC;

    ne_doip_sync_start(server->ipc_list_sync);
    length = ne_doip_list_length(server->ipc_client_list);
    NE_DOIP_PRINT("ne_doip_ipc_socket_data ipc_client_list length:[%d]\n", length);
    ne_doip_list_insert(server->ipc_client_list->prev, (ne_doip_list_t *) client);
    ne_doip_sync_end(server->ipc_list_sync);

    // client must create completely first
    // source adding into I/O event notification must be last step!
    if (ne_doip_source_add(server->sel_ipc, client->source, NE_DOIP_EVENT_READABLE) < 0) {
        NE_DOIP_PRINT("ipc_socket_data source add error!\n");
        ne_doip_list_remove((ne_doip_list_t *)client);
        free(client->connection);
        free(client->source);
        free(client);
        close(client_fd);
        return -1;
    }
    return 0;
}

static int
ne_doip_tcp_socket_data(int fd, uint32_t mask, void *data)
{
    ne_doip_server_t *server = (ne_doip_server_t *)data;
    struct sockaddr_in addr;
    socklen_t length = sizeof addr;
    memset(&addr, 0, length);
    int client_fd;

    client_fd = ne_doip_os_accept(fd, (struct sockaddr *) &addr, &length);
    if (client_fd < 0) {
        NE_DOIP_PRINT("ne_doip_tcp_socket_data client_fd error!\n");
        return -1;
    }

    if (global_server->config == NULL) {
        return -1;
    }

    if (NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server->config->entity_type) {
        if (NE_DOIP_FALSE == global_server->config->activation_line_flag) {
            NE_DOIP_PRINT("activation_line_flag is false!\n");
            return -1;
        }
    }

    ne_doip_sync_start(server->net_list_sync);
    uint16_t current_open_socket_num = ne_doip_list_length(server->net_client_list);
    ne_doip_sync_end(server->net_list_sync);
    if (current_open_socket_num >= server->config->mcts + 1) {
        NE_DOIP_PRINT("The number of currently open sockets has reached the maximum!\n");
        return -1;
    }

    char *ip = inet_ntoa(addr.sin_addr);
    uint16_t port = ntohs(addr.sin_port);
    NE_DOIP_PRINT("ne_doip_tcp_socket_data accept fd:[%d], addr:[%s], port:[%u]\n", client_fd, ip, port);

    if (NULL == ip || 0 == strcmp(NE_DOIP_UDP_BROADCAST_IP, ip)) {
        return -1;
    }

    ne_doip_client_t *client;
    client = ne_doip_client_create(server, server->sel_net, client_fd);
    if (!client) {
        close(client_fd);
        return -1;
    }

    // Can determine which [IF] has received the data
    struct sockaddr_in connected_addr;
    socklen_t connected_addr_len = sizeof connected_addr;
    getsockname(client_fd, (struct sockaddr *)&connected_addr, &connected_addr_len);
    char *target_ip = inet_ntoa(connected_addr.sin_addr);
    char ifname[16] = { 0 };
    ne_doip_os_get_ifname(ifname, client_fd, target_ip);
    NE_DOIP_PRINT("finded ifname is [%s].\n", ifname);

    client->client_type = NE_DOIP_CLIENT_TYPE_NET;

    ne_doip_sync_start(server->net_list_sync);
    ne_doip_list_insert(server->net_client_list->prev, (ne_doip_list_t *) client);
    ne_doip_sync_end(server->net_list_sync);

    // client must create completely first
    // source adding into I/O event notification must be last step!
    if (ne_doip_source_add(server->sel_net, client->source, NE_DOIP_EVENT_READABLE) < 0) {
        NE_DOIP_PRINT("tcp_socket_data source add error!\n");
        ne_doip_list_remove((ne_doip_list_t *)client);
        free(client->connection);
        free(client->source);
        free(client);
        close(client_fd);
        return -1;
    }

    // Append table data to the connection table list of the manager
    ne_doip_addr_data_t* addr_data = malloc(sizeof *addr_data);
    memset(addr_data, 0, sizeof(ne_doip_addr_data_t));

    addr_data->fd = client_fd;
    addr_data->ip = (char*)malloc(strlen(ip) + 1);

    memcpy(addr_data->ip, ip, strlen(ip) + 1);
    addr_data->port = port;
    addr_data->comm_type = NE_DOIP_SOCKET_TYPE_TCP;
    addr_data->connection_state = NE_DOIP_CONNECT_STATE_INITIALIZED;
    ne_doip_add_connection_table(addr_data);

    free(addr_data->ip);
    free(addr_data);

    return 0;
}

static int
ne_doip_tcp6_socket_data(int fd, uint32_t mask, void *data)
{
    ne_doip_server_t *server = (ne_doip_server_t *)data;
    struct sockaddr_in6 addr;
    socklen_t length = sizeof addr;
    memset(&addr, 0, length);
    int client_fd;

    client_fd = ne_doip_os_accept(fd, (struct sockaddr *) &addr, &length);
    if (client_fd < 0) {
        NE_DOIP_PRINT("tcp6_socket_data client_fd error!\n");
        return -1;
    }

    if (global_server->config == NULL) {
        return -1;
    }

    if (NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server->config->entity_type) {
        if (NE_DOIP_FALSE == global_server->config->activation_line_flag) {
            NE_DOIP_PRINT("activation_line_flag is false!\n");
            return -1;
        }
    }

    ne_doip_sync_start(server->net_list_sync);
    uint16_t current_open_socket_num = ne_doip_list_length(server->net_client_list);
    ne_doip_sync_end(server->net_list_sync);
    if (current_open_socket_num >= server->config->mcts + 1) {
        NE_DOIP_PRINT("The number of currently open sockets has reached the maximum!\n");
        return -1;
    }

    char ip[INET6_ADDRSTRLEN];
    memset(ip, 0, INET6_ADDRSTRLEN);
    if (inet_ntop(AF_INET6, &addr.sin6_addr, ip, INET6_ADDRSTRLEN) == NULL) {
        NE_DOIP_PRINT("tcp6_socket_data convert error!\n");
        return -1;
    }
    uint16_t port = ntohs(addr.sin6_port);
    NE_DOIP_PRINT("tcp6_socket_data accept fd:[%d], addr:[%s], port:[%u]\n", client_fd, ip, port);

    if (0 == strcmp(NE_DOIP_UDP6_BROADCAST_IP, ip)) {
        return -1;
    }

    ne_doip_client_t *client;
    client = ne_doip_client_create(server, server->sel_net, client_fd);
    if (!client) {
        close(client_fd);
        return -1;
    }

    client->client_type = NE_DOIP_CLIENT_TYPE_NET;

    ne_doip_sync_start(server->net_list_sync);
    ne_doip_list_insert(server->net_client_list->prev, (ne_doip_list_t *) client);
    ne_doip_sync_end(server->net_list_sync);

    // client must create completely first
    // source adding into I/O event notification must be last step!
    if (ne_doip_source_add(server->sel_net, client->source, NE_DOIP_EVENT_READABLE) < 0) {
        NE_DOIP_PRINT("tcp6_socket_data source add error!\n");
        ne_doip_list_remove((ne_doip_list_t *)client);
        free(client->connection);
        free(client->source);
        free(client);
        close(client_fd);
        return -1;
    }

    // Append table data to the connection table list of the manager
    ne_doip_addr_data_t* addr_data = malloc(sizeof *addr_data);
    memset(addr_data, 0, sizeof(ne_doip_addr_data_t));

    addr_data->fd = client_fd;
    addr_data->ip = (char*)malloc(strlen(ip) + 1);

    memcpy(addr_data->ip, ip, strlen(ip) + 1);
    addr_data->port = port;
    addr_data->comm_type = NE_DOIP_SOCKET_TYPE_TCP;
    addr_data->connection_state = NE_DOIP_CONNECT_STATE_INITIALIZED;
    ne_doip_add_connection_table(addr_data);

    free(addr_data->ip);
    free(addr_data);

    return 0;
}

static int
ne_doip_udp_socket_data(int fd, uint32_t mask, void *data)
{
    int byte_count;
    struct sockaddr_in addr;
    socklen_t size;
    size = sizeof addr;

    ne_doip_server_t *server = (ne_doip_server_t *)data;
    ne_doip_client_t *client = server->udp_client;


    byte_count = ne_doip_connection_recvfrom(client->connection, (struct sockaddr*)&addr, &size);
    if (byte_count <= 0) {
        return -1;
    }
    else {

        if (global_server->config == NULL) {
            return -1;
        }

        if (NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server->config->entity_type) {
            if (NE_DOIP_FALSE == global_server->config->activation_line_flag) {
                NE_DOIP_PRINT("activation_line_flag is false!\n");
                return -1;
            }
        }

        client->connection->fd = fd;

        char *ip = inet_ntoa(addr.sin_addr);
        uint16_t port = ntohs(addr.sin_port);
        NE_DOIP_PRINT("udp socketfd is [%d].ip is [%s], port is [%u]\n", client->connection->fd, ip, port);

        if (NULL == ip || 0 == strcmp(NE_DOIP_UDP_BROADCAST_IP, ip)) {
            return -1;
        }

        // Parsing the protocol through the manager interface
        ne_doip_link_data_t* link_data = malloc(sizeof *link_data);
        memset(link_data, 0, sizeof(ne_doip_link_data_t));

        link_data->fd = fd;
        link_data->comm_type = NE_DOIP_SOCKET_TYPE_UNKNOWN;
        link_data->ip = (char*)malloc(strlen(ip) + 1);

        memcpy(link_data->ip, ip, strlen(ip) + 1);
        link_data->port = port;
        link_data->data_size = client->connection->in.data_size;
        link_data->data = malloc(client->connection->in.data_size);
        memset(link_data->data, 0, client->connection->in.data_size);

        memcpy(link_data->data, client->connection->in.data, client->connection->in.data_size);
        ne_doip_net_unpack(link_data);
        free(link_data->data);
        free(link_data);
    }

    return 0;
}

static int
ne_doip_udp6_socket_data(int fd, uint32_t mask, void *data)
{
    int byte_count;
    struct sockaddr_in6 addr;
    socklen_t size = sizeof addr;
    memset(&addr, 0, size);

    ne_doip_server_t *server = (ne_doip_server_t *)data;
    ne_doip_client_t *client = server->udp6_client;


    byte_count = ne_doip_connection_recvfrom(client->connection, (struct sockaddr*)&addr, &size);
    if (byte_count <= 0) {
        return -1;
    }
    else {
        if (global_server->config == NULL) {
            return -1;
        }

        if (NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server->config->entity_type) {
            if (NE_DOIP_FALSE == global_server->config->activation_line_flag) {
                NE_DOIP_PRINT("activation_line_flag is false!\n");
                return -1;
            }
        }

        char ip[INET6_ADDRSTRLEN];
        memset(ip, 0, INET6_ADDRSTRLEN);
        if (inet_ntop(AF_INET6, &addr.sin6_addr, ip, INET6_ADDRSTRLEN) == NULL) {
            NE_DOIP_PRINT("udp_socket6_data convert error!\n");
        }
        uint16_t port = ntohs(addr.sin6_port);
        NE_DOIP_PRINT("udp_socket6_data addr:[%s], port:[%u]\n", ip, port);

        if (NULL == ip || 0 == strcmp(NE_DOIP_UDP6_BROADCAST_IP, ip)) {
            return -1;
        }

        // Parsing the protocol through the manager interface
        ne_doip_link_data_t* link_data = malloc(sizeof *link_data);
        memset(link_data, 0, sizeof(ne_doip_link_data_t));

        link_data->fd = fd;
        link_data->comm_type = NE_DOIP_SOCKET_TYPE_UNKNOWN;
        link_data->ip = (char*)malloc(strlen(ip) + 1);

        memcpy(link_data->ip, ip, strlen(ip) + 1);
        link_data->port = port;
        link_data->data_size = client->connection->in.data_size;
        link_data->data = malloc(client->connection->in.data_size);
        memset(link_data->data, 0, client->connection->in.data_size);

        memcpy(link_data->data, client->connection->in.data, client->connection->in.data_size);
        ne_doip_net_unpack(link_data);
        free(link_data->data);
        free(link_data);
    }


    return 0;
}

static int
ne_doip_add_ipc_socket(ne_doip_server_t *server, const char *name)
{
    if (server->ipc_socket != NULL) {
        NE_DOIP_PRINT("ne_doip_add_ipc_socket ipc_socket is null!\n");
        return -1;
    }

    ne_doip_ipc_socket_t *s;
    socklen_t size;
    int name_size;

    s = malloc(sizeof *s);
    memset(s, 0, sizeof(ne_doip_ipc_socket_t));

    errno = 0;
    s->fd = ne_doip_os_socket(AF_UNIX, SOCK_STREAM, 0);
    if (s->fd < 0) {
        NE_DOIP_PRINT("ne_doip_add_ipc_socket fd error code:%d, message:%s\n", errno, strerror(errno));
        free(s);
        return -1;
    }

    memset(&s->addr, 0, sizeof s->addr);
    s->addr.sun_family = AF_UNIX;
    name_size = snprintf(s->addr.sun_path, sizeof s->addr.sun_path,
                 "%s/%s", NE_DOIP_IPC_PATH, name) + 1;

    assert(name_size > 0);
    if (name_size > (int)sizeof s->addr.sun_path) {
        NE_DOIP_PRINT("ne_doip_add_ipc_socket name_size error!\n");
        close(s->fd);
        free(s);
        /* to prevent programs reporting
         * "failed to add socket: Success" */
        errno = ENAMETOOLONG;
        return -1;
    };

    errno = 0;
    size = offsetof (struct sockaddr_un, sun_path) + name_size;
    if (bind(s->fd, (struct sockaddr *)&(s->addr), size) < 0) {
        NE_DOIP_PRINT("ne_doip_add_ipc_socket bind error code:%d, message:%s\n", errno, strerror(errno));
        unlink(s->addr.sun_path);
        close(s->fd);
        free(s);
        return -1;
    }

    if (listen(s->fd, 1) < 0) {
        NE_DOIP_PRINT("ne_doip_add_ipc_socket listen error!\n");
        unlink(s->addr.sun_path);
        close(s->fd);
        free(s);
        return -1;
    }

    s->source = ne_doip_event_source_create(server->sel_ipc, s->fd,
                                          ne_doip_ipc_socket_data, server);
    if (s->source == NULL) {
        NE_DOIP_PRINT("ne_doip_add_ipc_socket source error!\n");
        unlink(s->addr.sun_path);
        close(s->fd);
        free(s);
        return -1;
    }

    int res = ne_doip_source_add(server->sel_ipc, s->source, NE_DOIP_EVENT_READABLE);
    if (res < 0) {
        NE_DOIP_PRINT("ne_doip_add_ipc_socket source add error!\n");
        free(s->source);
        unlink(s->addr.sun_path);
        close(s->fd);
        free(s);
        return -1;
    }
    server->ipc_socket = s;

    return 0;
}

static int
ne_doip_release_ipc_socket(ne_doip_server_t *server)
{
    ne_doip_ipc_socket_t *s;
    s = server->ipc_socket;

    if (s == NULL) {
        return -1;
    }

    ne_doip_event_source_remove(s->source);
    free(s->source);
    s->source = NULL;

    // fd has been closed during source remove
    s->fd = -1;

    unlink(s->addr.sun_path);

    free(s);
    server->ipc_socket = NULL;
    return 0;
}

static int
ne_doip_add_ipv4_tcp_socket(ne_doip_server_t *server)
{
    if (server->ipv4_tcp_socket != NULL) {
        NE_DOIP_PRINT("ne_doip_add_ipv4_tcp_socket ipv4_tcp_socket null!\n");
        return -1;
    }

    ne_doip_ipv4_socket_t *s;
    socklen_t size;

    s = malloc(sizeof *s);
    memset(s, 0, sizeof(ne_doip_ipv4_socket_t));

    s->fd = ne_doip_os_socket(AF_INET, SOCK_STREAM, 0);
    if (s->fd < 0) {
        NE_DOIP_PRINT("ne_doip_add_ipv4_tcp_socket fd error!\n");
        free(s);
        return -1;
    }

    s->addr.sin_family = AF_INET;
    s->addr.sin_port = htons(NE_DOIP_TCP_DATA_PORT);
    s->addr.sin_addr.s_addr = htonl(INADDR_ANY);

    size = sizeof s->addr;

    const int opt = 1;
    if (setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt)) {
        close(s->fd);
        free(s);
        NE_DOIP_PRINT("ne_doip_add_ipv4_tcp_socket setsockopt error!\n");
        return -1;
    }

    if (bind(s->fd, (struct sockaddr *) &s->addr, size) < 0) {
        close(s->fd);
        free(s);
        NE_DOIP_PRINT("ne_doip_add_ipv4_tcp_socket bind error!\n");
        return -1;
    }

    if (listen(s->fd, 1) < 0) {
        close(s->fd);
        free(s);
        NE_DOIP_PRINT("ne_doip_add_ipv4_tcp_socket listen error!\n");
        return -1;
    }

    s->source = ne_doip_event_source_create(server->sel_net, s->fd,
                    ne_doip_tcp_socket_data, server);
    if (s->source == NULL) {
        NE_DOIP_PRINT("ne_doip_add_ipv4_tcp_socket source error!\n");
        close(s->fd);
        free(s);
        return -1;
    }

    int res = ne_doip_source_add(server->sel_net, s->source, NE_DOIP_EVENT_READABLE);
    if (res < 0) {
        NE_DOIP_PRINT("ne_doip_add_ipv4_tcp_socket source add error!\n");
        free(s->source);
        close(s->fd);
        free(s);
        return -1;
    }
    server->ipv4_tcp_socket = s;

    return 0;
}

static int
ne_doip_add_ipv6_tcp_socket(ne_doip_server_t *server)
{
    if (server->ipv6_tcp_socket != NULL) {
        NE_DOIP_PRINT("add_ipv6_tcp_socket ipv6_tcp_socket null!\n");
        return -1;
    }

    ne_doip_ipv6_socket_t *s;
    socklen_t size;

    s = malloc(sizeof *s);
    memset(s, 0, sizeof(ne_doip_ipv6_socket_t));

    s->fd = ne_doip_os_socket(AF_INET6, SOCK_STREAM, 0);
    if (s->fd < 0) {
        NE_DOIP_PRINT("add_ipv6_tcp_socket fd error!\n");
        free(s);
        return -1;
    }

    memset(&(s->addr), 0, sizeof s->addr);
    s->addr.sin6_family = AF_INET6;
    s->addr.sin6_port = htons(NE_DOIP_TCP_DATA_PORT);
    s->addr.sin6_addr = in6addr_any;

    size = sizeof(struct sockaddr_in6);

    const int opt = 1;
    if (setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt)) {
        close(s->fd);
        free(s);
        NE_DOIP_PRINT("add_ipv6_tcp_socket setsockopt1 error!\n");
        return -1;
    }

    if (setsockopt(s->fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof opt)) {
        close(s->fd);
        free(s);
        NE_DOIP_PRINT("add_ipv6_tcp_socket setsockopt2 error!\n");
        return -1;
    }

    if (bind(s->fd, (struct sockaddr *) &(s->addr), size) < 0) {
        close(s->fd);
        free(s);
        NE_DOIP_PRINT("add_ipv6_tcp_socket bind error!\n");
        return -1;
    }

    if (listen(s->fd, 5) < 0) {
        close(s->fd);
        free(s);
        NE_DOIP_PRINT("add_ipv6_tcp_socket listen error!\n");
        return -1;
    }

    s->source = ne_doip_event_source_create(server->sel_net, s->fd,
                    ne_doip_tcp6_socket_data, server);
    if (s->source == NULL) {
        NE_DOIP_PRINT("add_ipv6_tcp_socket source error!\n");
        close(s->fd);
        free(s);
        return -1;
    }

    int res = ne_doip_source_add(server->sel_net, s->source, NE_DOIP_EVENT_READABLE);
    if (res < 0) {
        NE_DOIP_PRINT("add_ipv6_tcp_socket source add error!\n");
        free(s->source);
        close(s->fd);
        free(s);
        return -1;
    }
    server->ipv6_tcp_socket = s;

    return 0;
}

static int
ne_doip_release_ipv4_tcp_socket(ne_doip_server_t *server)
{
    ne_doip_ipv4_socket_t *s;
    s = server->ipv4_tcp_socket;
    if (s == NULL) {
        return -1;
    }
    ne_doip_event_source_remove(s->source);
    free(s->source);
    s->source = NULL;

    // fd has been closed during source remove
    s->fd = -1;

    free(s);
    server->ipv4_tcp_socket = NULL;

    return 0;
}

int
ne_doip_release_ipv6_tcp_socket(ne_doip_server_t *server)
{
    ne_doip_ipv6_socket_t *s;
    s = server->ipv6_tcp_socket;
    if (s == NULL) {
        return -1;
    }
    ne_doip_event_source_remove(s->source);
    free(s->source);
    s->source = NULL;

    // fd has been closed during source remove
    s->fd = -1;

    free(s);
    server->ipv6_tcp_socket = NULL;

    return 0;
}

static int
ne_doip_add_ipv4_udp_socket(ne_doip_server_t *server)
{
    if (server->ipv4_udp_socket != NULL) {
        // unique ipv4 udp socket
        return -1;
    }

    ne_doip_ipv4_socket_t *s;
    socklen_t size;

    s = malloc(sizeof *s);
    memset(s, 0, sizeof(ne_doip_ipv4_socket_t));

    s->fd = ne_doip_os_socket(AF_INET, SOCK_DGRAM, 0);
    if (s->fd < 0) {
        free(s);
        return -1;
    }

    s->addr.sin_family = AF_INET;
    s->addr.sin_port = htons(NE_DOIP_UDP_DISCOVERY_PORT);
    s->addr.sin_addr.s_addr = htonl(INADDR_ANY);

    size = sizeof s->addr;

    errno = 0;
    const int opt = 1;
    if (setsockopt(s->fd, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &opt, sizeof opt)) {
        close(s->fd);
        free(s);
        NE_DOIP_PRINT("ne_doip_add_ipv4_udp_socket setsockopt error!\n");
        return -1;
    }

    errno = 0;
    if (bind(s->fd, (struct sockaddr *) &s->addr, size) < 0) {
        close(s->fd);
        free(s);
        printf("add_ipv6_udp_socket fail code:%d, message:%s!\n", errno, strerror(errno));
        return -1;
    }

    s->source = ne_doip_event_source_create(server->sel_net, s->fd,
                                          ne_doip_udp_socket_data, server);
    if (s->source == NULL) {
        NE_DOIP_PRINT("add_ipv4_udp_socket source error!\n");
        close(s->fd);
        free(s);
        return -1;
    }

    int res = ne_doip_source_add(server->sel_net, s->source, NE_DOIP_EVENT_READABLE);
    if (res < 0) {
        NE_DOIP_PRINT("add_ipv4_udp_socket source add error!\n");
        free(s->source);
        close(s->fd);
        free(s);
        return -1;
    }
    server->ipv4_udp_socket = s;

    ne_doip_connection_t *conn;
    conn = malloc(sizeof *conn);
    memset(conn, 0, sizeof *conn);

    conn->fd = s->fd;
    ne_doip_client_t *client;
    client = malloc(sizeof *client);
    memset(client, 0, sizeof *client);

    client->server = server;
    client->connection = conn;

    server->udp_client = client;

    return 0;
}

static int
ne_doip_add_ipv6_udp_socket(ne_doip_server_t *server)
{
    if (server->ipv6_udp_socket != NULL) {
        // unique ipv6 udp socket
        return -1;
    }

    ne_doip_ipv6_socket_t *s;
    socklen_t size;

    s = malloc(sizeof *s);
    memset(s, 0, sizeof *s);
    s->fd = ne_doip_os_socket(AF_INET6, SOCK_DGRAM, 0);
    if (s->fd < 0) {
        free(s);
        return -1;
    }

    memset(&(s->addr), 0, sizeof s->addr);
    s->addr.sin6_family = AF_INET6;
    s->addr.sin6_port = htons(NE_DOIP_UDP_DISCOVERY_PORT);
    s->addr.sin6_addr = in6addr_any;
    s->addr.sin6_scope_id = if_nametoindex(server->config->ifname);

    size = sizeof s->addr;

    const int opt = 1;
    if (setsockopt(s->fd, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &opt, sizeof opt)) {
        close(s->fd);
        free(s);
        NE_DOIP_PRINT("add_ipv6_udp_socket setsockopt fail code:%d, message:%s!\n", errno, strerror(errno));
        return -1;
    }

    if (bind(s->fd, (struct sockaddr *) &s->addr, size) < 0) {
        close(s->fd);
        free(s);
        NE_DOIP_PRINT("add_ipv6_udp_socket fail code:%d, message:%s!\n", errno, strerror(errno));
        return -1;
    }

    s->source = ne_doip_event_source_create(server->sel_net, s->fd,
                    ne_doip_udp6_socket_data, server);
    if (s->source == NULL) {
        NE_DOIP_PRINT("add_ipv6_udp_socket source error!\n");
        close(s->fd);
        free(s);
        return -1;
    }

    int res = ne_doip_source_add(server->sel_net, s->source, NE_DOIP_EVENT_READABLE);
    if (res < 0) {
        NE_DOIP_PRINT("add_ipv6_udp_socket source add error!\n");
        free(s->source);
        close(s->fd);
        free(s);
        return -1;
    }
    server->ipv6_udp_socket = s;

    ne_doip_connection_t *conn;
    conn = malloc(sizeof *conn);
    memset(conn, 0, sizeof *conn);
    
    conn->fd = s->fd;
    ne_doip_client_t *client;
    client = malloc(sizeof *client);
    memset(client, 0, sizeof *client);

    client->server = server;
    client->connection = conn;

    server->udp6_client = client;

    return 0;
}

static int
ne_doip_release_ipv4_udp_socket(ne_doip_server_t *server)
{
    ne_doip_ipv4_socket_t *s;
    s = server->ipv4_udp_socket;
    if (s == NULL) {
        return -1;
    }
    ne_doip_event_source_remove(s->source);
    free(s->source);
    s->source = NULL;

    // fd has been closed during source remove
    s->fd = -1;

    free(s);

    server->ipv4_udp_socket = NULL;

    ne_doip_client_t *client = server->udp_client;
    free(client->connection);
    free(client);
    server->udp_client = NULL;

    return 0;
}

int
ne_doip_release_ipv6_udp_socket(ne_doip_server_t *server)
{
    ne_doip_ipv6_socket_t *s;
    s = server->ipv6_udp_socket;

    if (s == NULL) {
        return -1;
    }

    ne_doip_event_source_remove(s->source);
    free(s->source);
    s->source = NULL;

    // fd has been closed during source remove
    s->fd = -1;

    free(s);

    server->ipv6_udp_socket = NULL;

    ne_doip_client_t *client = server->udp6_client;
    free(client->connection);
    free(client);
    server->udp6_client = NULL;

    return 0;
}

static void*
ne_doip_ipc_run(ne_doip_thread_t *thread, void *data)
{
    NE_DOIP_PRINT("ne_doip_ipc_run enter\n");
    ne_doip_server_t *server = (ne_doip_server_t *) data;

    while (server->ipc_run) {
        ne_doip_event_loop_dispatch(server->sel_ipc, -1, NE_DOIP_TYPE_IPC);
        ne_doip_free_client(server, NE_DOIP_CLIENT_TYPE_IPC);
    }

    NE_DOIP_PRINT("ne_doip_ipc_run exit\n");
    return NULL;
}

static void*
ne_doip_net_run(ne_doip_thread_t *thread, void *data)
{
    NE_DOIP_PRINT("ne_doip_net_run enter\n");
    ne_doip_server_t *server = (ne_doip_server_t *) data;

    while (server->net_run) {
        ne_doip_event_loop_dispatch(server->sel_net, -1, NE_DOIP_TYPE_NET);
        ne_doip_free_client(server, NE_DOIP_CLIENT_TYPE_NET);
    }

    NE_DOIP_PRINT("ne_doip_net_run exit\n");
    return NULL;
}

static void
ne_doip_server_run(ne_doip_server_t *server)
{
    global_server->ipc_run = 1;
    global_server->net_run = 1;

    global_server->epoll_ipc_thread = ne_doip_thread_create(ne_doip_ipc_run, server, NE_DOIP_EPOLL_IPC_THREAD);
    global_server->epoll_net_thread = ne_doip_thread_create(ne_doip_net_run, server, NE_DOIP_EPOLL_NET_THREAD);
}

void
ne_doip_udp_send(ne_doip_connection_t *conn, char *ip, uint16_t port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
    ne_doip_connection_sendto(conn, (struct sockaddr*)&addr, sizeof addr);
}

void
ne_doip_udp6_send(ne_doip_connection_t *conn, char *ip, uint16_t port)
{
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof addr);
    addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, ip, &addr.sin6_addr);
    addr.sin6_port = htons(port);
    ne_doip_connection_sendto(conn, (struct sockaddr*)&addr, sizeof addr);
}

int
ne_doip_test_tcp_create(ne_doip_server_t *server, char *ip, uint16_t port)
{
    NE_DOIP_PRINT("ne_doip_test_tcp_create enter! ip is [%s], port is [%d]\n", ip, port);

    if (server == NULL) {
        NE_DOIP_PRINT("ne_doip_test_tcp_create server == NULL\n");
        return -1;
    }

    if (ip == NULL) {
        NE_DOIP_PRINT("ne_doip_test_tcp_create ip == NULL\n");
        return -1;
    }

    int fd;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);

    fd = ne_doip_os_socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    errno = 0;
    const int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt)) {
        close(fd);
        fd = -1;
        NE_DOIP_PRINT("setsockopt error code:%d, message:%s\n", errno, strerror(errno));
        return fd;
    }


    int ret = ne_doip_os_connect(fd, (struct sockaddr *) &addr, sizeof addr);
    if (ret < 0) {
        NE_DOIP_PRINT("test_tcp_create connect error code:%d, message:%s\n", errno, strerror(errno));
        close(fd);
        fd = -1;
        return fd;
    }

    ne_doip_client_t *client;
    client = ne_doip_client_create(server, server->sel_net, fd);
    if (!client) {
        close(fd);
        fd = -1;
        return fd;
    }

    client->client_type = NE_DOIP_CLIENT_TYPE_TEST;

    ne_doip_sync_start(server->test_list_sync);
    ne_doip_list_insert(server->test_client_list->prev, (ne_doip_list_t *) client);
    ne_doip_sync_end(server->test_list_sync);
    // client must create completely first
    // source adding into I/O event notification must be last step!
    if (ne_doip_source_add(server->sel_net, client->source, NE_DOIP_EVENT_READABLE) < 0) {
        NE_DOIP_PRINT("doip_test_tcp_create source add error!\n");
        ne_doip_list_remove((ne_doip_list_t *)client);
        free(client->connection);
        free(client->source);
        free(client);
        close(fd);
        return -1;
    }

    return fd;
}

int
ne_doip_test_tcp6_create(ne_doip_server_t *server, char *ip, uint16_t port)
{
    NE_DOIP_PRINT("ne_doip_test_tcp6_create enter! ip is [%s], port is [%d]\n", ip, port);

    if (server == NULL) {
        NE_DOIP_PRINT("test_tcp6_create server == NULL\n");
        return -1;
    }

    if (ip == NULL) {
        NE_DOIP_PRINT("test_tcp6_create ip == NULL\n");
        return -1;
    }

    int fd;
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof addr);

    fd = ne_doip_os_socket(AF_INET6, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    inet_pton(AF_INET6, ip, &(addr.sin6_addr));
    addr.sin6_scope_id = if_nametoindex(server->config->ifname);

    int size = sizeof(struct sockaddr_in6);

    errno = 0;
    const int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt)) {
        close(fd);
        fd = -1;
        NE_DOIP_PRINT("setsockopt1 error code:%d, message:%s\n", errno, strerror(errno));
        return fd;
    }

    errno = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof opt)) {
        close(fd);
        fd = -1;
        NE_DOIP_PRINT("setsockopt2 error code:%d, message:%s\n", errno, strerror(errno));
        return fd;
    }

    NE_DOIP_PRINT("test_tcp6_create connect begin\n");
    errno = 0;
    int ret = ne_doip_os_connect(fd, (struct sockaddr *) &addr, size);
    if (ret < 0) {
        NE_DOIP_PRINT("test_tcp_create connect error code:%d, message:%s\n", errno, strerror(errno));
        close(fd);
        fd = -1;
        return fd;
    }

    ne_doip_client_t *client;
    client = ne_doip_client_create(server, server->sel_net, fd);
    if (!client) {
        close(fd);
        fd = -1;
        NE_DOIP_PRINT("setsockopt error code:%d, message:%s\n", errno, strerror(errno));
        return fd;
    }

    client->client_type = NE_DOIP_CLIENT_TYPE_TEST;

    ne_doip_sync_start(server->test_list_sync);
    ne_doip_list_insert(server->test_client_list->prev, (ne_doip_list_t *) client);
    ne_doip_sync_end(server->test_list_sync);
    // client must create completely first
    // source adding into I/O event notification must be last step!
    if (ne_doip_source_add(server->sel_net, client->source, NE_DOIP_EVENT_READABLE) < 0) {
        NE_DOIP_PRINT("test_tcp6_create source add error!\n");
        ne_doip_list_remove((ne_doip_list_t *)client);
        free(client->connection);
        free(client->source);
        free(client);
        close(fd);
        return -1;
    }
    return fd;
}

// disconnect tcp link
int
ne_doip_server_disconnect(ne_doip_server_t *server, int socket_type, int fd)
{
    if (server == NULL) {
        return -1;
    }

    ne_doip_sync_t* doip_sync = NULL;
    ne_doip_list_t* doip_list = NULL;
    if (NE_DOIP_SOCKET_TYPE_TCP == socket_type) {
        doip_sync = server->net_list_sync;
        doip_list = server->net_client_list;

    }
    else if (NE_DOIP_SOCKET_TYPE_TEST == socket_type) {
        doip_sync = server->test_list_sync;
        doip_list = server->test_client_list;
    }
    else {
        return -1;
    }

    ne_doip_sync_start(doip_sync);

    ne_doip_client_t *client;
    ne_doip_client_t *tmp;
    ne_doip_list_for_each_safe(client, tmp, doip_list, base) {
        int fd_in = ne_doip_event_source_get_fd(client->source);
        if (fd_in == fd) {
            ne_doip_event_source_remove(client->source);

            ne_doip_list_t *list_node = (ne_doip_list_t *)client;
            ne_doip_list_remove(list_node);

            ne_doip_sync_start(client->server->net_list_free_sync);
            ne_doip_list_init(list_node);
            ne_doip_list_insert(client->server->client_list_free, list_node);
            ne_doip_sync_end(client->server->net_list_free_sync);

            ne_doip_sync_end(doip_sync);

            ne_doip_remove_connection_table(fd_in, socket_type);

            return 0;
        }
    }

    ne_doip_sync_end(doip_sync);

    return -1;
}

// disconnect all tcp link
int
ne_doip_server_disconnect_all(ne_doip_server_t *server, int socket_type)
{
    NE_DOIP_PRINT("ne_doip_server_disconnect_all is enter...\n");
    if (server == NULL) {
        return -1;
    }

    ne_doip_sync_t* doip_sync = NULL;
    ne_doip_list_t* doip_list = NULL;
    if (NE_DOIP_SOCKET_TYPE_TCP == socket_type) {
        doip_sync = server->net_list_sync;
        doip_list = server->net_client_list;

    }
    else if (NE_DOIP_SOCKET_TYPE_TEST == socket_type) {
        doip_sync = server->test_list_sync;
        doip_list = server->test_client_list;
    }
    else {
        return -1;
    }

    ne_doip_sync_start(doip_sync);

    ne_doip_client_t *client;
    ne_doip_client_t *tmp;
    ne_doip_list_for_each_safe(client, tmp, doip_list, base) {
        int fd = ne_doip_event_source_get_fd(client->source);
        ne_doip_event_source_remove(client->source);

        ne_doip_list_t *list_node = (ne_doip_list_t *)client;
        ne_doip_list_remove(list_node);

        ne_doip_sync_start(client->server->net_list_free_sync);
        ne_doip_list_init(list_node);
        ne_doip_list_insert(client->server->client_list_free, list_node);
        ne_doip_sync_end(client->server->net_list_free_sync);

        ne_doip_sync_end(doip_sync);

        ne_doip_remove_connection_table(fd, socket_type);

        return 0;
    }

    ne_doip_sync_end(doip_sync);

    return -1;
}

// ********************************************************************
// external interface
// ********************************************************************

DOIP_EXPORT
void ne_doip_server_init(const char* config_path)
{
    NE_DOIP_PRINT("ne_doip_server_init enter!\n");
    if (global_server != NULL) {
        return;
    }

    global_server = ne_doip_server_create();
    if (global_server == NULL) {
        NE_DOIP_PRINT("global_server is  NULL!\n");
        return;
    }

    global_server->config = ne_doip_load_config(config_path);
    if (NULL == global_server->config) {
        NE_DOIP_PRINT("global_server config is NULL! pelease input config file! \n");
    }
    else {
        if (global_server->config->egw_control == NE_DOIP_TRUE) {
            NE_DOIP_PRINT("[plan_2] the nodes in the domain are not visible and are routed by the edge gateway. \n");
        }
        else {
            NE_DOIP_PRINT("[plan_1] the nodes in the domain are visible. \n");
            
        }
    }
    global_server->vin_gid_sync_flag = NE_DOIP_VIN_GID_NOT_SYNCHRONIZED;
    global_server->power_mode = NE_DOIP_POWER_MODE_NOTREADY;

    ne_doip_server_manager_create(global_server);

    int ret = ne_doip_add_ipc_socket(global_server, NE_DOIP_SERVER_NAME);
    if (ret < 0) {
        NE_DOIP_PRINT("ne_doip_add_ipc_socket failed!\n");
    }

    ne_doip_server_run(global_server);

}

DOIP_EXPORT
void ne_doip_server_deinit()
{
    NE_DOIP_PRINT("ne_doip_server_deinit enter!\n");

    // stop thread
    ne_doip_server_stop(global_server);
    ne_doip_release_ipc_socket(global_server);

    if (NULL == global_server->config || NE_DOIP_NET_TYPE_IPV4 == global_server->config->net_type) {
        ne_doip_release_ipv4_tcp_socket(global_server);
        ne_doip_release_ipv4_udp_socket(global_server);
    }
    else if (NE_DOIP_NET_TYPE_IPV6 == global_server->config->net_type) {
        ne_doip_release_ipv6_tcp_socket(global_server);
        ne_doip_release_ipv6_udp_socket(global_server);
    }
    ne_doip_server_destroy(global_server);

    NE_DOIP_PRINT("ne_doip_server_deinit exit!\n");
}

DOIP_EXPORT
void ne_doip_set_vin_info(const char *vin)
{
    NE_DOIP_PRINT("ne_doip_set_vin_info is enter. vin is %s\n", vin);
    if (NULL == vin || strlen(vin) != NE_DOIP_VIN_SIZE) {
        return;
    }

    if (global_server != NULL && global_server->config != NULL) {
        strncpy(global_server->config->vin, vin, NE_DOIP_VIN_SIZE);
        global_server->vin_gid_sync_flag = NE_DOIP_VIN_GID_SYNCHRONIZED;
    }
}

DOIP_EXPORT
void ne_doip_set_eid_info(const char *eid)
{
    NE_DOIP_PRINT("ne_doip_set_eid_info is enter. eid is %s\n", eid);
    if (NULL == eid || strlen(eid) != NE_DOIP_EID_SIZE * 2) {
        return;
    }

    if (global_server != NULL && global_server->config != NULL) {
        ne_doip_convert_mac(global_server->config->eid, eid);
    }
}

DOIP_EXPORT
void ne_doip_set_gid_info(const char *gid)
{
    NE_DOIP_PRINT("ne_doip_set_gid_info is enter. gid is %s\n", gid);
    if (NULL == gid || strlen(gid) != NE_DOIP_GID_SIZE * 2) {
        return;
    }

    if (global_server != NULL && global_server->config != NULL) {
        ne_doip_convert_mac(global_server->config->gid, gid);
    }
}

DOIP_EXPORT
void ne_doip_activation_line_switch_active()
{
    if (global_server != NULL && global_server->config != NULL) {
        if (NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server->config->entity_type) {
            NE_DOIP_PRINT("ne_doip_activation_line_switch_active enter!\n");
            global_server->config->activation_line_flag = NE_DOIP_TRUE;
        }
    }
}

DOIP_EXPORT
void ne_doip_activation_line_switch_deactive()
{
    if (NULL == global_server || NULL == global_server->config) {
        return;
    }
    if (NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server->config->entity_type) {
        NE_DOIP_PRINT("ne_doip_activation_line_switch_deactive enter!\n");
        global_server->config->activation_line_flag = NE_DOIP_FALSE;
        ne_doip_server_disconnect_all(global_server, NE_DOIP_SOCKET_TYPE_TCP);
        if (NE_DOIP_FALSE == global_server->config->egw_control) {
            ne_doip_broadcast_activation_line_deactive();
        }
    }
}

DOIP_EXPORT
void ne_doip_request_ip_addr_assignment(const char* if_name)
{
    NE_DOIP_PRINT("ne_doip_request_ip_addr_assignment enter!\n");
    if (NULL == global_server || NULL == global_server->config) {
        return;
    }

    int ret = -1;
    if (global_server->config->net_type != NE_DOIP_NET_TYPE_IPV6) {
        if (NULL == global_server->ipv4_tcp_socket) {
            ret = ne_doip_add_ipv4_tcp_socket(global_server);
            if (ret < 0) {
                NE_DOIP_PRINT("ne_doip_add_ipv4_tcp_socket failed!\n");
            }
        }
        else {
            NE_DOIP_PRINT("tcp socket already inited!\n");
        }

        if (NULL == global_server->ipv4_udp_socket) {
            ret = ne_doip_add_ipv4_udp_socket(global_server);
            if (ret < 0) {
                NE_DOIP_PRINT("ne_doip_add_ipv4_udp_socket failed!\n");
            }
        }
        else {
            NE_DOIP_PRINT("udp socket already inited!\n");
        }
    }
    else {
        if (NULL == global_server->ipv6_tcp_socket) {
            ret = ne_doip_add_ipv6_tcp_socket(global_server);
            if (ret < 0) {
                NE_DOIP_PRINT("ne_doip_add_ipv6_tcp_socket failed!\n");
            }
        }
        else {
            NE_DOIP_PRINT("tcp socket already inited!\n");
        }

        if (NULL == global_server->ipv6_udp_socket) {
            ret = ne_doip_add_ipv6_udp_socket(global_server);
            if (ret < 0) {
                NE_DOIP_PRINT("ne_doip_add_ipv6_udp_socket failed!\n");
            }
        }
    }

    memcpy(global_server->config->ifname, if_name, strlen(if_name));

    if (NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY == global_server->config->entity_type
        || NE_DOIP_FALSE == global_server->config->egw_control) {
        ne_doip_vehicle_announce(if_name);
    }
}

DOIP_EXPORT
void ne_doip_release_ip_addr_assignment(const char* if_name)
{
    NE_DOIP_PRINT("ne_doip_release_ip_addr_assignment enter!\n");
    if (NULL == global_server || NULL == global_server->config) {
        return;
    }
    ne_doip_vehicle_announce_state_reset();
    ne_doip_server_disconnect_all(global_server, NE_DOIP_SOCKET_TYPE_TCP);
}

DOIP_EXPORT
void ne_doip_powermode_status_change(unsigned char power_mode_status)
{
    if (global_server != NULL && global_server->config != NULL) {
        if (power_mode_status > NE_DOIP_POWER_MODE_NOTSUPPORTED) {
            NE_DOIP_PRINT("invalid power mode status!\n");
            return;
        }
        global_server->power_mode = power_mode_status;
    }
}
/* EOF */