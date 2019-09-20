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
 * @file ne_doip_config.h
 * @brief config file management
 */

#ifndef NE_DOIP_CONFIG_H
#define NE_DOIP_CONFIG_H

#include "ne_doip_def.h"


/**
* @brief Doip node type
* When the client registers a logical address, it distinguishes its own node type.
*/
typedef enum NE_DOIP_NODE_TYPE
{
    NE_DOIP_NT_GATEWAY = 0x00,    ///< Node type is gateway
    NE_DOIP_NT_NODE    = 0x01,    ///< Node type is node
    NE_DOIP_NT_UNKOWN  = 0xFF     ///< Node type is unkown
}ne_doip_node_type_t;


ne_doip_config_t* ne_doip_load_config(const char *path);
void ne_doip_unload_config(ne_doip_config_t*config);

ne_doip_net_source_t* ne_doip_net_source_list_find_by_ifname(ne_doip_config_t*config, const char *ifname);
ne_doip_net_source_t* ne_doip_net_source_list_find_by_announce_wait_timeid(ne_doip_config_t*config, int timer_id);
ne_doip_net_source_t* ne_doip_net_source_list_find_by_announce_interval_timeid(ne_doip_config_t*config, int timer_id);
uint8_t ne_doip_is_functianal_address(ne_doip_config_t*config, uint16_t logical_address);

#endif // NE_DOIP_CONFIG_H
/* EOF */