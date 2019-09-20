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
 * @file ne_doip_server_manager.h
 * @brief Protocol parsing module
 */

#ifndef NE_DOIP_SERVER_MANAGER_H
#define NE_DOIP_SERVER_MANAGER_H

#include "ne_doip_def.h"
#include "ne_doip_timer.h"
#include "ne_doip_data_queue.h"
#include "ne_doip_threadpool.h"


#define NE_DOIP_NO_FURTHER_ACTION_REQUIRED  0x00
#define NE_DOIP_FURTHER_ACTION_REQUIRED     0x10

#define NE_DOIP_VIN_GID_SYNCHRONIZED        0x00
#define NE_DOIP_VIN_GID_NOT_SYNCHRONIZED    0x10

#define NE_DOIP_PROTOCOL_VERSION_LENGTH 1
#define NE_DOIP_INVERSE_PROTOCOL_VERSION_LENGTH 1
#define NE_DOIP_PAYLOAD_TYPE_LENGTH 2
#define NE_DOIP_PAYLOAD_LENGTH_LENGTH 4

#define NE_DOIP_ACTIVATION_TYPE_LENGTH 1
#define NE_DOIP_S_RESERVED_LENGTH 4
#define NE_DOIP_HEADER_COMMON_LENGTH 8
#define NE_DOIP_INTERNAL_HEADER_LENGTH 5


#define NE_DOIP_ROUTING_ACTIVATION_RES_LENGTH        NE_DOIP_HEADER_COMMON_LENGTH + \
                                                     NE_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                                     NE_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                                     NE_DOIP_RA_RES_CODE_LENGTH +\
                                                     NE_DOIP_S_RESERVED_LENGTH
#define NE_DOIP_ENTITY_STATUS_RES_LENGTH             NE_DOIP_HEADER_COMMON_LENGTH + \
                                                     NE_DOIP_NODE_TYPE_LENGTH + \
                                                     NE_DOIP_MCTS_LENGTH + \
                                                     NE_DOIP_NCTS_LENGTH + \
                                                     NE_DOIP_MDS_LENGTH
#define NE_DOIP_HEADER_NEGATIVE_ACK_LENGTH           NE_DOIP_HEADER_COMMON_LENGTH + \
                                                     NE_DOIP_NACK_CODE_LENGTH
#define NE_DOIP_POWERMODE_INFO_RES_LENGTH            NE_DOIP_HEADER_COMMON_LENGTH + \
                                                     NE_DOIP_POWERMODE_LENGTH
#define NE_DOIP_DIAG_POSITIVE_ACK_LENGTH             NE_DOIP_HEADER_COMMON_LENGTH + \
                                                     NE_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                                     NE_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                                     NE_DOIP_ACK_CODE_LENGTH
#define NE_DOIP_DIAG_NEGATIVE_ACK_LENGTH             NE_DOIP_HEADER_COMMON_LENGTH + \
                                                     NE_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                                     NE_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                                     NE_DOIP_NACK_CODE_LENGTH
#define NE_DOIP_ROUTING_ACTIVATION_MAND_LENGTH       NE_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                                     NE_DOIP_ACTIVATION_TYPE_LENGTH + \
                                                     NE_DOIP_S_RESERVED_LENGTH
#define NE_DOIP_ROUTING_ACTIVATION_ALL_LENGTH        NE_DOIP_LOGICAL_ADDRESS_LENGTH + \
                                                     NE_DOIP_ACTIVATION_TYPE_LENGTH + \
                                                     NE_DOIP_S_RESERVED_LENGTH + \
                                                     NE_DOIP_S_RESERVED_LENGTH

enum NE_DOIP_POWER_MODE
{
    NE_DOIP_POWER_MODE_NOTREADY     = 0x00,
    NE_DOIP_POWER_MODE_READY        = 0x01,
    NE_DOIP_POWER_MODE_NOTSUPPORTED = 0x02
};

