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
 * @file ne_doip_node.h
 * @brief DoIP node interface
 * 1.Provide interfaces to users 2.IPC communication with server 3.Server notification callback to the user
 */

#ifndef NE_DOIP_NODE_H
#define NE_DOIP_NODE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "ne_doip_comm_def.h"


/**
* @brief doip node create. doip node logical address range must be 0x0001~0x0DFF or 0x1000~0x7FFF.
*
* @param [in] logical_source_address : Unique identifier doip node.
* @param [in] doip_instence_type : Type of doip instance. Entity self or ecu of gateway.
* @param [in] callback_register : A collection of all callback function registered structures.
*
* @return The function execution returns NE_DOIP_RESULT_OK successfully.
*         If an internal error returns another code, see the file ne_doip_comm_def.h for details.
*
* @attention Synchronous I/F.
*/
ne_doip_result_t ne_doip_node_create(uint16_t logical_source_address,
                                     ne_doip_instence_type_t doip_instence_type,
                                     ne_doip_node_callback_register_t callback_register);

/**
* @brief Destroy the doip node of the corresponding logical address.
*
* @param [in] logical_source_address : Unique identifier doip node.
*
* @return The function execution returns NE_DOIP_RESULT_OK successfully.
*         If an internal error returns another code, see the file ne_doip_comm_def.h for details.
*
* @attention Synchronous I/F.
*/
ne_doip_result_t ne_doip_node_destroy(uint16_t logical_source_address);

/**
* @brief send diag data to test equipment from client.
*
* @param [in] diag_data_request : This is ne_doip_diag_data_request_t struct. For detailed definition, please see ne_doip_comm_def.h
*
* @return The function execution returns NE_DOIP_RESULT_OK successfully.
*         If an internal error returns another code, see the file ne_doip_comm_def.h for details.
*
* @attention Synchronous I/F.
*/
ne_doip_result_t ne_doip_node_diag_data_request(ne_doip_diag_data_request_t* diag_data_request);

/**
* @brief After the function confirmation_callback callback, the upper application sends the confirmation result through the interface.
*
* @param [in] logical_source_address : Must be doip entity logical address.
* @param [in] logical_target_address : Unique identifier test equipment
* @param [in] result : User confirmation reslut. Defined in file ne_doip_comm_def.h
*
* @return The function execution returns NE_DOIP_RESULT_OK successfully.
*         If an internal error returns another code, see the file ne_doip_comm_def.h for details.
*
* @attention Synchronous I/F.
*/
ne_doip_result_t ne_doip_node_send_user_conf_result(uint16_t logical_source_address,
                                                    uint16_t logical_target_address,
                                                    ne_doip_user_con_result_t result);

#ifdef  __cplusplus
}
#endif

#endif // NE_DOIP_NODE_H
/* EOF */