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
 * @file ne_doip_comm_def.h
 * @brief Definition file
 */

#ifndef NE_DOIP_COMM_DEF_H
#define NE_DOIP_COMM_DEF_H

#include <stdint.h>

#define NE_DOIP_VIN_SIZE            17   ///< Vehicle identification number size
#define NE_DOIP_EID_SIZE            6    ///< Entify iedntification size
#define NE_DOIP_GID_SIZE            6    ///< Group identification size

/**
* @brief Doip result code
* The result code generated during the operation of the doip stack
*/
typedef enum NE_DOIP_RESULT
{
    NE_DOIP_RESULT_OK                 = 0x00,    ///< Success
    NE_DOIP_RESULT_HDR_ERROR          = 0x01,    ///< Header data format error 
    NE_DOIP_RESULT_TIMEOUT_A          = 0x02,    ///< Communication timeout
    NE_DOIP_RESULT_UNKNOWN_SA         = 0x03,    ///< Source address is unknown
    NE_DOIP_RESULT_INVALID_SA         = 0x04,    ///< Source address is invalid
    NE_DOIP_RESULT_UNKNOWN_TA         = 0x05,    ///< Target address is unknown
    NE_DOIP_RESULT_MESSAGE_TOO_LARGE  = 0x06,    ///< Payload size too large
    NE_DOIP_RESULT_OUT_OF_MEMORY      = 0x07,    ///< Insufficient buffer
    NE_DOIP_RESULT_TARGET_UNREACHABLE = 0x08,    ///< Target address not be reached
    NE_DOIP_RESULT_NO_LINK            = 0x09,    ///< No link
    NE_DOIP_RESULT_NO_SOCKET          = 0x0A,    ///< No socket
    NE_DOIP_RESULT_NOT_INITIALIZED    = 0x10,    ///< Call interface order error
    NE_DOIP_RESULT_PARAMETER_ERROR    = 0x11,    ///< The parameter is incorrect
    NE_DOIP_RESULT_REPEAT_REGIST      = 0x12,    ///< The node is Repeat registration
    NE_DOIP_RESULT_ERROR              = 0xFF     ///< Common error
}ne_doip_result_t;

/**
* @brief Doip node type
* When the client registers a logical address, it distinguishes its own node type.
*/
typedef enum NE_DOIP_INSTANCE_TYPE
{
    NE_DOIP_INSTANCE_TYPE_ENTITY = 0x00,    ///< Instance type is entity[gateway or doip_node]
    NE_DOIP_INSTANCE_TYPE_ECU    = 0x01,    ///< Instance type is ECU
    NE_DOIP_INSTANCE_TYPE_UNKOWN = 0xFF     ///< Instance type is unkwon
}ne_doip_instence_type_t;

/**
* @brief Target address type
* Used to distinguish between physical or functional addressing
*/
typedef enum NE_DOIP_TA_TYPE
{
    NE_DOIP_TA_TYPE_PHYSICAL       = 0x00,    ///< Physical addressing
    NE_DOIP_TA_TYPE_FUNCTIONAL     = 0x01     ///< Functional addressing
}ne_doip_ta_type_t;

/**
* @brief User confirmation result
* The routing activation phase, when the user needs to confirm, the result of the transmission
*/
typedef enum NE_DOIP_USER_CON_RESLUT
{
    NE_DOIP_USER_CON_RESLUT_REJECT       = 0x00,    ///< User reject confirmation
    NE_DOIP_USER_CON_RESLUT_NO_CON       = 0x01,    ///< User does not need to confirm
    NE_DOIP_USER_CON_RESLUT_PASS         = 0x02     ///< User passed the confirmation
}ne_doip_user_con_result_t;

/**
* @brief Diagnostic message request info
* [Doip node use] The structure used to request of [Diagnostic response]
*/
typedef struct ne_doip_diag_data_request
{
    uint16_t logical_source_address;    ///< Unique identifier doip node.
    uint16_t logical_target_address;    ///< Unique identifier test euipment.
    ne_doip_ta_type_t ta_type;          ///< Target address type.
    char* data;                         ///< UDS data
    uint32_t data_length;               ///< UDS data length
}ne_doip_diag_data_request_t;

