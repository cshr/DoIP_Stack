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
 * @file ne_doip_def.h
 * @brief Definition file
 */

#ifndef NE_DOIP_DEF_H
#define NE_DOIP_DEF_H

#include <sys/un.h>
#include <netinet/in.h>
#include <stdint.h>

#include "ne_doip_comm_def.h"
#include "ne_doip_event_loop.h"
#include "ne_doip_connection.h"
#include "ne_doip_thread.h"
#include "ne_doip_util.h"

#define NE_DOIP_DEBUG

#define NE_DOIP_TRUE 1
#define NE_DOIP_FALSE 0

#define NE_DOIP_DEFAULT_VERSION 0xFF

#define NE_DOIP_TCP_DATA_PORT       13400
#define NE_DOIP_UDP_DISCOVERY_PORT  13400
#define NE_DOIP_UDP_BROADCAST_IP    "255.255.255.255"
#define NE_DOIP_UDP6_BROADCAST_IP   "FF02::1"

#define NE_DOIP_A_DOIP_ANNOUNCE_WAIT_MAX   500 // 500 ms
#define NE_DOIP_A_DOIP_ANNOUNCE_INTERVAL   500 // 500 ms
#define NE_DOIP_T_TCP_INITIAL_INACTIVITY   2000 // 2 s
#define NE_DOIP_T_TCP_GENE_INACTIVITY      300000 // 5 min
#define NE_DOIP_T_TCP_ALIVE_CHECK          500 // 500 ms

#define NE_DOIP_IN_PAYLOADTYPE_NODE_RGIST            0x01
#define NE_DOIP_IN_PAYLOADTYPE_DIAG_REQUEST          0x02
#define NE_DOIP_IN_PAYLOADTYPE_DIAG_CONFIRM          0x03
#define NE_DOIP_IN_PAYLOADTYPE_DIAG_INDICATION       0x04
#define NE_DOIP_IN_PAYLOADTYPE_REGIST_USER_CONF      0x05
#define NE_DOIP_IN_PAYLOADTYPE_USER_CONF_RESULT      0x06

#define NE_DOIP_IN_EQUIP_RGIST                0xF0
#define NE_DOIP_IN_EQUIP_HEADER_NACK          0xF1
#define NE_DOIP_IN_EQUIP_ANN_IDEN_RES         0xF2
#define NE_DOIP_IN_EQUIP_IDENTITY_REQ         0xF3
#define NE_DOIP_IN_EQUIP_IDENTITY_REQ_EID     0xF4
#define NE_DOIP_IN_EQUIP_IDENTITY_REQ_VIN     0xF5
#define NE_DOIP_IN_EQUIP_ROUTING_ACTIVE       0xF6
#define NE_DOIP_IN_EQUIP_ALIVE_CHECK          0xF7
#define NE_DOIP_IN_EQUIP_ENTITY_STATUS        0xF8
#define NE_DOIP_IN_EQUIP_POWER_MODE           0xF9
#define NE_DOIP_IN_EQUIP_DIAGNOSTIC_PACK      0xFA
#define NE_DOIP_IN_EQUIP_DIAGNOSTIC_NACK      0xFB
#define NE_DOIP_IN_EQUIP_DIAGNOSTIC           0xFC

#define NE_DOIP_OUT_VIN_GID_SYNC              0xABAB
#define NE_DOIP_OUT_ACTIVATION_DEACTIVE       0xBCBC