enum NE_DOIP_PAYLOAD_TYPE_INFO
{
    NE_DOIP_PAYLOAD_TYPE_HEADER_NEGATIVE_ACK          = 0x00,   // ---->for doip node
    NE_DOIP_PAYLOAD_TYPE_VEHICLE_ANNOUNCEMENT         = 0x01,   // ---->for doip node
    NE_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_RESPONSE    = 0x02,   // ---->for doip node
    NE_DOIP_PAYLOAD_TYPE_ROUTING_ACTIVE_RESPONSE      = 0x03,   // ---->for doip node
    NE_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_REQUEST          = 0x04,   // ---->for doip node
    NE_DOIP_PAYLOAD_TYPE_ENTITY_STATUS_RESPONSE       = 0x05,   // ---->for doip node
    NE_DOIP_PAYLOAD_TYPE_POWER_MODE_INFO_RESPONSE     = 0x06,   // ---->for doip node
    NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_POSITIVE_ACK      = 0x07,   // ---->for doip node
    NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_NEGATIVE_ACK      = 0x08,   // ---->for doip node
    NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_FROM_ENTITY       = 0x09,   // ---->for doip node
    NE_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST     = 0x10,   // ---->for internal equip
    NE_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST_EID = 0x11,   // ---->for internal equip
    NE_DOIP_PAYLOAD_TYPE_VEHICLE_IDENTIFY_REQUEST_VIN = 0x12,   // ---->for internal equip
    NE_DOIP_PAYLOAD_TYPE_ROUTING_ACTIVE_REQUEST       = 0x13,   // ---->for internal equip
    NE_DOIP_PAYLOAD_TYPE_ALIVE_CHECK_RESPONSE         = 0x14,   // ---->for internal equip
    NE_DOIP_PAYLOAD_TYPE_ENTITY_STATUS_REQUEST        = 0x15,   // ---->for internal equip
    NE_DOIP_PAYLOAD_TYPE_POWER_MODE_INFO_REQUEST      = 0x16,   // ---->for internal equip
    NE_DOIP_PAYLOAD_TYPE_DIAGNOSTIC_FROM_EQUIP        = 0x17,   // ---->for internal equip
};

typedef enum NE_DOIP_DIAG_SOURCE_TYPE
{
    NE_DOIP_DIAG_SOURCE_TYPE_INTERNAL_EQUIP     = 0x00,
    NE_DOIP_DIAG_SOURCE_TYPE_EXTERNAL_EQUIP     = 0x01
}ne_doip_diag_source_type_t;

typedef enum NE_DOIP_HEADER_NACK_CODE
{
    NE_DOIP_HEADER_NACK_INCORRECT_PATTERN_FORMAT = 0x00, // mandatory
    NE_DOIP_HEADER_NACK_UNKNOWN_PAYLOAD_TYPE     = 0x01, // mandatory
    NE_DOIP_HEADER_NACK_MESSAGE_TOO_LARGE        = 0x02, // mandatory
    NE_DOIP_HEADER_NACK_OUT_OF_MEMORY            = 0x03, // mandatory
    NE_DOIP_HEADER_NACK_INVALID_PAYLOAD_LENGTH   = 0x04, // mandatory
}ne_doip_header_nack_code_t;

typedef enum NE_DOIP_DIAGNOSTIC_NACK_CODE
{
    NE_DOIP_DIAGNOSTIC_NACK_INVALID_SA              = 0x02, // mandatory
    NE_DOIP_DIAGNOSTIC_NACK_UNKNOWN_TA              = 0x03, // mandatory
    NE_DOIP_DIAGNOSTIC_NACK_DIAG_MSG_TOO_LARGE      = 0x04, // mandatory
    NE_DOIP_DIAGNOSTIC_NACK_OUT_OF_MEMORY           = 0x05, // mandatory
    NE_DOIP_DIAGNOSTIC_NACK_TARGET_UNREACHABLE      = 0x06, // optional
    NE_DOIP_DIAGNOSTIC_NACK_UNKNOWN_NETWORK         = 0x07, // optional
    NE_DOIP_DIAGNOSTIC_NACK_TRANS_PROTO_ERROR       = 0x08, // optional
}ne_doip_diagnostic_nack_code_t;

typedef enum NE_DOIP_ROUTING_ACTIVE_RES_CODE
{
    NE_DOIP_RA_RES_UNKNOWN_SOURCE_ADDRESS              = 0x00, // mandatory
    NE_DOIP_RA_RES_ALL_SOCKET_REGISTED_AND_ACTIVE      = 0x01, // mandatory
    NE_DOIP_RA_RES_SA_DIFFERENT_ACTIVATED_SOCKET       = 0x02, // mandatory
    NE_DOIP_RA_RES_SA_REGISTED_DIFFERENT_SOCKET        = 0x03, // mandatory
    NE_DOIP_RA_RES_MISSING_AUTHENTICATION              = 0x04, // optional
    NE_DOIP_RA_RES_REJECT_CONFIRMATION                 = 0x05, // optional
    NE_DOIP_RA_RES_UNSUPPORTED_RA_TYPE                 = 0x06, // mandatory
    NE_DOIP_RA_RES_ROUTING_SUCCESSFULLY_ACTIVATED      = 0x10, // mandatory
    NE_DOIP_RA_RES_CONFIRMATION_REQUIRED               = 0x11, // optional
}ne_doip_routing_activation_res_code_t;

typedef enum NE_DOIP_DIAG_ROUTING_STEP
{
    NE_DOIP_DIAG_ROUTING_STEP_1    = 0x00,
    NE_DOIP_DIAG_ROUTING_STEP_2    = 0x01,
}ne_doip_diag_routing_step_t;

typedef struct ne_doip_node_ipc_table
{
    ne_doip_list_t base;
    int fd;
    int tcp_fd;
    uint8_t comm_type;
    uint16_t ecu_logical_address;
    uint16_t entity_logical_address;
    uint8_t confirmation_flag;
}ne_doip_node_ipc_table_t;