/**
* @brief Diagnostic message confirm info
* [Doip node use] The structure used to confirm of [Diagnostic response]
*/
typedef struct ne_doip_diag_data_confirm
{
    uint16_t logical_source_address;    ///< Unique identifier doip node.
    uint16_t logical_target_address;    ///< Unique identifier test euipment.
    ne_doip_ta_type_t ta_type;          ///< Target address type.
    ne_doip_result_t result;            ///< Confirm result
}ne_doip_diag_data_confirm_t;

/**
* @brief Diagnostic message indication info
* [Doip node use] The structure used to indication of [Diagnostic request]
*/
typedef struct ne_doip_diag_data_indication
{
    uint16_t logical_source_address;    ///< Unique identifier test euipment.
    uint16_t logical_target_address;    ///< Unique identifier doip node.
    ne_doip_ta_type_t ta_type;          ///< Target address type.
    char* data;                         ///< RAW data
    uint32_t data_length;               ///< RAW data length.
    uint32_t total_payload_size;        ///< The total size of the transmitted data
    uint32_t current_payload_position;  ///< The size of the data currently being transferred
    ne_doip_result_t result;            ///< Indication result
}ne_doip_diag_data_indication_t;

/**
* @brief Vehicle identity response info
* [Internal test equipment use] Response data structure received after the vehicle discovery request
*/
typedef struct ne_doip_vehicle_identity_resinfo
{
    char vin[NE_DOIP_VIN_SIZE];    ///< VIN info
    char eid[NE_DOIP_EID_SIZE];    ///< EID info
    char gid[NE_DOIP_GID_SIZE];    ///< GID info
    uint16_t logical_address;      ///< Doip node logical address
    unsigned char further_action;  ///< A centralized security approach is used.
    unsigned char vin_gid_sync;    ///< Flag indicating whether all doip entities have been synchronized
}ne_doip_vehicle_identity_resinfo_t;

/**
* @brief Routing activation response info
* [Internal test equipment use] Response data structure received after the routing activation request
*/
typedef struct ne_doip_routing_active_resinfo
{
    uint16_t equip_logical_address;     ///< Euip logical address
    uint16_t entity_logical_address;    ///< Doip node logical address
    uint8_t routing_active_res_code;    ///< Routing activation response code
    uint32_t reserved_by_ISO13400;      ///< Reserved by ISO 13400
    uint32_t reserved_by_oem;           ///< Reserved by oem
}ne_doip_routing_active_resinfo_t;

/**
* @brief Doip entity status response info
* [Internal test equipment use] Response data structure received after the entity status request
*/
typedef struct ne_doip_entity_status_info
{
    uint8_t node_type;            ///< Doip instance type[node or gateway]
    uint8_t MCTS;                 ///< Max. concurrent TCP_ DATA sockets
    uint8_t NCTS;                 ///< Currently open TCP_ DATA sockets
    uint32_t MDS;                 ///< Max. data size
}ne_doip_entity_status_info_t;


/**
* @brief [Doip node use] Indication callback when doip module received [Diagnostic request].
*
* @param [in] diag_data_indication : This is ne_doip_diag_data_indication_t struct. See the data definition above for details.
*
* @attention The pointer is released by the doip module.
*/
typedef void (*ne_doip_diag_indication_callback) (ne_doip_diag_data_indication_t* diag_data_indication);

/**
* @brief [Doip node use] Confirm callback after sending out [Diagnostic response] reqest.
*
* @param [in] diag_data_confirm : This is ne_doip_diag_data_confirm_t struct. See the data definition above for details.
*
* @attention The pointer is released by the doip module.
*/
typedef void (*ne_doip_diag_confirm_callback) (ne_doip_diag_data_confirm_t *diag_data_confirm);

/**
* @brief [Doip node use] User-confirmed callback function during routing activation.
*
* @param [in] logical_souce_address : Unique identifier test euipment.
* @param [in] logical_target_address : Unique identifier doip node.
*
* @attention After the callback, please be sure to call the user confirmation result to the doip module.
*/
typedef void (*ne_doip_user_conf_callback) (uint16_t logical_souce_address, uint16_t logical_target_address);

