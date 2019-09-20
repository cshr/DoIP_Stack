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
 * @file ne_doip_client.h
 * @brief internal test equipment interface
 */

#ifndef NE_DOIP_CLIENT_H
#define NE_DOIP_CLIENT_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "ne_doip_comm_def.h"


/**
* @brief Internal test equipment create. Used to simulate external test equipment.
*
* @param [in] logical_source_address : Unique identifier internal test equipment.
* @param [in] callback_register : A collection of all callback function registered structures.
*
* @return The function execution returns NE_DOIP_RESULT_OK successfully.
*         If an internal error returns another code, see the file ne_doip_comm_def.h for details.
*
* @attention Synchronous I/F. The logical address range must be 0x0F00~0x0F7F.
*/
ne_doip_result_t ne_doip_equip_create(uint16_t logical_source_address,
                                      ne_doip_equip_callback_register_t callback_register);

/**
* @brief internal test equipment destroy.
*
* @return The function execution returns NE_DOIP_RESULT_OK successfully.
*         If an internal error returns another code, see the file ne_doip_comm_def.h for details.
*
* @attention Synchronous I/F.
*/
ne_doip_result_t ne_doip_equip_destroy();

/**
* @brief vehicle identity request.
*
* @param [in] data : Request data for vehicle identity.
* @param [in] length : transfer data length. data is NULL [length:0] | data is EID_Array [length:6] | data is VIN_Array [length:17]
*
* @return The function execution returns NE_DOIP_RESULT_OK successfully.
*         If an internal error returns another code, see the file ne_doip_comm_def.h for details.
*
* @attention Synchronous I/F. The length of the data must conform to the protocol standard
*/
ne_doip_result_t ne_doip_equip_identity(char* data, uint32_t length);

/**
* @brief routing activation request.
*
* @param [in] logical_source_address : Unique identifier internal test equipment.
* @param [in] activation_type : 0x00->Default | 0x01->WWH-OBD | 0xE0->Central security | Other->Reserved.
* @param [in] oem_specific_use : Oem custom parameter, which can be used for authentication when route is activated.
* @param [in] eid : Used to activate the doip entity. If it is set to null or its own EID, it will activate itself.
*
* @return The function execution returns NE_DOIP_RESULT_OK successfully.
*         If an internal error returns another code, see the file ne_doip_comm_def.h for details.
*
* @attention Synchronous I/F.
*/
ne_doip_result_t ne_doip_equip_routing_active(uint16_t logical_source_address,
                                              uint8_t activation_type, uint32_t oem_specific_use,
                                              char* eid);

/**
* @brief alive check response. [This interface can maintain a TCP connection and can be sent from time to time.]
*
* @param [in] logical_target_address : doip node logical address that needs to be Keep the TCP connection.
*
* @return The function execution returns NE_DOIP_RESULT_OK successfully.
*         If an internal error returns another code, see the file ne_doip_comm_def.h for details.
*
* @attention Synchronous I/F.
*/
ne_doip_result_t ne_doip_equip_alive_check_res(uint16_t logical_target_address);

/**
* @brief entity status request.
*
* @param [in] eid : Used to request the corresponding doip entity. If it is set to null or its own EID, it will return its own entity information.
*
* @return The function execution returns NE_DOIP_RESULT_OK successfully.
*         If an internal error returns another code, see the file ne_doip_comm_def.h for details.
*
* @attention Synchronous I/F.
*/
ne_doip_result_t ne_doip_equip_entity_status_req(char* eid);

/**
* @brief power mode request.
*
* @param [in] eid : Used to request the corresponding doip entity. If it is set to null or its own EID, it will return its own entity information.
*
* @return The function execution returns NE_DOIP_RESULT_OK successfully.
*         If an internal error returns another code, see the file ne_doip_comm_def.h for details.
*
* @attention Synchronous I/F.
*/
ne_doip_result_t ne_doip_equip_power_mode_req(char* eid);

/**
* @brief diagnostic request.
*
* @param [in] logical_target_address : doip node logical address that needs to be diagnosed.
* @param [in] ta_type : physical or functional addressing type.
* @param [in] data : transfer data.
* @param [in] length : transfer data length.
*
* @return The function execution returns NE_DOIP_RESULT_OK successfully.
*         If an internal error returns another code, see the file ne_doip_comm_def.h for details.
*
* @attention Synchronous I/F.
*/
ne_doip_result_t ne_doip_equip_diagnostic(uint16_t logical_target_address,
                                          ne_doip_ta_type_t ta_type,
                                          char* data,
                                          uint32_t length);

#ifdef  __cplusplus
}
#endif

#endif // NE_DOIP_CLIENT_H
/* EOF */