typedef struct ne_doip_node_tcp_table
{
    ne_doip_list_t base;
    int fd;
    int cache_fd; // Cache fd for direct transfer of data after the second packet of big data
    int routing_fd; // fd temporary storage in the route activation/alive check phase
    char* ip;
    uint16_t port;
    uint8_t comm_type;
    uint8_t connection_state;
    uint16_t entity_logical_address;
    uint16_t equip_logical_address;
    uint16_t logical_routing_address;
    unsigned char fd_regist_flag;
    unsigned char all_alive_check_flag;
    unsigned char single_alive_check_flag;
    unsigned char diag_data_fail_flag;
    unsigned char functional_addressing_flag;
    uint32_t authen_info;
    uint32_t diag_data_total_length;
    uint32_t diag_data_current_pos;
    int tcp_initial_inactivity_timeid;
    int tcp_generral_inactivity_timeid;
    int tcp_alive_check_timeid;
}ne_doip_node_tcp_table_t;

typedef struct ne_doip_node_udp_table
{
    ne_doip_list_t base;
    int fd;
    char* ip;
    uint16_t port;
    uint8_t comm_type;
    int vehicle_identify_wait_timeid;
}ne_doip_node_udp_table_t;

typedef struct ne_doip_equip_ipc_table
{
    ne_doip_list_t base;
    int fd;
    int cache_fd; // Cache fd for direct transfer of data after the second packet of big data
    int tcp_fd;
    uint8_t comm_type;
    unsigned char functional_addressing_flag;
    uint16_t entity_logical_address;
    uint16_t equip_logical_address;
    uint32_t diag_data_total_length;
    uint32_t diag_data_current_pos;
}ne_doip_equip_ipc_table_t;

typedef struct ne_doip_equip_tcp_table
{
    ne_doip_list_t base;
    int fd;
    int source_fd; // Record the source of tcp/ipc fd
    char* ip;
    uint16_t port;
    uint16_t entity_logical_address;
    uint8_t comm_type;
    ne_doip_queue_manager_t *queue_manager;
    ne_doip_diag_routing_step_t diag_routing_step;
}ne_doip_equip_tcp_table_t;

typedef struct ne_doip_equip_udp_table
{
    ne_doip_list_t base;
    int fd;
    int source_fd;
    char* ip;
    uint16_t port;
    uint16_t entity_logical_address;
    uint8_t comm_type;
    char eid[NE_DOIP_EID_SIZE];
}ne_doip_equip_udp_table_t;

typedef struct ne_doip_server_manager
{
    ne_doip_server_t *server;
    ne_doip_timer_manager_t *timer_manager;
    ne_doip_threadpool_t *threadpool;
    ne_doip_sync_t *node_ipc_list_sync;
    ne_doip_sync_t *node_tcp_list_sync;
    ne_doip_sync_t *node_udp_list_sync;
    ne_doip_list_t *node_ipc_list;
    ne_doip_list_t *node_tcp_list;
    ne_doip_list_t *node_udp_list;
    ne_doip_sync_t *equip_ipc_list_sync;
    ne_doip_sync_t *equip_tcp_list_sync;
    ne_doip_sync_t *equip_udp_list_sync;
    ne_doip_list_t *equip_ipc_list;
    ne_doip_list_t *equip_tcp_list;
    ne_doip_list_t *equip_udp_list;
    uint8_t alive_check_ncts;
}ne_doip_server_manager_t;

typedef struct ne_doip_addr_data
{
    int fd;
    char* ip;
    uint16_t port;
    uint8_t comm_type;
    uint8_t connection_state;
}ne_doip_addr_data_t;

typedef struct ne_doip_link_data
{
    int fd;
    char* ip;
    uint16_t port;
    uint8_t comm_type;
    char *data;
    uint32_t data_size;
}ne_doip_link_data_t;

typedef struct ne_doip_integrate_task
{
    ne_doip_diag_source_type_t diag_source_type;
    ne_doip_link_data_t* link_data;
    uint32_t pos;
    uint32_t payload_length;
}ne_doip_integrate_task_t;

void ne_doip_server_manager_create(ne_doip_server_t *server);
void ne_doip_server_manager_destroy();
void ne_doip_add_connection_table(ne_doip_addr_data_t *addr_data);
void ne_doip_remove_connection_table(int fd, uint8_t socket_type);
void ne_doip_pack(ne_doip_link_data_t *link_data, uint8_t type, char* data, uint32_t length);
void ne_doip_ipc_unpack(ne_doip_link_data_t *link_data);
void ne_doip_net_unpack(ne_doip_link_data_t *link_data);
void ne_doip_vehicle_announce(const char* if_name);
void ne_doip_vehicle_announce_state_reset();
void ne_doip_broadcast_activation_line_deactive();

#endif // NE_DOIP_SERVER_MANAGER_H
/* EOF */