/**
* @brief [Internal test equipment use] Vehicle identity response callback function.
*
* @param [in] info : This is ne_doip_vehicle_identity_resinfo_t struct. See the data definition above for details.
*
* @attention The pointer is released by the doip module.
*/
typedef void (*ne_doip_vehicle_identity_callback) (ne_doip_vehicle_identity_resinfo_t* info);

/**
* @brief [Internal test equipment use] Routing activation response callback function.
*
* @param [in] info : This is ne_doip_routing_active_resinfo_t struct. See the data definition above for details.
*
* @attention The pointer is released by the doip module.
*/
typedef void (*ne_doip_routing_active_callback) (ne_doip_routing_active_resinfo_t* info);

/**
* @brief [Internal test equipment use] Doip entity status response callback function.
*
* @param [in] info : This is ne_doip_entity_status_info_t struct. See the data definition above for details.
*
* @attention The pointer is released by the doip module.
*/
typedef void (*ne_doip_entity_status_callback) (ne_doip_entity_status_info_t* info);

/**
* @brief [Internal test equipment use] Power mode information response callback function.
*
* @param [in] power_mode : Identifies whether or not the vehicle is in diagnostic power mode and ready to perform reliable diagnostics.
*/
typedef void (*ne_doip_power_mode_callback) (uint8_t power_mode);

/**
* @brief [Internal test equipment use] Diagnostic positive ack callback function.
*
* @param [in] logical_target_address : Unique identifier doip node.
* @param [in] positive_ack_code : Contains the diagnostic message positive acknowledge code.
*/
typedef void (*ne_doip_diag_pack_callback) (uint16_t logical_target_address, uint8_t positive_ack_code);

/**
* @brief [Internal test equipment use] Diagnostic negtive ack callback function.
*
* @param [in] logical_target_address : Unique identifier doip node.
* @param [in] positive_ack_code : Contains the diagnostic message negtive acknowledge code.
*/
typedef void (*ne_doip_diag_nack_callback) (uint16_t logical_target_address, uint8_t negative_ack_code);

/**
* @brief [Internal test equipment use] Diagnostic message response callback function.
*
* @param [in] logical_target_address : Unique identifier doip node.
* @param [in] data : UDS data.
* @param [in] length : UDS data length.
*
* @attention The pointer is released by the doip module.
*/
typedef void (*ne_doip_diagnostic_callback) (uint16_t logical_target_address, char* data, uint32_t length);

/**
* @brief Related callback function registration when creating a doip node
* [Doip node use] <indcation_cb> and <confirm_cb> are mandatory, <user_conf_cb> is optional
*/
typedef struct ne_doip_node_callback_register
{
    ne_doip_diag_indication_callback indication_cb;    ///< doip diagnostic indication callback.
    ne_doip_diag_confirm_callback confirm_cb;         ///< doip diagnostic confirmation callback.
    ne_doip_user_conf_callback user_conf_cb;          ///< doip user confirmation callback.[Confirmation of routing activation phase]
}ne_doip_node_callback_register_t;

/**
* @brief Related callback function registration when creating a test equipment
* [Internal test equipment use] The client needs to register all callbacks,
*     and if it does not register the corresponding request, the response will be lost.
*/
typedef struct ne_doip_equip_callback_register
{
    ne_doip_vehicle_identity_callback vehicle_identity_cb;    ///< doip vehicle identity res callback.
    ne_doip_routing_active_callback routing_active_cb;        ///< doip routing activation res callback.
    ne_doip_entity_status_callback entity_status_cb;          ///< doip entity state res callback.
    ne_doip_power_mode_callback power_mode_cb;                ///< doip power mode res callback.
    ne_doip_diag_pack_callback diag_pack_cb;                  ///< doip diag pack callback.
    ne_doip_diag_nack_callback diag_nack_cb;                  ///< doip diag nack callback.
    ne_doip_diagnostic_callback diagnostic_cb;                ///< doip diag res callback.
}ne_doip_equip_callback_register_t;


#endif // NE_DOIP_COMM_DEF_H
/* EOF */