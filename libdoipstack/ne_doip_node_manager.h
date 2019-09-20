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
 * @file ne_doip_node_manager.h
 * @brief Protocol parsing module
 */

#ifndef NE_DOIP_NODE_MANAGER_H
#define NE_DOIP_NODE_MANAGER_H

#include "ne_doip_def.h"



int ne_doip_pack_node_regist(ne_doip_connection_t *conn, uint16_t logical_addr,
                             ne_doip_instence_type_t doip_instence_type);
int ne_doip_pack_regis_confirmation(ne_doip_connection_t *conn);
int ne_doip_pack_user_conf_result(ne_doip_connection_t *conn,
                                  uint16_t logical_source_address,
                                  uint16_t logical_target_address,
                                  ne_doip_user_con_result_t result);
int ne_doip_pack_diagnositc_res(ne_doip_connection_t *conn,
                                uint16_t logical_source_address,
                                uint16_t logical_target_address,
                                char* data,
                                uint32_t length);
void ne_doip_node_unpack(ne_doip_node_t* doip_node);


#endif // NE_DOIP_NODE_MANAGER_H
/* EOF */