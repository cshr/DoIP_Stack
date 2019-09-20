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

#include "ne_doip_node_manager.h"
#include "ne_doip_util.h"


int ne_doip_pack_node_regist(ne_doip_connection_t *conn, uint16_t logical_addr,
                             ne_doip_instence_type_t doip_instence_type)
{
    NE_DOIP_PRINT("[doip_node] ne_doip_pack_announce start ..\n");
    if (NULL == conn) {
        NE_DOIP_PRINT("conn is null ..\n");
        return -1;
    }

    uint32_t size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_LOGICAL_ADDRESS_LENGTH + NE_DOIP_IN_DATA_LENGTH + NE_DOIP_INSTANCE_TYPE_LENGTH;
    char* data = (char*)malloc(size);
    memset(data, 0, size);

    uint32_t payload_length = NE_DOIP_LOGICAL_ADDRESS_LENGTH + NE_DOIP_INSTANCE_TYPE_LENGTH;

    uint8_t pos = 0;
    data[pos] = NE_DOIP_IN_PAYLOADTYPE_NODE_RGIST;
    pos += NE_DOIP_IN_COMMAND_LENGTH;
    memcpy(data + pos, &payload_length, NE_DOIP_IN_DATA_LENGTH);
    pos += NE_DOIP_IN_DATA_LENGTH;
    logical_addr = ne_doip_bswap_16(logical_addr);
    memcpy(data + pos, &logical_addr, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    memcpy(data + pos, &doip_instence_type, NE_DOIP_INSTANCE_TYPE_LENGTH);

    memcpy(conn->out.data, data, size);
    conn->out.data_size = size;
    free(data);
    return 0;
}

int ne_doip_pack_regis_confirmation(ne_doip_connection_t *conn)
{
    NE_DOIP_PRINT("[doip_node] ne_doip_pack_regis_confirmation start ..\n");
    if (NULL == conn) {
        NE_DOIP_PRINT("conn is null ..\n");
        return -1;
    }

    uint32_t size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH;
    char* data = (char*)malloc(size);
    memset(data, 0, size);
    uint32_t payload_length = 0;

    uint8_t pos = 0;
    data[pos] = NE_DOIP_IN_PAYLOADTYPE_REGIST_USER_CONF;
    pos += NE_DOIP_IN_COMMAND_LENGTH;
    memcpy(data + pos, &payload_length, NE_DOIP_IN_DATA_LENGTH);

    memcpy(conn->out.data, data, size);
    conn->out.data_size = size;
    free(data);
    return 0;
}

int ne_doip_pack_user_conf_result(ne_doip_connection_t *conn,
                                  uint16_t logical_source_address,
                                  uint16_t logical_target_address, 
                                  ne_doip_user_con_result_t result)
{
    NE_DOIP_PRINT("[doip_node] ne_doip_pack_user_conf_result start ..\n");
    if (NULL == conn) {
        NE_DOIP_PRINT("conn is null ..\n");
        return -1;
    }

    uint32_t size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + NE_DOIP_LOGICAL_ADDRESS_LENGTH * 2 + NE_DOIP_CONF_RESULT_LENGTH;
    char* data = (char*)malloc(size);
    memset(data, 0, size);

    uint32_t payload_length = NE_DOIP_LOGICAL_ADDRESS_LENGTH * 2 + NE_DOIP_CONF_RESULT_LENGTH;
    uint8_t pos = 0;
    data[pos] = NE_DOIP_IN_PAYLOADTYPE_USER_CONF_RESULT;
    pos += NE_DOIP_IN_COMMAND_LENGTH;
    memcpy(data + pos, &payload_length, NE_DOIP_IN_DATA_LENGTH);
    pos += NE_DOIP_IN_DATA_LENGTH;
    uint16_t logical_addr = ne_doip_bswap_16(logical_source_address);
    memcpy(data + pos, &logical_addr, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    logical_addr = ne_doip_bswap_16(logical_target_address);
    memcpy(data + pos, &logical_addr, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    memcpy(data + pos, &result, NE_DOIP_CONF_RESULT_LENGTH);

    memcpy(conn->out.data, data, size);
    conn->out.data_size = size;
    free(data);
    return 0;
}

int ne_doip_pack_diagnositc_res(ne_doip_connection_t *conn,
                                uint16_t logical_source_address,
                                uint16_t logical_target_address,
                                char* data,
                                uint32_t length)
{
    NE_DOIP_PRINT("ne_doip_pack_diagnositc_res start ..\n");
    if (NULL == conn) {
        NE_DOIP_PRINT("conn is null ..\n");
        return -1;
    }

    uint32_t size = length + NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + NE_DOIP_LOGICAL_ADDRESS_LENGTH * 2;
    char* send_data = (char*)malloc(size);
    memset(send_data, 0, size);
    
    uint32_t payload_length = NE_DOIP_LOGICAL_ADDRESS_LENGTH * 2 + length;
    uint8_t pos = 0;
    send_data[pos] = NE_DOIP_IN_PAYLOADTYPE_DIAG_REQUEST;
    pos += NE_DOIP_IN_COMMAND_LENGTH;
    memcpy(send_data + pos, &payload_length, NE_DOIP_IN_DATA_LENGTH);
    pos += NE_DOIP_IN_DATA_LENGTH;
    uint16_t logical_addr = ne_doip_bswap_16(logical_source_address);
    memcpy(send_data + pos, &logical_addr, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    logical_addr = ne_doip_bswap_16(logical_target_address);
    memcpy(send_data + pos, &logical_addr, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    memcpy(send_data + pos, data, length);

    memcpy(conn->out.data, send_data, size);

    conn->out.data_size = size;
    free(send_data);
    return 0;
}

uint32_t ne_doip_node_unpack_exec(ne_doip_node_t* doip_node, uint32_t pos)
{
    ne_doip_connection_t *conn = doip_node->connection;

    if ((doip_node->diag_data_total_length - doip_node->diag_data_current_pos) > 0) {
        uint32_t rest_length = doip_node->diag_data_total_length - doip_node->diag_data_current_pos;
        NE_DOIP_PRINT("rest_length is %u...\n", rest_length);
        if (rest_length > conn->in.data_size) {
            doip_node->diag_data_current_pos += conn->in.data_size;
            if (doip_node->indication_callback != NULL) {
                ne_doip_diag_data_indication_t* diag_data_indication = malloc(sizeof *diag_data_indication);
                memset(diag_data_indication, 0, sizeof(ne_doip_diag_data_indication_t));

                diag_data_indication->logical_source_address = doip_node->logical_target_address;
                diag_data_indication->logical_target_address = doip_node->logical_source_address;
                diag_data_indication->ta_type = doip_node->ta_type;

                diag_data_indication->data = malloc(conn->in.data_size);
                memset(diag_data_indication->data, 0, conn->in.data_size);

                memcpy(diag_data_indication->data, conn->in.data, conn->in.data_size);
                diag_data_indication->data_length = conn->in.data_size;
                diag_data_indication->total_payload_size = doip_node->diag_data_total_length;
                diag_data_indication->current_payload_position = doip_node->diag_data_current_pos;
                diag_data_indication->result = NE_DOIP_RESULT_OK;

                doip_node->indication_callback(diag_data_indication);
                free(diag_data_indication->data);
                free(diag_data_indication);
            }
            pos = conn->in.data_size;
        }
        else {
            doip_node->diag_data_current_pos += rest_length;
            if (doip_node->indication_callback != NULL) {
                ne_doip_diag_data_indication_t* diag_data_indication = malloc(sizeof *diag_data_indication);
                memset(diag_data_indication, 0, sizeof *diag_data_indication);

                diag_data_indication->logical_source_address = doip_node->logical_target_address;
                diag_data_indication->logical_target_address = doip_node->logical_source_address;
                diag_data_indication->ta_type = doip_node->ta_type;

                diag_data_indication->data = malloc(rest_length);
                memset(diag_data_indication->data, 0, rest_length);

                memcpy(diag_data_indication->data, conn->in.data, rest_length);
                diag_data_indication->data_length = rest_length;
                diag_data_indication->total_payload_size = doip_node->diag_data_total_length;
                diag_data_indication->current_payload_position = doip_node->diag_data_current_pos;
                diag_data_indication->result = NE_DOIP_RESULT_OK;

                doip_node->indication_callback(diag_data_indication);
                free(diag_data_indication->data);
                free(diag_data_indication);
            }
            pos = rest_length;
            doip_node->diag_data_total_length = 0;
            doip_node->diag_data_current_pos = 0;
        }
    }
    else {
        unsigned char res_type = conn->in.data[pos];

        if (NE_DOIP_IN_PAYLOADTYPE_DIAG_INDICATION == res_type) {
            NE_DOIP_PRINT("type is [NE_DOIP_IN_PAYLOADTYPE_DIAG_INDICATION] .\n");
            pos += NE_DOIP_IN_COMMAND_LENGTH;
            memcpy(&doip_node->diag_data_total_length, conn->in.data + pos, NE_DOIP_IN_DATA_LENGTH);
            pos += NE_DOIP_IN_DATA_LENGTH;
            memcpy(&doip_node->ta_type, conn->in.data + pos, NE_DOIP_TA_TYPE_LENGTH);
            pos += NE_DOIP_TA_TYPE_LENGTH;
            uint32_t payload_data_length = 0;
            if (doip_node->diag_data_total_length > conn->in.data_size - NE_DOIP_IN_COMMAND_LENGTH - NE_DOIP_IN_DATA_LENGTH - NE_DOIP_TA_TYPE_LENGTH) {
                payload_data_length = conn->in.data_size - NE_DOIP_IN_COMMAND_LENGTH - NE_DOIP_IN_DATA_LENGTH - NE_DOIP_TA_TYPE_LENGTH;
            }
            else {
                payload_data_length = doip_node->diag_data_total_length;
            }
            doip_node->diag_data_current_pos += payload_data_length;

            char* buffer = (char*)malloc(payload_data_length);
            memset(buffer, 0, payload_data_length);
            if (pos + payload_data_length > conn->in.data_size) {
                NE_DOIP_PRINT("Data is stuck, this case is not processed\n");
                free(buffer);
                buffer = NULL;
                return 0;
            }
            memcpy(buffer, conn->in.data + pos, payload_data_length);
            pos += payload_data_length;

            uint16_t equip_logical_address;
            uint16_t entity_logical_address;
            uint8_t pos_t = 0;
            memcpy(&equip_logical_address, buffer, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            pos_t += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
            memcpy(&entity_logical_address, buffer + pos_t, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            equip_logical_address = ne_doip_bswap_16(equip_logical_address);
            entity_logical_address = ne_doip_bswap_16(entity_logical_address);

            doip_node->logical_target_address = equip_logical_address;

            if (entity_logical_address != doip_node->logical_source_address
                && doip_node->ta_type != NE_DOIP_TA_TYPE_FUNCTIONAL) {
                NE_DOIP_PRINT("entity address is not match!\n");
                free(buffer);
                buffer = NULL;
                return pos;
            }
            else {
                if (doip_node->indication_callback != NULL) {
                    ne_doip_diag_data_indication_t* diag_data_indication = malloc(sizeof *diag_data_indication);
                    memset(diag_data_indication, 0, sizeof(ne_doip_diag_data_indication_t));

                    diag_data_indication->logical_source_address = doip_node->logical_target_address;
                    diag_data_indication->logical_target_address = doip_node->logical_source_address;
                    diag_data_indication->ta_type = doip_node->ta_type;
                    diag_data_indication->data = buffer;
                    diag_data_indication->data_length = payload_data_length;
                    diag_data_indication->total_payload_size = doip_node->diag_data_total_length;
                    diag_data_indication->current_payload_position = doip_node->diag_data_current_pos;
                    diag_data_indication->result = NE_DOIP_RESULT_OK;

                    doip_node->indication_callback(diag_data_indication);
                    free(buffer);
                    free(diag_data_indication);
                }
                else {
                    free(buffer);
                    buffer = NULL;
                }
            }

            if (doip_node->diag_data_current_pos == doip_node->diag_data_total_length) {
                doip_node->diag_data_total_length = 0;
                doip_node->diag_data_current_pos = 0;
            }
        }
        else if (NE_DOIP_IN_PAYLOADTYPE_USER_CONF_RESULT == res_type) {
            NE_DOIP_PRINT("type is [NE_DOIP_IN_PAYLOADTYPE_USER_CONF_RESULT] .\n");
            pos += NE_DOIP_IN_COMMAND_LENGTH;
            uint32_t payload_data_length = 0;
            memcpy(&payload_data_length, conn->in.data + pos, NE_DOIP_IN_DATA_LENGTH);
            pos += NE_DOIP_IN_DATA_LENGTH;

            if (pos + payload_data_length > conn->in.data_size) {
                NE_DOIP_PRINT("Data is stuck, this case is not processed\n");
                return 0;
            }
            uint16_t equip_logical_address;
            uint16_t entity_logical_address;
            memcpy(&equip_logical_address, conn->in.data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
            memcpy(&entity_logical_address, conn->in.data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            pos = pos - NE_DOIP_LOGICAL_ADDRESS_LENGTH + payload_data_length;
            equip_logical_address = ne_doip_bswap_16(equip_logical_address);
            entity_logical_address = ne_doip_bswap_16(entity_logical_address);

            if (entity_logical_address != doip_node->logical_source_address) {
                NE_DOIP_PRINT("entity address is not match!\n");
                return pos;
            }
            else {
                if (doip_node->user_conf_callback != NULL) {
                    doip_node->user_conf_callback(equip_logical_address, entity_logical_address);
                }
            }
        }
        else if (NE_DOIP_IN_PAYLOADTYPE_DIAG_CONFIRM == res_type) {
            NE_DOIP_PRINT("type is [NE_DOIP_IN_PAYLOADTYPE_DIAG_CONFIRM] .\n");
            pos += NE_DOIP_IN_COMMAND_LENGTH;
            uint32_t payload_data_length = 0;
            memcpy(&payload_data_length, conn->in.data + pos, NE_DOIP_IN_DATA_LENGTH);
            pos += NE_DOIP_IN_DATA_LENGTH;

            if (pos + payload_data_length > conn->in.data_size) {
                NE_DOIP_PRINT("Data is stuck, this case is not processed\n");
                return 0;
            }
            uint16_t equip_logical_address;
            uint16_t entity_logical_address;
            memcpy(&equip_logical_address, conn->in.data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
            memcpy(&entity_logical_address, conn->in.data + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            pos = pos - NE_DOIP_LOGICAL_ADDRESS_LENGTH + payload_data_length;
            equip_logical_address = ne_doip_bswap_16(equip_logical_address);
            entity_logical_address = ne_doip_bswap_16(entity_logical_address);
            if (entity_logical_address != doip_node->logical_source_address) {
                NE_DOIP_PRINT("entity address is not match!\n");
                return pos;
            }
            else {
                ne_doip_diag_data_confirm_t* diag_data_confirm = malloc(sizeof *diag_data_confirm);
                memset(diag_data_confirm, 0, sizeof(ne_doip_diag_data_confirm_t));
                
                diag_data_confirm->logical_source_address = equip_logical_address;
                diag_data_confirm->logical_target_address = entity_logical_address;
                diag_data_confirm->ta_type = doip_node->ta_type;
                diag_data_confirm->result = NE_DOIP_RESULT_OK;

                doip_node->confirm_callback(diag_data_confirm);
                free(diag_data_confirm);
            }
        }
        else {
            return 0;
        }
    }

    return pos;
}

void ne_doip_node_unpack(ne_doip_node_t* doip_node)
{
    NE_DOIP_PRINT("[doip_node] ne_doip_node_unpack start ..\n");
    if (NULL == doip_node) {
        NE_DOIP_PRINT("doip_node is null ..\n");
        return;
    }

    ne_doip_connection_t *conn = doip_node->connection;
    if (NULL == conn) {
        NE_DOIP_PRINT("conn is null ..\n");
        return;
    }

#ifdef NE_DOIP_DEBUG
    printf("recv data is ");
    uint32_t i;
    for (i = 0; i < conn->in.data_size; ++i) {
        printf("%02X", (unsigned char)conn->in.data[i]);
    }
    printf("\n");
#endif

    uint32_t pos = 0;
    do {
        pos = ne_doip_node_unpack_exec(doip_node, pos);
        if (0 == pos) {
            break;
        }
    } while (conn->in.data_size - pos > 0);
}
/* EOF */
