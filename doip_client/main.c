#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>

#include "../libdoipstack/ne_doip_client.h"
#include "../libdoipstack/ne_doip_node.h"
#include "../libdoipstack/ne_doip_comm_def.h"
#include "../libdoipstack/ne_doip_util.h"


static ne_doip_vehicle_identity_resinfo_t* my_identity_resinfo = NULL;
static ne_doip_routing_active_resinfo_t* my_active_resinfo = NULL;
static ne_doip_entity_status_info_t* my_entity_status_info = NULL;
uint16_t equip_logical_address = 0x0F1F;
char* recv_file_name = "/home/anxuecheng/recv_data.dat";


void diag_indication_callback_1(ne_doip_diag_data_indication_t* diag_data_indication)
{
    NE_DOIP_PRINT("diag_indication_callback_1 is called... result code is [%d]\n", diag_data_indication->result);
    NE_DOIP_PRINT("logical_source_address is [%04X]\n", diag_data_indication->logical_source_address);
    NE_DOIP_PRINT("logical_target_address is [%04X]\n", diag_data_indication->logical_target_address);
    NE_DOIP_PRINT("TAType is [%u]\n", diag_data_indication->ta_type);

    // printf("recv data is ");
    // uint32_t i;
    // for (i = 0; i < diag_data_indication->data_length; ++i) {
    //     printf("%02X", diag_data_indication->data[i]);
    // }
    // printf("\n");

    if (diag_data_indication->current_payload_position == diag_data_indication->total_payload_size) {
        NE_DOIP_PRINT("diagnostic start...\n");
        sleep(1);
        NE_DOIP_PRINT("diagnostic end...\n");

        char send_data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

        ne_doip_diag_data_request_t* diag_data_request = malloc(sizeof *diag_data_request);
        diag_data_request->logical_source_address = diag_data_indication->logical_target_address;
        diag_data_request->logical_target_address = diag_data_indication->logical_source_address;
        diag_data_request->ta_type = diag_data_indication->ta_type;
        diag_data_request->data = send_data;
        diag_data_request->data_length = 6;
        ne_doip_node_diag_data_request(diag_data_request);
        free(diag_data_request);
        NE_DOIP_PRINT("client send diagnostic data is end...\n");
    }

}

void diag_confirm_callback_1(ne_doip_diag_data_confirm_t *diag_data_confirm)
{
    NE_DOIP_PRINT("diag_confirm_callback_1 is called... result code is [%d]\n", diag_data_confirm->result);
    NE_DOIP_PRINT("logical_source_address is [%04X]\n", diag_data_confirm->logical_source_address);
    NE_DOIP_PRINT("logical_target_address is [%04X]\n", diag_data_confirm->logical_target_address);
    NE_DOIP_PRINT("TAType is [%u]\n", diag_data_confirm->ta_type);
}

void diag_indication_callback_2(ne_doip_diag_data_indication_t* diag_data_indication)
{
    NE_DOIP_PRINT("diag_indication_callback_2 is called... result code is [%d]\n", diag_data_indication->result);
    NE_DOIP_PRINT("logical_source_address is [%04X]\n", diag_data_indication->logical_source_address);
    NE_DOIP_PRINT("logical_target_address is [%04X]\n", diag_data_indication->logical_target_address);
    NE_DOIP_PRINT("TAType is [%u]\n", diag_data_indication->ta_type);

    // printf("recv data is ");
    // uint32_t i;
    // for (i = 0; i < diag_data_indication->data_length; ++i) {
    //     printf("%02X", diag_data_indication->data[i]);
    // }
    // printf("\n");

    if (diag_data_indication->current_payload_position == diag_data_indication->total_payload_size) {
        NE_DOIP_PRINT("diagnostic start...\n");
        sleep(1);
        NE_DOIP_PRINT("diagnostic end...\n");

        char send_data[] = {0x11, 0x22, 0x33, 0x04, 0x05, 0x06};

        ne_doip_diag_data_request_t* diag_data_request = malloc(sizeof *diag_data_request);
        if (NULL == diag_data_request) {
            return;
        }
        memset(diag_data_request, 0, sizeof(ne_doip_diag_data_request_t));
        diag_data_request->logical_source_address = diag_data_indication->logical_target_address;
        diag_data_request->logical_target_address = diag_data_indication->logical_source_address;
        diag_data_request->ta_type = diag_data_indication->ta_type;
        diag_data_request->data = send_data;
        diag_data_request->data_length = 6;
        ne_doip_node_diag_data_request(diag_data_request);
        free(diag_data_request);
        NE_DOIP_PRINT("client send diagnostic data is end...\n");
    }
}

void diag_confirm_callback_2(ne_doip_diag_data_confirm_t *diag_data_confirm)
{
    NE_DOIP_PRINT("diag_confirm_callback_2 is called... result code is [%d]\n", diag_data_confirm->result);
    NE_DOIP_PRINT("logical_source_address is [%04X]\n", diag_data_confirm->logical_source_address);
    NE_DOIP_PRINT("logical_target_address is [%04X]\n", diag_data_confirm->logical_target_address);
    NE_DOIP_PRINT("TAType is [%u]\n", diag_data_confirm->ta_type);
}