#define NE_DOIP_ST_PAYLOADTYPE_VEHICLE_INDENTIFY         0x0001
#define NE_DOIP_ST_PAYLOADTYPE_VEHICLE_INDENTIFY_EID     0x0002
#define NE_DOIP_ST_PAYLOADTYPE_VEHICLE_INDENTIFY_VIN     0x0003
#define NE_DOIP_ST_PAYLOADTYPE_ROUTING_ACTIVE_REQ        0x0005
#define NE_DOIP_ST_PAYLOADTYPE_ALIVE_CHECK_RES           0x0008
#define NE_DOIP_ST_PAYLOADTYPE_ENTINTY_STATUS_REQ        0x4001
#define NE_DOIP_ST_PAYLOADTYPE_POWER_MODE_REQ            0x4003
#define NE_DOIP_ST_PAYLOADTYPE_DIAGNOSTIC_REQ            0x8001
#define NE_DOIP_ST_PAYLOADTYPE_HEADER_NEGTIVE_ACK        0x0000
#define NE_DOIP_ST_PAYLOADTYPE_ANNOUNCE_OR_IDENTIFYRES   0x0004
#define NE_DOIP_ST_PAYLOADTYPE_ROUTING_ACTIVE_RES        0x0006
#define NE_DOIP_ST_PAYLOADTYPE_ALIVE_CHECK_REQ           0x0007
#define NE_DOIP_ST_PAYLOADTYPE_ENTINTY_STATUS_RES        0x4002
#define NE_DOIP_ST_PAYLOADTYPE_POWER_MODE_RES            0x4004
#define NE_DOIP_ST_PAYLOADTYPE_DIAG_POSITIVE_ACK         0x8002
#define NE_DOIP_ST_PAYLOADTYPE_DIAG_NEGATIVE_ACK         0x8003

#define NE_DOIP_WWH_OBD_FUNCTIANAL_ADDRESS    0xE000
#define NE_DOIP_OEM_LOWER_FUNCTIANAL_ADDRESS  0xE400
#define NE_DOIP_OEM_UPPER_FUNCTIANAL_ADDRESS  0xEFFF

#define NE_DOIP_ALL_ECU_NUM           50

#define NE_DOIP_IN_COMMAND_LENGTH      1
#define NE_DOIP_IN_DATA_LENGTH         4
#define NE_DOIP_ST_COMMAND_LENGTH      2
#define NE_DOIP_LOGICAL_ADDRESS_LENGTH 2
#define NE_DOIP_INSTANCE_TYPE_LENGTH   1
#define NE_DOIP_CONF_RESULT_LENGTH     1
#define NE_DOIP_TA_TYPE_LENGTH         1
#define NE_DOIP_BLANK_2_LENGTH         2
#define NE_DOIP_FURTHER_ACTION_LENGTH  1
#define NE_DOIP_VIN_GID_SYNC_LENGTH    1
#define NE_DOIP_RA_RES_CODE_LENGTH     1
#define NE_DOIP_NODE_TYPE_LENGTH       1
#define NE_DOIP_MCTS_LENGTH            1
#define NE_DOIP_NCTS_LENGTH            1
#define NE_DOIP_MDS_LENGTH             4
#define NE_DOIP_POWERMODE_LENGTH       1
#define NE_DOIP_ACK_CODE_LENGTH        1
#define NE_DOIP_NACK_CODE_LENGTH       1
#define NE_DOIP_IFNAME_SIZE            10
#define NE_DOIP_ANNOUNCE_OR_IDENTITYRES_MAND_LENGTH  NE_DOIP_EID_SIZE + \
                                                     NE_DOIP_GID_SIZE + \
                                                     NE_DOIP_VIN_SIZE + \
                                                     NE_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                                     NE_DOIP_FURTHER_ACTION_LENGTH
#define NE_DOIP_ANNOUNCE_OR_IDENTITYRES_ALL_LENGTH   NE_DOIP_EID_SIZE + \
                                                     NE_DOIP_GID_SIZE + \
                                                     NE_DOIP_VIN_SIZE + \
                                                     NE_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                                     NE_DOIP_FURTHER_ACTION_LENGTH + \
                                                     NE_DOIP_VIN_GID_SYNC_LENGTH

enum NE_DOIP_NET_TYPE
{
    NE_DOIP_NET_TYPE_IPV4 = 0x00,
    NE_DOIP_NET_TYPE_IPV6 = 0x01
};

