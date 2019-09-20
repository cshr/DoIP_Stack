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

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "ne_doip_data_queue.h"
#include "ne_doip_util.h"


ne_doip_queue_manager_t * ne_doip_queue_init()
{
    ne_doip_queue_manager_t *mgr = malloc(sizeof(ne_doip_queue_manager_t));
    if (NULL == mgr) {
        return NULL;
    }
    memset(mgr, 0, sizeof(ne_doip_queue_manager_t));
    mgr->tail = NULL;
    mgr->head = NULL;
    NE_DOIP_PRINT("queue init is succeed !\n");
    return mgr;
}

int ne_doip_queue_deinit(ne_doip_queue_manager_t *mgr)
{
    if (NULL == mgr) {
        return -1;
    }
    ne_doip_routing_data_queue_t *free_data = mgr->head;
    while (mgr->head != NULL) {
        free_data = mgr->head->next;
        free(mgr->head->routing_data->data);
        free(mgr->head->routing_data);
        free(mgr->head);
        mgr->head = free_data;
    }
    free(mgr);
    NE_DOIP_PRINT("queue deinit is succeed !\n");
    return 0;
}

int ne_doip_queue_empty(ne_doip_queue_manager_t *mgr)
{
    return mgr->head == NULL ? 1 : 0;
}

int ne_doip_queue_size(ne_doip_queue_manager_t *mgr)
{
    int queue_data_size = 0;
    if (NULL == mgr) {
        return -1;
    }
    ne_doip_routing_data_queue_t *num = mgr->head;
    while (num != NULL) {
        queue_data_size += num->routing_data->data_size;
        num = num->next;
    }
    NE_DOIP_PRINT("queue data size is :[%d]\n", queue_data_size);
    return queue_data_size;

}

int ne_doip_inser_queue(ne_doip_queue_manager_t *mgr, ne_doip_routing_data_t* data)
{
    if (NULL == mgr) {
        return -1;
    }

    ne_doip_routing_data_queue_t *queue_node = malloc(sizeof(ne_doip_routing_data_queue_t));
    memset(queue_node, 0, sizeof(ne_doip_routing_data_queue_t));

    queue_node->routing_data = malloc(sizeof(ne_doip_routing_data_t));
    memset(queue_node->routing_data, 0, sizeof(ne_doip_routing_data_t));

    queue_node->routing_data->fd = data->fd;
    queue_node->routing_data->data = malloc(data->data_size);
    memset(queue_node->routing_data->data, 0, data->data_size);

    memcpy(queue_node->routing_data->data, data->data, data->data_size);
    queue_node->routing_data->data_size = data->data_size;
    queue_node->next = NULL;
    // memcpy(queue_node->data, data, sizeof(ne_doip_routing_data_t));

    if (NULL == mgr->head) {
        mgr->head = mgr->tail = queue_node;
        NE_DOIP_PRINT("insert first data is succeed !\n");
        return 0;
    }

    mgr->tail->next = queue_node;
    mgr->tail = queue_node;
    NE_DOIP_PRINT("queue insert data is succeed !\n");
    return 0;
}

ne_doip_routing_data_t * ne_doip_front_queue(ne_doip_queue_manager_t *mgr)
{
    if (NULL == mgr || NULL == mgr->head) {
        return NULL;
    }
    ne_doip_routing_data_queue_t *node_data = mgr->head;
    mgr->head = mgr->head->next;

    ne_doip_routing_data_t *buff = malloc(sizeof(ne_doip_routing_data_t));
    memset(buff, 0, sizeof(ne_doip_routing_data_t));

    buff->fd = node_data->routing_data->fd;
    buff->data_size = node_data->routing_data->data_size;
    
    buff->data = malloc(node_data->routing_data->data_size);
    memset(buff->data, 0, node_data->routing_data->data_size);
    memcpy(buff->data, node_data->routing_data->data, node_data->routing_data->data_size);

    // memcpy(buff, node_data->data, sizeof(ne_doip_routing_data_t));

    free(node_data->routing_data->data);
    free(node_data->routing_data);
    free(node_data);
    NE_DOIP_PRINT("queue front data is succeed !\n");
    return buff;

}

int ne_doip_queue_clear(ne_doip_queue_manager_t *mgr, int flag_fd)
{
    NE_DOIP_PRINT("statr clean routing failed fd !\n");
    if (NULL == mgr) {
        return -1;
    }

    if (flag_fd <= 0) {
        NE_DOIP_PRINT("flag_fd <= 0 erroe\n");
        return -1;
    }
    ne_doip_routing_data_queue_t *select_queue = mgr->head;
    ne_doip_routing_data_queue_t *tmp = NULL;
    while (select_queue != NULL) {
        if (select_queue->routing_data->fd == flag_fd) {
            if (select_queue == mgr->head) {
                // head noed
                mgr->head = select_queue->next;
                free(select_queue->routing_data->data);
                free(select_queue->routing_data);
                free(select_queue);
                select_queue = mgr->head;
            }
            else {
                // non head node
                tmp = select_queue;
                select_queue = select_queue->next;
                free(tmp->routing_data->data);
                free(tmp->routing_data);
                free(tmp);
            }
        }
        else {
            select_queue = select_queue->next;
        } 
    }
    NE_DOIP_PRINT("routing failed clear queue fd is succeed !\n");
    return 0;
}