void diag_indication_callback_3(ne_doip_diag_data_indication_t* diag_data_indication)
{
    NE_DOIP_PRINT("diag_indication_callback_3 is called... result code is [%d]\n", diag_data_indication->result);
    NE_DOIP_PRINT("logical_source_address is [%04X]\n", diag_data_indication->logical_source_address);
    NE_DOIP_PRINT("logical_target_address is [%04X]\n", diag_data_indication->logical_target_address);
    NE_DOIP_PRINT("TAType is [%u]\n", diag_data_indication->ta_type);

    // printf("recv data is ");
    // uint32_t i;
    // for (i = 0; i < diag_data_indication->data_length; ++i) {
    //     printf("%02X", diag_data_indication->data[i]);
    // }
    // printf("\n");

    if (diag_data_indication->current_payload_position == diag_data_indication->total_payload_size) {
        NE_DOIP_PRINT("diagnostic start...\n");
        sleep(1);
        NE_DOIP_PRINT("diagnostic end...\n");

        char send_data[] = {0x01, 0x02, 0x03, 0x44, 0x55, 0x66};

        ne_doip_diag_data_request_t* diag_data_request = malloc(sizeof *diag_data_request);
        if (NULL == diag_data_request) {
            return;
        }
        diag_data_request->logical_source_address = diag_data_indication->logical_target_address;
        diag_data_request->logical_target_address = diag_data_indication->logical_source_address;
        diag_data_request->ta_type = diag_data_indication->ta_type;
        diag_data_request->data = send_data;
        diag_data_request->data_length = 6;
        ne_doip_node_diag_data_request(diag_data_request);
        free(diag_data_request);
        NE_DOIP_PRINT("client send diagnostic data is end...\n");
    }
}

void diag_confirm_callback_3(ne_doip_diag_data_confirm_t *diag_data_confirm)
{
    NE_DOIP_PRINT("diag_confirm_callback_3 is called... result code is [%d]\n", diag_data_confirm->result);
    NE_DOIP_PRINT("logical_source_address is [%04X]\n", diag_data_confirm->logical_source_address);
    NE_DOIP_PRINT("logical_target_address is [%04X]\n", diag_data_confirm->logical_target_address);
    NE_DOIP_PRINT("TAType is [%u]\n", diag_data_confirm->ta_type);
}

void user_conf_callback(uint16_t logical_souce_address, uint16_t logical_target_address)
{
    NE_DOIP_PRINT("user_conf_callback is enter...\n");
    sleep(1);
    ne_doip_node_send_user_conf_result(logical_target_address, logical_souce_address, NE_DOIP_USER_CON_RESLUT_REJECT);
    // ne_doip_node_send_user_conf_result(logical_target_address, logical_souce_address, NE_DOIP_USER_CON_RESLUT_NO_CON);
    // ne_doip_node_send_user_conf_result(logical_target_address, logical_souce_address, NE_DOIP_USER_CON_RESLUT_PASS);
}

void equip_vehicle_identity_callback_func(ne_doip_vehicle_identity_resinfo_t* info)
{
    NE_DOIP_PRINT("equip_vehicle_identity_callback_func is enter...\n");
    *my_identity_resinfo = *info;
    printf("ne_doip_vehicle_identity_resinfo_t vin is ");
    uint8_t i;
    for (i = 0; i < NE_DOIP_VIN_SIZE; ++i) {
        printf("%02X", (unsigned char)my_identity_resinfo->vin[i]);
    }
    printf("\n");
    uint8_t j;
    printf("ne_doip_vehicle_identity_resinfo_t eid is ");
    for (j = 0; j < NE_DOIP_EID_SIZE; ++j) {
        printf("%02X", (unsigned char)my_identity_resinfo->eid[j]);
    }
    printf("\n");
    uint8_t q;
    printf("ne_doip_vehicle_identity_resinfo_t gid is ");
    for (q = 0; q < NE_DOIP_GID_SIZE; ++q) {
        printf("%02X", (unsigned char)my_identity_resinfo->gid[q]);
    }
    printf("\n");
    NE_DOIP_PRINT("ne_doip_vehicle_identity_resinfo_t logical_address is %04X\n", my_identity_resinfo->logical_address);

    // ne_doip_equip_routing_active(equip_logical_address, 0, 0, my_identity_resinfo->eid);
    ne_doip_equip_routing_active(equip_logical_address, 0, 0, NULL);
}

