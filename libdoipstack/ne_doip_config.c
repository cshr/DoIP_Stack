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
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <errno.h>
#include "ne_doip_config.h"
#include "ne_doip_util.h"
#include "ne_doip_version.h"

#define EPSION 1e-1

char*
ne_doip_strim(char *str) // Remove the leading and trailing Spaces
{
    char *end, *sp, *ep;
    int len;
    sp = str;
    end = str + strlen(str) - 1;
    ep = end;

    while (sp <= end && isspace(*sp)) {
        sp++;
    }
    while (ep >= sp && isspace(*ep)) {
        ep--;
    }
    len = (ep < sp) ? 0:(ep-sp) + 1;
    sp[len] = '\0';
    return sp;
}


char*
ne_doip_delquotes(char *str) // Remove the leading and trailing "
{
    char *p = str;
    int i = 0;
    while (*p) {
        if (*p != '"') {
            str[i++] = *p;
        }
        p++;
    }
    str[i] = '\0';
    return str;
}

int
ne_doip_delstatement(char *temp, uint8_t *annotate) // Remove the null line and annotation
{
    int tempct = strlen(temp);
    if (tempct == 0) {
        return -1;
    }

    if ((temp[0] == '<') && (temp[1] == '!') && (temp[2] == '-') && (temp[3] == '-')) {
        (*annotate)++;
    }

    if ((temp[strlen(temp) - 1] == '>') && (temp[tempct - 2] == '-') && (temp[tempct - 3] == '-')) {
        (*annotate)++;
    }

    if ((*annotate) != 0) {
        if ((*annotate) == 2) {
            (*annotate) = 0;
        }
        return -1;
    }
    return 0;

}

int
ne_doip_bracket_ismatch(char *xmlstr)
{
    uint32_t i;
    int leftbracket = 0;
    int rightbracket = 0;
    int slashlabel = 0;
    for (i = 0; i < strlen(xmlstr); i++) {
        if (xmlstr[i] == '<') {
            leftbracket++;
        }
        else if (xmlstr[i] == '>') {
            if (leftbracket > rightbracket) {
                rightbracket++;
            }
            else {
                NE_DOIP_PRINT("[error] miss left bracket\n");
                return -1;
            }
        }
        else if (xmlstr[i] == '/') {
            if (xmlstr[i-1] != '<') {
                NE_DOIP_PRINT("[error] black slash is valid\n");
                return -1;
            }
            slashlabel++;
        }
    }
    if (rightbracket != leftbracket) {
        NE_DOIP_PRINT("[error] miss right bracket\n");
        return -1;
    }
    if (leftbracket != slashlabel * 2) {
        NE_DOIP_PRINT("[error] miss slash label\n");
        return -1;
    }
    return 0;
}

int
ne_doip_ismore_twotimes(char* temp)
{
    uint32_t m;
    int leftbracket = 0;
    int rigthbracket = 0;
    for (m = 0; m < strlen(temp); m++) {
        if (temp[m] == '<') {
            leftbracket++;
        }
        else if (temp[m] == '>') {
            rigthbracket++;
        }
    }
    if (leftbracket > 2) {
        NE_DOIP_PRINT("[error] xml leftbracket is more 2\n");
        return -1;
    }
    else if (rigthbracket > 2) {
        NE_DOIP_PRINT("[error] xml rigthbracket is more 2\n");
        return -1;
    }
    return 0;

}