enum NE_DOIP_EVENT_INFO
{
    NE_DOIP_EVENT_READABLE = 0x01,
    NE_DOIP_EVENT_WRITABLE = 0x02,
    NE_DOIP_EVENT_HANGUP   = 0x04,
    NE_DOIP_EVENT_ERROR    = 0x08
};

enum NE_DOIP_SOCKET_TYPE
{
    NE_DOIP_SOCKET_TYPE_UNKNOWN   = 0x00,
    NE_DOIP_SOCKET_TYPE_IPC       = 0x01,
    NE_DOIP_SOCKET_TYPE_TCP       = 0x02,
    NE_DOIP_SOCKET_TYPE_TEST      = 0x03,
    NE_DOIP_SOCKET_TYPE_UDP_UNI   = 0x04,
    NE_DOIP_SOCKET_TYPE_UDP_MOUTI = 0x05
};

enum NE_DOIP_CONNECT_STATE
{
    NE_DOIP_CONNECT_STATE_LISTEN                      = 0x00,
    NE_DOIP_CONNECT_STATE_INITIALIZED                 = 0x01,
    NE_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_AUTH = 0x02,
    NE_DOIP_CONNECT_STATE_REGISTERED_PENDING_FOR_CONF = 0x03,
    NE_DOIP_CONNECT_STATE_REGISTERED_ROUTING_ACTIVE   = 0x04,
    NE_DOIP_CONNECT_STATE_FINALIZE                    = 0x05
};

typedef enum NE_DOIP_ANNOUNCE_STATE
{
    NE_DOIP_IP_NOT_CONFIGURED_NOT_ANNOUNCED    = 0x00,
    NE_DOIP_IP_CONFIGURED_NOT_ANNOUNCED        = 0x01,
    NE_DOIP_IP_CONFIGURED_ANNOUNCED            = 0x02
}ne_doip_announce_state_t;

typedef enum NE_DOIP_ENTITY_TYPE
{
    NE_DOIP_ENTITY_TYPE_UNKNOWN         = 0x00,
    NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY    = 0x01,
    NE_DOIP_ENTITY_TYPE_GATEWAY         = 0x02,
    NE_DOIP_ENTITY_TYPE_NODE            = 0x03
}ne_doip_entity_type_t;

// doip node information
typedef struct ne_doip_node
{
    ne_doip_list_t base;
    int fd;
    uint16_t logical_source_address;
    uint16_t logical_target_address;
    ne_doip_instence_type_t doip_instence_type;
    uint8_t ta_type;
    ne_doip_thread_t *thread;
    ne_doip_connection_t *connection;
    ne_doip_diag_indication_callback indication_callback;
    ne_doip_diag_confirm_callback confirm_callback;
    ne_doip_user_conf_callback user_conf_callback;
    uint32_t diag_data_total_length;
    uint32_t diag_data_current_pos;
}ne_doip_node_t;

// doip node list
typedef struct ne_doip_node_list
{
    ne_doip_sync_t *node_list_sync;
    ne_doip_list_t *node_list;
}ne_doip_node_list_t;

// internal test equipment information
typedef struct ne_doip_test_equip
{
    int fd;
    uint16_t logical_source_address;
    ne_doip_thread_t *thread;
    ne_doip_connection_t *connection;
    ne_doip_vehicle_identity_callback vehicle_identity_callback;
    ne_doip_routing_active_callback routing_active_callback;
    ne_doip_entity_status_callback entity_status_callback;
    ne_doip_power_mode_callback power_mode_callback;
    ne_doip_diag_pack_callback daig_pack_callback;
    ne_doip_diag_nack_callback daig_nack_callback;
    ne_doip_diagnostic_callback diagnostic_callback;
}ne_doip_test_equip_t;

// Socket for interprocess communication
typedef struct ne_doip_ipc_socket
{
    int fd;
    struct sockaddr_un addr;
    ne_doip_event_source_t *source;
}ne_doip_ipc_socket_t;