void equip_routing_active_callback_func(ne_doip_routing_active_resinfo_t* info)
{
    NE_DOIP_PRINT("equip_routing_active_callback_func is enter...\n");
    *my_active_resinfo = *info;
    NE_DOIP_PRINT("ne_doip_routing_active_resinfo_t logical_address is %04X \n", my_active_resinfo->entity_logical_address);
    NE_DOIP_PRINT("ne_doip_routing_active_resinfo_t routing_active_res_code is %d \n", my_active_resinfo->routing_active_res_code);

    char send_data[] = {0x11, 0x22, 0x33, 0x33, 0x22, 0x11};
    // ne_doip_equip_diagnostic(my_active_resinfo->entity_logical_address, NE_DOIP_TA_TYPE_PHYSICAL, send_data, sizeof send_data);
    ne_doip_equip_diagnostic(0x0130, NE_DOIP_TA_TYPE_PHYSICAL, send_data, sizeof send_data);
}

void equip_entity_status_callback_func(ne_doip_entity_status_info_t* info)
{
    NE_DOIP_PRINT("equip_entity_status_callback_func is enter...\n");
    *my_entity_status_info = *info;
    NE_DOIP_PRINT("ne_doip_entity_status_info_t node_type is %d \n", my_entity_status_info->node_type);
    NE_DOIP_PRINT("ne_doip_entity_status_info_t MCTS is %d \n", my_entity_status_info->MCTS);
    NE_DOIP_PRINT("ne_doip_entity_status_info_t NCTS is %d \n", my_entity_status_info->NCTS);
    NE_DOIP_PRINT("ne_doip_entity_status_info_t MDS is %d \n", my_entity_status_info->MDS);

    ne_doip_equip_power_mode_req(my_identity_resinfo->eid);
}

void equip_power_mode_callback_func(uint16_t power_mode)
{
    NE_DOIP_PRINT("equip_power_mode_callback_func is enter...\n");
    NE_DOIP_PRINT("power_mode is %d \n", power_mode);
}

void equip_diagnostic_pack_callback_func(uint16_t logical_target_address, uint8_t positive_ack_code)
{
    NE_DOIP_PRINT("equip_diagnostic_pack_callback_func is enter...\n");
}

void equip_diagnostic_nack_callback_func(uint16_t logical_target_address, uint8_t negative_ack_code)
{
    NE_DOIP_PRINT("equip_diagnostic_nack_callback_func is enter...\n");
}

void equip_diagnostic_callback_func(uint16_t logical_target_address, unsigned char* data, uint32_t length)
{
    NE_DOIP_PRINT("equip_diagnostic_callback_func is enter...\n");
    NE_DOIP_PRINT("equipment logical_target_address is [%04X]\n", logical_target_address);

    printf("recv data is ");
    uint32_t i;
    for (i = 0; i < length; ++i) {
        printf("%02X", (unsigned char)data[i]);
    }
    printf("\n");

    ne_doip_equip_entity_status_req(my_identity_resinfo->eid);
}


int main()
{
    ne_doip_node_callback_register_t callback_register_1;
    memset(&callback_register_1, 0, sizeof callback_register_1);
    callback_register_1.indication_cb = diag_indication_callback_1;
    callback_register_1.confirm_cb = diag_confirm_callback_1;
    ne_doip_node_create(0x0130, NE_DOIP_INSTANCE_TYPE_ENTITY, callback_register_1);

    // sleep(3);
    // ne_doip_node_callback_register_t callback_register_2;
    // memset(&callback_register_2, 0, sizeof callback_register_2);
    // callback_register_2.indcation_cb = diag_indication_callback_2;
    // callback_register_2.confirm_cb = diag_confirm_callback_2;
    // ne_doip_node_create(0x0202, NE_DOIP_INSTANCE_TYPE_ECU, callback_register_2);

    // sleep(3);
    // ne_doip_node_callback_register_t callback_register_3;
    // memset(&callback_register_3, 0, sizeof callback_register_3);
    // callback_register_3.indcation_cb = diag_indication_callback_3;
    // callback_register_3.confirm_cb = diag_confirm_callback_3;
    // ne_doip_node_create(0x0203, NE_DOIP_INSTANCE_TYPE_ECU, callback_register_3);





    // sleep(5);

    // my_identity_resinfo = malloc(sizeof *my_identity_resinfo);
    // my_active_resinfo = malloc(sizeof *my_active_resinfo);
    // my_entity_status_info = malloc(sizeof *my_entity_status_info);

    // ne_doip_equip_callback_register_t equip_callback_register;
    // equip_callback_register.vehicle_identity_cb = equip_vehicle_identity_callback_func;
    // equip_callback_register.routing_active_cb = equip_routing_active_callback_func;
    // equip_callback_register.entity_status_cb = equip_entity_status_callback_func;
    // equip_callback_register.power_mode_cb = equip_power_mode_callback_func;
    // equip_callback_register.diag_pack_cb = equip_diagnostic_pack_callback_func;
    // equip_callback_register.diag_nack_cb = equip_diagnostic_nack_callback_func;
    // equip_callback_register.diagnostic_cb = equip_diagnostic_callback_func;
    // ne_doip_equip_create(equip_logical_address, equip_callback_register);


    // sleep(3);

    // ne_doip_equip_identity(NULL, 0);

    sleep(1000000);

    ne_doip_node_destroy(0x0201);
    ne_doip_node_destroy(0x0202);
    ne_doip_node_destroy(0x0203);

    exit(0);
}