int
ne_doip_check_xml_valid(const char *path)
{
    FILE* fp = fopen(path, "r");
    if (NULL == fp) {
        NE_DOIP_PRINT("open config file is failed![%d] [%s]\n", errno, strerror(errno));
        return -1;
    }

    struct stat st;
    stat(path, &st);
    char xmlstr[st.st_size]; // xml string
    char line[256]; // Receive the number of characters per line of the file
    char labstr[128][32]; // save tabel value
    uint8_t labcount = 0;
    uint8_t statementvalid = 0;
    uint8_t annotate = 0;
    memset(labstr, 0, sizeof *labstr);
    memset(xmlstr, 0, sizeof *xmlstr);
    while (fgets(line, sizeof(line), fp)) {
        char*temp = ne_doip_strim(line);
        int tempct = strlen(temp);
        int res = ne_doip_delstatement(temp, &annotate);
        if (res == -1) {
            continue;
        }
        if (strstr(temp, "<?") != NULL) {
            statementvalid = 1;
            continue;
        }
        else if (temp[0] != '<') {
            NE_DOIP_PRINT("[error] xml headbracket is valid\n");
            fclose(fp);
            return -1;
        }
        else if (temp[tempct - 1] != '>') {

            NE_DOIP_PRINT("[error] xml endbracket is valid\n");
            fclose(fp);
            return -1;
        }
        res = ne_doip_ismore_twotimes(temp);
        if (res == -1) {
            fclose(fp);
            return -1;
        }

        char *pLeft = strchr(temp, '<');
        char *pLeft1 = strrchr(temp, '<');
        char *pright1 = strrchr(temp, '>');
        char *pright = strchr(temp, '>');
        if (!strcmp(pLeft, pLeft1)) {
            if (pLeft[1] != '/') {
                int length = 0;
                char *pattribute = strchr(temp, ' ');
                if (pattribute != NULL) {
                    length = (long int)pattribute - (long int)pLeft - 1;
                }
                else {
                    length = (long int)pright - (long int)pLeft - 1;
                }

                if (length < 0) {
                    fclose(fp);
                    return -1;
                }

                char value[length + 1];
                value[length] = '\0';
                strncpy(value, pLeft + 1, length);
                strcpy(labstr[labcount], value);
                labcount++;

            }
            else {
                int length = (long int)pright - (long int)(pLeft + 1) - 1;
                char value[length + 1];
                value[length] = '\0';
                strncpy(value, pLeft + 2, length);
                int j;
                int flag = 0;
                for (j = 0; j < labcount; j++) {
                    if (!strcmp(labstr[j], value)) {
                        strcpy(labstr[j], " ");
                        flag = 1;
                    }
                }
                if (flag == 0) {
                    NE_DOIP_PRINT("[error]  tabel does not symmetry.[%s] \n", value);
                    fclose(fp);
                    return -1;
                }
            }
        }
        else {
            if (pLeft1[1] != '/') {
                NE_DOIP_PRINT("[error] miss left bracket\n");
                fclose(fp);
                return -1;
            }

            int length = (long int)pright - (long int)pLeft-1;

            if (length < 0) {
                fclose(fp);
                return -1;
            }
            char value[length + 1];
            value[length] = '\0';
            strncpy(value, pLeft+1, length);

            int length1 = (long int)pright1 - (long int)(pLeft1 + 1) - 1;
            if (length1 < 0) {
                fclose(fp);
                return -1;
            }
            char value1[length1 + 1];
            value1[length1] = '\0';
            strncpy(value1, pLeft1 + 2, length1);
            if (strcmp(value, value1)) {
                NE_DOIP_PRINT("[error]  tabel does not symmetry.[%s] [%s]\n", value, value1);
                fclose(fp);
                return -1;
            }
        }
        strcat(xmlstr, temp);
    }
    int res = ne_doip_bracket_ismatch(xmlstr);
    if (res == -1) {
        fclose(fp);
        return -1;
    }
    if (statementvalid == 0) {
        NE_DOIP_PRINT("[error] xml statement is valid\n");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

ne_doip_config_t*
ne_doip_load_config(const char *path)
{
    int result = ne_doip_check_xml_valid(path);
    if (result == -1) {
        NE_DOIP_PRINT("check xml is invalid!\n");
        return NULL;
    }

    ne_doip_config_t* config = (ne_doip_config_t*)malloc(sizeof *config);
    memset(config, 0, sizeof *config);
    // init routing table list
    ne_doip_routing_table_t *table = NULL;
    table = malloc(sizeof *table);
    memset(table, 0, sizeof *table);
    ne_doip_list_init((ne_doip_list_t *)table);
    config->routing_list = (ne_doip_list_t *)table;

    // init function group list
    ne_doip_func_group_t *func_group = NULL;
    func_group = malloc(sizeof *func_group);
    memset(func_group, 0, sizeof *func_group);
    ne_doip_list_init((ne_doip_list_t *)func_group);
    config->func_group_list = (ne_doip_list_t *)func_group;

    // init net source list
    ne_doip_net_source_t *net_source = NULL;
    net_source = malloc(sizeof *net_source);
    memset(net_source, 0, sizeof *net_source);
    ne_doip_list_init((ne_doip_list_t *)net_source);
    config->net_source_list = (ne_doip_list_t *)net_source;

    FILE* fp = fopen(path, "r");
    if (NULL == fp) {
        NE_DOIP_PRINT("[error]open config file is failed!\n");
        free(config);
        free(table);
        return NULL;
    }

    char line[256]; // Receive the number of characters per line of the file
    uint32_t group_count = 0;
    uint8_t ecuid_count = 0;
    uint8_t ecuid_index = 0;
    char entity_type[16] = { 0 };
    char* ecu_ip = NULL;
    uint8_t ecu_flag = 0;
    uint8_t ecu_flag1 = 0;
    uint8_t annotate1 = 0;
    uint8_t labelflag = 0;
    uint8_t net_source_fd = 0;
    uint8_t net_source_fd1 = 0;
    char source_type[8] = { 0 };
    char ifname[8] = { 0 };
    uint8_t nodeflag = 0;
    uint8_t entityflag = 0;
    uint16_t log_addr_arr[50] = { 0 };
    uint8_t entity_typeflag = 0;
    uint8_t gatewayflag = 0;
    uint8_t funcflag = 0;

    while (fgets(line, sizeof(line), fp)) {
        char*after = ne_doip_strim(line);
        int res = ne_doip_delstatement(after, &annotate1);
        if ((res == -1) || strstr(after, "<?")) {
            continue;
        }

        char *pLeft = strchr(after, '<');
        char *pLeft1 = strrchr(after, '<');
        char *pright = strchr(after, '>');

        int keylength = (long int)pright - (long int)pLeft - 1;
        char key[keylength + 1];
        key[keylength] = '\0';

        strncpy(key, after + 1, keylength);

        if (!strcmp(pLeft, pLeft1)) {
            if (after[1] != '/') {
                char *pattribute = strrchr(key, '=');
                if (pattribute != NULL) {
                    int attrlength = (long int)pattribute - (long int)key;
                    char front[attrlength + 1];
                    front[attrlength] = '\0';
                    strncpy(front, key, attrlength);
                    char*tempfront = ne_doip_strim(front);
                    if (0 == strcmp(tempfront, "entity ID")) {
                        char*temp1 = ne_doip_delquotes(pattribute + 1);
                        ecuid_count = atoi(temp1);
                        labelflag++;
                        if (ecuid_index != ecuid_count) {
                            labelflag = 0;
                            continue;
                        }
                        ecuid_index++;
                        continue;
                    }
                    else if (0 == strcmp(tempfront, "func_group ID")) {
                        char*temp = ne_doip_delquotes(pattribute + 1);
                        char*str;
                        group_count = strtol(temp, &str, 16);
                        funcflag++;
                        continue;
                    }
                }
                else {
                    if (0 == strcmp(key, "net_source")) {
                        net_source_fd++;
                        net_source_fd1++;
                        continue;
                    }
                    else if (0 == strcmp(key, "entity")) {
                        entityflag++;
                        continue;
                    }
                }
            }
            else {
                if (0 == strcmp(key, "/entity")) {
                    if (entityflag != 0) {
                        entityflag = 0;
                        continue;
                    }
                    if ((ecu_ip != NULL) && (strlen(ecu_ip) != 0) && (log_addr_arr != NULL)
                        && (strlen(entity_type) != 0)) {
                        if (nodeflag && ecu_flag >= 3) {
                            ecu_flag1 = 1;
                        }

                        int i;
                        for (i = 0; i < ecu_flag1; i++) {
                            ne_doip_routing_table_t* routing_table = malloc(sizeof *routing_table);
                            memset(routing_table, 0, sizeof *routing_table);

                            strcpy(routing_table->entity_type, entity_type);
                            routing_table->entity_logical_address = log_addr_arr[i];
                            routing_table->ip = malloc(strlen(ecu_ip) + 1);
                            strncpy(routing_table->ip, ecu_ip, strlen(ecu_ip) + 1);
                            ne_doip_list_insert(config->routing_list->prev, (ne_doip_list_t *)routing_table);
                        }
                    }

                    labelflag = 0;
                    entityflag = 0;
                    entity_typeflag = 0;
                    gatewayflag = 0;
                    ecu_flag = 0;
                    ecu_flag1 = 0;
                    memset(entity_type, 0, 16);
                    memset(log_addr_arr, 0, 50 * sizeof(uint16_t));
                    if (ecu_ip != NULL) {
                        free(ecu_ip);
                        ecu_ip = NULL;
                    }
                    continue;
                }
                else if (0 == strcmp(key, "/net_source")) {
                    if ((source_type != NULL) && (strlen(source_type) != 0) && (ifname != NULL)
                        && (strlen(ifname) != 0) && (ecu_flag == 2)) {
                        ne_doip_net_source_t *net_source = malloc(sizeof *net_source);
                        if (net_source != NULL) {
                            memset(net_source, 0, sizeof *net_source);
                            net_source->priority = net_source_fd1;
                            strcpy(net_source->source_type, source_type);
                            strcpy(net_source->ifname, ifname);
                            ne_doip_list_insert(config->net_source_list->prev, (ne_doip_list_t *)net_source);
                            memset(source_type, 0, 8);
                            memset(ifname, 0, 8);
                        }
                    }
                    net_source_fd = 0;
                    ecu_flag = 0;
                    continue;
                }
                else if (0 == strcmp(key, "/func_group")) {
                    if ((log_addr_arr != NULL) && (ecu_flag1 > 0) && (group_count > 0)) {
                        ne_doip_func_group_t* func = malloc(sizeof *func);
                        memset(func, 0, sizeof *func);
                        
                        func->group_member_num = ecu_flag1;
                        func->group_address = group_count;
                        int i;
                        for (i = 0; i < ecu_flag1; i++) {
                            func->logical_address_array[i] = log_addr_arr[i];
                        }
                        ne_doip_list_insert(config->func_group_list->prev, (ne_doip_list_t *)func);
                        memset(log_addr_arr, 0, 50 * sizeof(uint16_t));
                        ecu_flag = 0;
                        ecu_flag1 = 0;
                        funcflag = 0;
                    }
                continue;
                }
            }
        }
        else {
            int valuelength = (long int)pLeft1 - (long int)pright - 1;
            char value[valuelength + 1];
            value[valuelength] = '\0';
            strncpy(value, pright + 1, valuelength);
            if (!strcmp(key, "stack_version")) {

                if (strcmp(value, NE_DOIP_CONFIG_VERSION)) {
                    NE_DOIP_PRINT("[error] Version is mismatch [%s]\n", value);
                    free(config);
                    fclose(fp);
                    return NULL;
                }
                continue;
            }
            else if (!strcmp(key, "net_type")) {
                if (0 == strcmp(value, "IPv4")) {
                    config->net_type = NE_DOIP_NET_TYPE_IPV4;
                }
                else if (0 == strcmp(value, "IPv6")) {
                    config->net_type = NE_DOIP_NET_TYPE_IPV6;
                }
                else {
                    NE_DOIP_PRINT("[error] get net_type is failed!\n");
                }
                continue;
            }
            else if (!strcmp(key, "entity_type") && (entityflag == 1)) {
                if (0 == strcmp(value, "edge_gateway")) {
                    config->entity_type = NE_DOIP_ENTITY_TYPE_EDGE_GATEWAY;
                }
                else if (0 == strcmp(value, "gateway")) {
                    config->entity_type = NE_DOIP_ENTITY_TYPE_GATEWAY;
                }
                else if (0 == strcmp(value, "node")) {
                    config->entity_type = NE_DOIP_ENTITY_TYPE_NODE;
                }
                else {
                    NE_DOIP_PRINT("[error] entity_type of config is invalid.\n");
                }
                continue;
            }
            else if (!strcmp(key, "MCTS")) {
                int mcts = atoi(value);
                if (mcts > 0 && mcts <= 255) {
                    config->mcts = (uint8_t)mcts;
                }
                else {
                    NE_DOIP_PRINT("[error] MCTS is %d\n", mcts);
                }
                continue;
            }
            else if (!strcmp(key, "MDS")) {
                int mds = atoi(value);
                if ((value[0] == '-') || 0 == mds) {
                    NE_DOIP_PRINT("[error] MDS is %d\n", mds);
                }
                else {
                    config->mds = mds;
                }
                continue;
            }
            else if (!strcmp(key, "protocol_version")) {
                if (strlen(value) != 2) {
                    NE_DOIP_PRINT("[error] protocol_version  length is not 2.\n");
                }
                else {
                    char* str;
                    config->protocol_version = strtol(value, &str, 16);
                }
                continue;
            }
            else if (!strcmp(key, "egw_control")) {
                if (0 == strcmp(value, "TRUE") || 0 == strcmp(value, "true")) {
                    config->egw_control = NE_DOIP_TRUE;
                }
                continue;
            }
            else if (!strcmp(key, "tester_sa")) {
                if (strlen(value) != 4) {
                    NE_DOIP_PRINT("[error] tester_sa is invalid.\n");
                }
                else {
                    char* str;
                    config->tester_sa = strtol(value, &str, 16);
                }
                continue;
            }
            else if (!strcmp(key, "A_DoIP_Announce_Interval")) {
                int announce = atoi(value);
                config->announce_interval_time = (uint32_t)announce;
                continue;
            }
            else if (!strcmp(key, "T_TCP_General_Inactivity")) {
                int general = atoi(value);
                config->general_inactivity_time = (uint32_t)general;
                continue;
            }
            else if (!strcmp(key, "T_TCP_Initial_Inactivity")) {
                int initial = atoi(value);
                config->initial_inactivity_time = (uint32_t)initial;
                continue;
            }
            else if (!strcmp(key, "T_TCP_Alive_Check")) {
                int alive = atoi(value);
                config->alive_check_time = (uint32_t)alive;
                continue;
            }
            else if (!strcmp(key, "functianal_la")) {
                if (strlen(value) != 4) {
                    NE_DOIP_PRINT("[error] functianal_la is invalid.\n");
                }
                else {
                    char* str;
                    uint16_t logical_address = strtol(value, &str, 16);
                    if (NE_DOIP_WWH_OBD_FUNCTIANAL_ADDRESS == logical_address
                        || (NE_DOIP_OEM_UPPER_FUNCTIANAL_ADDRESS >= logical_address
                        && NE_DOIP_OEM_LOWER_FUNCTIANAL_ADDRESS <= logical_address)) {
                            config->functianal_la = logical_address;
                    }
                }
                continue;
            }
            else {
                if (labelflag == 1) {
                   if (!strcmp(key, "entity_type")) {
                        if (0 == strcmp(value, "node")) {
                            nodeflag = 1;
                        }
                        else if (0 == strcmp(value, "edge_gateway")) {
                            entity_typeflag++;
                        }
                        else if (0 == strcmp(value, "gateway")) {
                            gatewayflag++;
                        }
                        else {
                            NE_DOIP_PRINT("[error]entity_type is invalid!\n");
                        }
                        strcpy(entity_type, value);
                        ecu_flag++;
                   }
                   else if (nodeflag) {
                       if (!strcmp(key, "logical_address")) {
                            char* str;
                            log_addr_arr[0] = strtol(value, &str, 16);
                            ecu_flag++;
                        }
                        else if (!strcmp(key, "ip")) {
                            ecu_ip = malloc(strlen(value) + 1);
                            strncpy(ecu_ip, value, strlen(value) + 1);
                            ecu_flag++;
                        }

                        else {
                            NE_DOIP_PRINT("[error]Other elements\n");
                        }
                        continue;
                    }
                    else if ((gatewayflag || entity_typeflag)) {
                       if (!strcmp(key, "logical_address")) {
                            char* str;
                            log_addr_arr[ecu_flag1] = strtol(value, &str, 16);

                            ecu_flag1++;
                        }
                        else if (!strcmp(key, "ip")) {
                            if (entity_typeflag) {
                                if (strcmp(value, "127.0.0.1")) {
                                    entity_typeflag = 0;
                                    memset(log_addr_arr, 0, 50 * sizeof(uint16_t));
                                    continue;
                                }
                            }
                            ecu_ip = malloc(strlen(value) + 1);
                            strncpy(ecu_ip, value, strlen(value) + 1);

                        }

                        else {
                            NE_DOIP_PRINT("[error]Other elements\n");
                        }
                        continue;
                    }
                }
                else if (net_source_fd == 1) {
                    if (!strcmp(key, "source_type")) {
                        strcpy(source_type, value);
                        ecu_flag++;
                    }
                    else if (!strcmp(key, "ifname")) {
                        strcpy(ifname, value);
                        ecu_flag++;
                    }
                    continue;
                }
                else if (funcflag == 1) {
                   if (!strcmp(key, "logical_address")) {
                        char* str;
                        log_addr_arr[ecu_flag1] = strtol(value, &str, 16);
                        ecu_flag1++;
                    }
                    continue;
                }

            }
        }
    }

    fclose(fp);

    return config;
}

void
ne_doip_unload_config(ne_doip_config_t*config)
{
    if (config == NULL) {
        return;
    }

    if (config->routing_list) {
        ne_doip_routing_table_t *rout;
        ne_doip_routing_table_t *tmp;
        ne_doip_list_for_each_safe(rout, tmp, config->routing_list, base) {
            if (rout->ip != NULL) {
                free(rout->ip);
            }

            ne_doip_list_remove((ne_doip_list_t*)rout);
            free(rout);
        }
        free(config->routing_list);
    }
    if (config->net_source_list) {
        ne_doip_net_source_t *source;
        ne_doip_net_source_t *tmp;
        ne_doip_list_for_each_safe(source, tmp, config->net_source_list, base) {
            ne_doip_list_remove((ne_doip_list_t*)source);
            free(source);
        }
        free(config->net_source_list);
    }
    if (config->func_group_list) {
        ne_doip_func_group_t *func;
        ne_doip_func_group_t *tmp;
        ne_doip_list_for_each_safe(func, tmp, config->func_group_list, base) {
            ne_doip_list_remove((ne_doip_list_t*)func);
            free(func);
        }
        free(config->func_group_list);
    }

    free(config);
}

ne_doip_net_source_t *
ne_doip_net_source_list_find_by_ifname(ne_doip_config_t*config, const char *ifname)
{
    ne_doip_net_source_t *net_source = NULL;
    ne_doip_list_for_each(net_source, config->net_source_list, base) {
        if (0 == strncmp(net_source->ifname, ifname, strlen(ifname))) {
            return net_source;
        }
    }
    return NULL;
}

ne_doip_net_source_t *
ne_doip_net_source_list_find_by_announce_wait_timeid(ne_doip_config_t *config, int timer_id)
{
    ne_doip_net_source_t *net_source = NULL;
    ne_doip_list_for_each(net_source, config->net_source_list, base) {
        if (net_source->announce_wait_timeid == timer_id) {
            return net_source;
        }
    }
    return NULL;
}

ne_doip_net_source_t *
ne_doip_net_source_list_find_by_announce_interval_timeid(ne_doip_config_t *config, int timer_id)
{
    ne_doip_net_source_t *net_source = NULL;
    ne_doip_list_for_each(net_source, config->net_source_list, base) {
        if (net_source->announce_interval_timeid == timer_id) {
            return net_source;
        }
    }
    return NULL;
}

uint8_t
ne_doip_is_functianal_address(ne_doip_config_t *config, uint16_t logical_address)
{
    ne_doip_func_group_t *func_group = NULL;
    ne_doip_list_for_each(func_group, config->func_group_list, base) {
        if (func_group->group_address == logical_address) {
            return NE_DOIP_TRUE;
        }
    }
    return NE_DOIP_FALSE;
}
/* EOF */