// Socket for ipv4 network communication
typedef struct ne_doip_ipv4_socket
{
    int fd;
    int type;
    struct sockaddr_in addr;
    ne_doip_event_source_t *source;
}ne_doip_ipv4_socket_t;

// Socket for ipv6 network communication
typedef struct ne_doip_ipv6_socket
{
    int fd;
    int type;
    struct sockaddr_in6 addr;
    ne_doip_event_source_t *source;
}ne_doip_ipv6_socket_t;

// IF configuration table, support priority
typedef struct ne_doip_net_source
{
    ne_doip_list_t base;
    int priority;
    char source_type[8];
    char ifname[8];
    char prefix_if[8];
    ne_doip_announce_state_t announce_state;
    uint8_t announce_count;
    int announce_wait_timeid;
    int announce_interval_timeid;
}ne_doip_net_source_t;

// Diagnostic routing table
typedef struct ne_doip_routing_table
{
    ne_doip_list_t base;
    int fd;
    char entity_type[16];
    uint16_t entity_logical_address;
    char* ip;
}ne_doip_routing_table_t;

// Functional addressing grouping table
typedef struct ne_doip_func_group
{
    ne_doip_list_t base;
    uint16_t group_address;
    uint16_t logical_address_array[NE_DOIP_ALL_ECU_NUM];
    uint8_t group_member_num;
}ne_doip_func_group_t;

// doip config information
typedef struct ne_doip_config
{
    char vin[NE_DOIP_VIN_SIZE];
    char eid[NE_DOIP_EID_SIZE];
    char gid[NE_DOIP_GID_SIZE];
    char ifname[NE_DOIP_IFNAME_SIZE];
    ne_doip_entity_type_t entity_type;
    uint8_t net_type;
    uint8_t protocol_version;
    uint8_t mcts;
    uint8_t activation_line_flag;
    uint8_t egw_control;
    uint8_t need_vin_gid_sync;
    uint16_t tester_sa;
    uint16_t functianal_la;
    uint32_t mds;
    uint32_t authen_info;
    uint32_t announce_interval_time;
    uint32_t general_inactivity_time;
    uint32_t initial_inactivity_time;
    uint32_t alive_check_time;
    ne_doip_list_t *routing_list;
    ne_doip_list_t *func_group_list;
    ne_doip_list_t *net_source_list;
}ne_doip_config_t;

// doip client information
typedef struct ne_doip_client
{
    ne_doip_list_t base;
    struct ne_doip_server *server;
    ne_doip_connection_t *connection;
    ne_doip_event_source_t *source;
    uint8_t client_type;
}ne_doip_client_t;

// doip server information
typedef struct ne_doip_server
{
    int ipc_run;
    int net_run;
    ne_doip_select_t *sel_ipc;
    ne_doip_select_t *sel_net;
    uint8_t power_mode;
    uint8_t vin_gid_sync_flag;
    ne_doip_sync_t *ipc_list_sync;
    ne_doip_sync_t *net_list_sync;
    ne_doip_sync_t *test_list_sync;
    ne_doip_sync_t *net_list_free_sync;
    ne_doip_config_t *config;
    ne_doip_client_t *udp_client;
    ne_doip_client_t *udp6_client;
    ne_doip_list_t *ipc_client_list;
    ne_doip_list_t *net_client_list;
    ne_doip_list_t *test_client_list;
    ne_doip_list_t *client_list_free;
    ne_doip_thread_t* epoll_ipc_thread;
    ne_doip_thread_t* epoll_net_thread;
    ne_doip_ipc_socket_t *ipc_socket;
    ne_doip_ipv4_socket_t *ipv4_tcp_socket;
    ne_doip_ipv4_socket_t *ipv4_udp_socket;
    ne_doip_ipv6_socket_t *ipv6_tcp_socket;
    ne_doip_ipv6_socket_t *ipv6_udp_socket;
}ne_doip_server_t;

#endif // NE_DOIP_DEF_H
/* EOF */