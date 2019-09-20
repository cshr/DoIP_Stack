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
 * @file ne_doip_server.h
 * @brief Server interface
 */

#ifndef NE_DOIP_SERVER_H
#define NE_DOIP_SERVER_H

#ifdef  __cplusplus
extern "C" {
#endif

/**
* @brief Initialize the doip server.
*
* @param [in] config_path : Path to the doip stack configuration file.
*
* @attention Synchronous I/F.
*/
void ne_doip_server_init(const char* config_path);

/**
* @brief Deinitialize the doip server.
*
* @attention Synchronous I/F.
*/
void ne_doip_server_deinit();

/**
* @brief Set doip vin information 
*
* @attention Synchronous I/F.
*/
void ne_doip_set_vin_info(const char *vin);

/**
* @brief Set doip eid information
*
* @attention Synchronous I/F.
*/
void ne_doip_set_eid_info(const char *eid);

/**
* @brief Set doip gid information
*
* @attention Synchronous I/F.
*/
void ne_doip_set_gid_info(const char *gid);

/**
* @brief Activation line becomes active, the system calls this interface to notify the DoIP stack.
*
*        If there is no activation line connection, such as an OTA module, you need to call this interface to activate the doip stack.
*
* @attention Synchronous I/F.
*/
void ne_doip_activation_line_switch_active();

/**
* @brief Activation line becomes deactive, the system calls this interface to notify the DoIP stack.
*
*        If there is no activation line connection, such as the OTA module, if you need to release all sockets, call this interface to deactive doip stack.
*
* @attention Synchronous I/F.
*/
void ne_doip_activation_line_switch_deactive();

/**
* @brief When the valid IP address has been allocated, the system calls this interface to notify the DoIP stack.
*
* @param [in] if_name : If_name when IP is configured.
*
* @attention Synchronous I/F.
*/
void ne_doip_request_ip_addr_assignment(const char* if_name);

/**
* @brief When the IP address discards or invalidates its current IP address, the system calls this interface to notify the DoIP stack.
*
* @param [in] if_name : If_name when IP is released or invalid.
*
* @attention Synchronous I/F.
*/
void ne_doip_release_ip_addr_assignment(const char* if_name);

/**
* @brief update power mode status[0x00: not ready; 0x01: ready; 0x02: not supported].
*
* @attention Synchronous I/F.
*/
void ne_doip_powermode_status_change(unsigned char power_mode_status);


#ifdef  __cplusplus
}
#endif

#endif // NE_DOIP_SERVER_H
/* EOF */
