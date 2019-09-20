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

#include "ne_doip_client_manager.h"
#include "ne_doip_util.h"


int ne_doip_pack_test_equip_regist(ne_doip_connection_t *conn, uint16_t logical_source_address)
{
    NE_DOIP_PRINT("[test equip] ne_doip_pack_test_equip_regist start ..\n");

    uint32_t payload_length = NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    uint8_t pos = 0;
    conn->out.data[pos] = NE_DOIP_IN_EQUIP_RGIST;
    pos += NE_DOIP_IN_COMMAND_LENGTH;
    memcpy(conn->out.data + pos, &payload_length, NE_DOIP_IN_DATA_LENGTH);
    pos += NE_DOIP_IN_DATA_LENGTH;
    logical_source_address = ne_doip_bswap_16(logical_source_address);
    memcpy(conn->out.data + pos, &logical_source_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;

    int res = ne_doip_connection_write(conn);
    if (res > 0) {
        return 0;
    }
    else {
        return -1;
    }
}

int ne_doip_pack_vehicle_identify(ne_doip_connection_t *conn)
{
    NE_DOIP_PRINT("[test equip] ne_doip_pack_vehicle_identify start ..\n");

    uint32_t payload_length = 0;
    conn->out.data[0] = NE_DOIP_IN_EQUIP_IDENTITY_REQ;
    memcpy(conn->out.data + NE_DOIP_IN_COMMAND_LENGTH, &payload_length, NE_DOIP_IN_DATA_LENGTH);
    conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH;

    int res = ne_doip_connection_write(conn);
    if (res > 0) {
        return 0;
    }
    else {
        return -1;
    }
}

int ne_doip_pack_vehicle_identify_eid(ne_doip_connection_t *conn, char* eid)
{
    NE_DOIP_PRINT("[test equip] ne_doip_pack_vehicle_identify_eid start ..\n");

    uint32_t payload_length = NE_DOIP_EID_SIZE;
    uint8_t pos = 0;
    conn->out.data[pos] = NE_DOIP_IN_EQUIP_IDENTITY_REQ_EID;
    pos += NE_DOIP_IN_COMMAND_LENGTH;
    memcpy(conn->out.data + pos, &payload_length, NE_DOIP_IN_DATA_LENGTH);
    pos += NE_DOIP_IN_DATA_LENGTH;
    memcpy(conn->out.data + pos, eid, NE_DOIP_EID_SIZE);
    conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;
    
    int res = ne_doip_connection_write(conn);
    if (res > 0) {
        return 0;
    }
    else {
        return -1;
    }
}

int ne_doip_pack_vehicle_identify_vin(ne_doip_connection_t *conn, char* vin)
{
    NE_DOIP_PRINT("[test equip] ne_doip_pack_vehicle_identify_vin start ..\n");

    uint32_t payload_length = NE_DOIP_VIN_SIZE;
    uint8_t pos = 0;
    conn->out.data[pos] = NE_DOIP_IN_EQUIP_IDENTITY_REQ_VIN;
    pos += NE_DOIP_IN_COMMAND_LENGTH;
    memcpy(conn->out.data + pos, &payload_length, NE_DOIP_IN_DATA_LENGTH);
    pos += NE_DOIP_IN_DATA_LENGTH;
    memcpy(conn->out.data + pos, vin, NE_DOIP_VIN_SIZE);
    conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;

    int res = ne_doip_connection_write(conn);
    if (res > 0) {
        return 0;
    }
    else {
        return -1;
    }
}

int ne_doip_pack_routing_active(ne_doip_connection_t *conn, uint16_t logical_source_address, 
                                uint8_t activation_type, uint32_t oem_specific_use, char* eid)
{
    NE_DOIP_PRINT("[test equip] ne_doip_pack_routing_active start ..\n");

    uint32_t payload_length = 0;
    if (0 == oem_specific_use) {
        payload_length = NE_DOIP_EID_SIZE + NE_DOIP_LOGICAL_ADDRESS_LENGTH + NE_DOIP_ACTIVATION_TYPE_LENGTH + NE_DOIP_RESERVED_LENGTH;
    }
    else {
        payload_length = NE_DOIP_EID_SIZE + NE_DOIP_LOGICAL_ADDRESS_LENGTH + NE_DOIP_ACTIVATION_TYPE_LENGTH + NE_DOIP_RESERVED_LENGTH + NE_DOIP_OEM_SPECIFIC_LENGTH;
    }

    uint8_t pos = 0;
    conn->out.data[pos] = NE_DOIP_IN_EQUIP_ROUTING_ACTIVE;
    pos += NE_DOIP_IN_COMMAND_LENGTH;
    memcpy(conn->out.data + pos, &payload_length, NE_DOIP_IN_DATA_LENGTH);
    pos += NE_DOIP_IN_DATA_LENGTH;
    if (eid != NULL) {
        memcpy(conn->out.data + pos, eid, NE_DOIP_EID_SIZE);
    }
    pos += NE_DOIP_EID_SIZE;
    logical_source_address = ne_doip_bswap_16(logical_source_address);
    memcpy(conn->out.data + pos, &logical_source_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    conn->out.data[pos] = activation_type;
    pos += NE_DOIP_ACTIVATION_TYPE_LENGTH;
    memset(conn->out.data + pos, 0, NE_DOIP_RESERVED_LENGTH);  // 4 byte, default 0x00000000, reserved by ISO 13400
    if (oem_specific_use != 0) {
        pos += NE_DOIP_RESERVED_LENGTH;
        oem_specific_use = ne_doip_bswap_32(oem_specific_use);
        memcpy(conn->out.data + pos, &oem_specific_use, NE_DOIP_OEM_SPECIFIC_LENGTH);
    }
    conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;

    int res = ne_doip_connection_write(conn);
    if (res > 0) {
        return 0;
    }
    else {
        return -1;
    }
}

int ne_doip_pack_alive_check_response(ne_doip_connection_t *conn, uint16_t logical_target_address)
{
    NE_DOIP_PRINT("[test equip] ne_doip_pack_alive_check_response start ..\n");

    uint32_t payload_length = NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    uint8_t pos = 0;
    conn->out.data[pos] = NE_DOIP_IN_EQUIP_ALIVE_CHECK;
    pos += NE_DOIP_IN_COMMAND_LENGTH;
    memcpy(conn->out.data + pos, &payload_length, NE_DOIP_IN_DATA_LENGTH);
    pos += NE_DOIP_IN_DATA_LENGTH;
    logical_target_address = ne_doip_bswap_16(logical_target_address);
    memcpy(conn->out.data + pos, &logical_target_address, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + NE_DOIP_LOGICAL_ADDRESS_LENGTH;

    int res = ne_doip_connection_write(conn);
    if (res > 0) {
        return 0;
    }
    else {
        return -1;
    }
}

int ne_doip_pack_entity_status(ne_doip_connection_t *conn, char* eid)
{
    NE_DOIP_PRINT("[test equip] ne_doip_pack_entity_status start ..\n");

    uint32_t payload_length = NE_DOIP_EID_SIZE;
    uint8_t pos = 0;
    conn->out.data[pos] = NE_DOIP_IN_EQUIP_ENTITY_STATUS;
    pos += NE_DOIP_IN_COMMAND_LENGTH;
    memcpy(conn->out.data + pos, &payload_length, NE_DOIP_IN_DATA_LENGTH);
    pos += NE_DOIP_IN_DATA_LENGTH;
    if (eid != NULL) {
        memcpy(conn->out.data + pos, eid, NE_DOIP_EID_SIZE);
    }
    conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;

    int res = ne_doip_connection_write(conn);
    if (res > 0) {
        return 0;
    }
    else {
        return -1;
    }
}

int ne_doip_pack_power_mode(ne_doip_connection_t *conn, char* eid)
{
    NE_DOIP_PRINT("[test equip] ne_doip_pack_power_mode start ..\n");

    uint32_t payload_length = NE_DOIP_EID_SIZE;
    uint8_t pos = 0;
    conn->out.data[pos] = NE_DOIP_IN_EQUIP_POWER_MODE;
    pos += NE_DOIP_IN_COMMAND_LENGTH;
    memcpy(conn->out.data + pos, &payload_length, NE_DOIP_IN_DATA_LENGTH);
    pos += NE_DOIP_IN_DATA_LENGTH;
    if (eid != NULL) {
        memcpy(conn->out.data + pos, eid, NE_DOIP_EID_SIZE);
    }
    conn->out.data_size = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + payload_length;

    int res = ne_doip_connection_write(conn);
    if (res > 0) {
        return 0;
    }
    else {
        return -1;
    }
}

int ne_doip_pack_diagnositc_req(ne_doip_connection_t *conn,
                                uint16_t logical_source_address,
                                uint16_t logical_target_address,
                                ne_doip_ta_type_t ta_type,
                                char* data,
                                uint32_t length)
{
    NE_DOIP_PRINT("[test equip] ne_doip_pack_diagnositc_req start ..\n");

    uint8_t header_length = NE_DOIP_IN_COMMAND_LENGTH + NE_DOIP_IN_DATA_LENGTH + NE_DOIP_LOGICAL_ADDRESS_LENGTH * 2 + NE_DOIP_TA_TYPE_LENGTH;

    uint32_t size = header_length + NE_DOIP_BLANK_2_LENGTH + length;
    char* send_data = (char*)malloc(size);
    memset(send_data, 0, size);

    uint32_t payload_length = NE_DOIP_LOGICAL_ADDRESS_LENGTH * 2 + length;
    uint8_t pos = 0;
    send_data[pos] = NE_DOIP_IN_EQUIP_DIAGNOSTIC;
    pos += NE_DOIP_IN_COMMAND_LENGTH;
    memcpy(send_data + pos, &payload_length, NE_DOIP_IN_DATA_LENGTH);
    pos += NE_DOIP_IN_DATA_LENGTH;
    send_data[pos++] = 0x00; // Complement the standard protocol header size
    send_data[pos++] = 0x00; // Complement the standard protocol header size
    send_data[pos++] = ta_type;
    uint16_t logical_addr = ne_doip_bswap_16(logical_source_address);
    memcpy(send_data + pos, &logical_addr, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    logical_addr = ne_doip_bswap_16(logical_target_address);
    memcpy(send_data + pos, &logical_addr, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
    pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
    memcpy(send_data + pos, data, length);

    int res = ne_doip_connection_write_raw(conn->fd, send_data, size);
    free(send_data);
    if (res > 0) {
        return 0;
    }
    else {
        return -1;
    }
}

uint32_t ne_doip_equip_unpack_exec(ne_doip_test_equip_t* test_equip, uint32_t pos_t)
{
    ne_doip_connection_t *conn = test_equip->connection;

    unsigned char res_type = conn->in.data[pos_t];
    pos_t += NE_DOIP_IN_COMMAND_LENGTH;
    uint32_t payload_data_length = 0;
    memcpy(&payload_data_length, conn->in.data + pos_t, NE_DOIP_IN_DATA_LENGTH);
    pos_t += NE_DOIP_IN_DATA_LENGTH;

    if (pos_t + payload_data_length > conn->in.data_size) {
        NE_DOIP_PRINT("Data is stuck, this case is not processed\n");
        return 0;
    }

    char* buffer = (char*)malloc(payload_data_length);
    memset(buffer, 0, payload_data_length);

    memcpy(buffer, conn->in.data + pos_t, payload_data_length);
    pos_t += payload_data_length;

    switch (res_type) {
    case NE_DOIP_IN_EQUIP_HEADER_NACK:
        // This case is temporarily canceled. do nothing
        break;
    case NE_DOIP_IN_EQUIP_ANN_IDEN_RES:
    {
        NE_DOIP_PRINT("internal payload type is [NE_DOIP_IN_EQUIP_ANN_IDEN_RES]!\n");

        struct ne_doip_vehicle_identity_resinfo* info = (struct ne_doip_vehicle_identity_resinfo*)malloc(sizeof *info);
        memset(info, 0, sizeof *info);

        uint8_t pos = 0;
        memcpy(info->vin, buffer, NE_DOIP_VIN_SIZE);
        pos += NE_DOIP_VIN_SIZE;
        uint16_t entity_logical_address = 0x0000;
        memcpy(&entity_logical_address, buffer + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        info->logical_address = ne_doip_bswap_16(entity_logical_address);
        memcpy(info->eid, buffer + pos, NE_DOIP_EID_SIZE);
        pos += NE_DOIP_EID_SIZE;
        memcpy(info->gid, buffer + pos, NE_DOIP_GID_SIZE);
        pos += NE_DOIP_GID_SIZE;
        memcpy(&info->further_action, buffer + pos, NE_DOIP_FURTHER_ACTION_LENGTH);
        if (NE_DOIP_ANNOUNCE_OR_IDENTITYRES_ALL_LENGTH == payload_data_length) {
            pos += NE_DOIP_FURTHER_ACTION_LENGTH;
            memcpy(&info->vin_gid_sync, buffer + pos, NE_DOIP_VIN_GID_SYNC_LENGTH);
        }

        test_equip->vehicle_identity_callback(info);
        free(info);
        break;
    }
    case NE_DOIP_IN_EQUIP_ROUTING_ACTIVE:
    {
        NE_DOIP_PRINT("internal payload type is [NE_DOIP_IN_EQUIP_ROUTING_ACTIVE]!\n");

        uint8_t pos = 0;
        uint16_t equip_logical_address = 0x0000;
        memcpy(&equip_logical_address, buffer, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        equip_logical_address = ne_doip_bswap_16(equip_logical_address);
        if (equip_logical_address != test_equip->logical_source_address) {
            NE_DOIP_PRINT("test equip address is not match!\n");
            break;
        }
        else {
            ne_doip_routing_active_resinfo_t* info = (ne_doip_routing_active_resinfo_t*)malloc(sizeof *info);
            memset(info, 0, sizeof(ne_doip_routing_active_resinfo_t));

            uint16_t entity_logical_address = 0x0000;
            memcpy(&entity_logical_address, buffer + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
            pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
            info->entity_logical_address = ne_doip_bswap_16(entity_logical_address);
            info->equip_logical_address = equip_logical_address;
            memcpy(&info->routing_active_res_code, buffer + pos, NE_DOIP_RA_RES_CODE_LENGTH);
            pos += NE_DOIP_RA_RES_CODE_LENGTH;
            memcpy(&info->reserved_by_ISO13400, buffer + pos, NE_DOIP_RESERVED_LENGTH);
            pos += NE_DOIP_RESERVED_LENGTH;
            if (NE_DOIP_OEM_SPECIFIC_LENGTH == payload_data_length - pos) {
                memcpy(&info->reserved_by_oem, buffer + pos, NE_DOIP_OEM_SPECIFIC_LENGTH);
            }
            test_equip->routing_active_callback(info);
            free(info);
        }
        break;
    }
    case NE_DOIP_IN_EQUIP_ENTITY_STATUS:
    {
        NE_DOIP_PRINT("internal payload type is [NE_DOIP_IN_EQUIP_ENTITY_STATUS]!\n");
        struct ne_doip_entity_status_info* info = (struct ne_doip_entity_status_info*)malloc(sizeof *info);
        memset(info, 0, sizeof *info);
        
        uint8_t pos = 0;
        memcpy(&info->node_type, buffer, NE_DOIP_NODE_TYPE_LENGTH);
        pos += NE_DOIP_NODE_TYPE_LENGTH;
        memcpy(&info->MCTS, buffer + pos, NE_DOIP_MCTS_LENGTH);
        pos += NE_DOIP_MCTS_LENGTH;
        memcpy(&info->NCTS, buffer + pos, NE_DOIP_NCTS_LENGTH);
        pos += NE_DOIP_NCTS_LENGTH;
        memcpy(&info->MDS, buffer + pos, NE_DOIP_MDS_LENGTH);

        test_equip->entity_status_callback(info);
        free(info);
        break;
    }
    case NE_DOIP_IN_EQUIP_POWER_MODE:
    {
        NE_DOIP_PRINT("internal payload type is [NE_DOIP_IN_EQUIP_POWER_MODE]!\n");
        uint8_t power_mode = 0x00;
        memcpy(&power_mode, buffer, NE_DOIP_POWERMODE_LENGTH);
        test_equip->power_mode_callback(power_mode);
        break;
    }
    case NE_DOIP_IN_EQUIP_DIAGNOSTIC_PACK:
    {
        NE_DOIP_PRINT("internal payload type is [NE_DOIP_IN_EQUIP_DIAGNOSTIC_PACK]!\n");
        uint16_t entity_logical_address = 0x0000;
        uint16_t equip_logical_address = 0x0000;
        uint8_t pos = 0;
        memcpy(&entity_logical_address, buffer, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        memcpy(&equip_logical_address, buffer + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        entity_logical_address = ne_doip_bswap_16(entity_logical_address);
        equip_logical_address = ne_doip_bswap_16(equip_logical_address);
        if (equip_logical_address != test_equip->logical_source_address) {
            NE_DOIP_PRINT("test equip address is not match!\n");
            break;
        }
        else {
            uint8_t positive_ack_code = 0x00;
            memcpy(&positive_ack_code, buffer + pos, NE_DOIP_ACK_CODE_LENGTH);
            test_equip->daig_pack_callback(entity_logical_address, positive_ack_code);
        }
        break;
    }
    case NE_DOIP_IN_EQUIP_DIAGNOSTIC_NACK:
    {
        NE_DOIP_PRINT("internal payload type is [NE_DOIP_IN_EQUIP_DIAGNOSTIC_NACK]!\n");
        uint16_t entity_logical_address = 0x0000;
        uint16_t equip_logical_address = 0x0000;
        uint8_t pos = 0;
        memcpy(&entity_logical_address, buffer, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        memcpy(&equip_logical_address, buffer + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        entity_logical_address = ne_doip_bswap_16(entity_logical_address);
        equip_logical_address = ne_doip_bswap_16(equip_logical_address);
        if (equip_logical_address != test_equip->logical_source_address) {
            NE_DOIP_PRINT("test equip address is not match!\n");
            break;
        }
        else {
            uint8_t negative_ack_code = 0x00;
            memcpy(&negative_ack_code, buffer + pos, NE_DOIP_NACK_CODE_LENGTH);
            test_equip->daig_pack_callback(entity_logical_address, negative_ack_code);
        }
        break;
    }
    case NE_DOIP_IN_EQUIP_DIAGNOSTIC:
    {
        NE_DOIP_PRINT("internal payload type is [NE_DOIP_IN_EQUIP_DIAGNOSTIC]!\n");
        uint16_t entity_logical_address = 0x0000;
        uint16_t equip_logical_address = 0x0000;
        uint8_t pos = 0;
        memcpy(&entity_logical_address, buffer, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        memcpy(&equip_logical_address, buffer + pos, NE_DOIP_LOGICAL_ADDRESS_LENGTH);
        pos += NE_DOIP_LOGICAL_ADDRESS_LENGTH;
        entity_logical_address = ne_doip_bswap_16(entity_logical_address);
        equip_logical_address = ne_doip_bswap_16(equip_logical_address);

        if (equip_logical_address != test_equip->logical_source_address) {
            NE_DOIP_PRINT("test equip address is not match!\n");
            break;
        }
        else {
            test_equip->diagnostic_callback(entity_logical_address, buffer + pos, payload_data_length - pos);
        }
        break;
    }
    default:
        break;
    }

    free(buffer);

    return pos_t;

}

void ne_doip_equip_unpack(ne_doip_test_equip_t* test_equip)
{
    NE_DOIP_PRINT("[test equip] ne_doip_equip_unpack start ..\n");
    if (NULL == test_equip) {
        NE_DOIP_PRINT("doip_equip is null ..\n");
        return;
    }

    ne_doip_connection_t *conn = test_equip->connection;
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
        pos = ne_doip_equip_unpack_exec(test_equip, pos);
        if (0 == pos) {
            break;
        }
    } while (conn->in.data_size - pos > 0);
}
/* EOF */
