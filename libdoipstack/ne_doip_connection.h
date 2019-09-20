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
 * @file ne_doip_connection.h
 * @brief Socket operation
 */

#ifndef NE_DOIP_CONNECTION_H
#define NE_DOIP_CONNECTION_H

#include <sys/socket.h>

#define NE_DOIP_MAX_BUFFER_SIZE     1500

typedef struct ne_doip_buffer
{
    char data[NE_DOIP_MAX_BUFFER_SIZE];
    unsigned int data_size;
}ne_doip_buffer_t;

typedef struct ne_doip_connection
{
    int fd;
    ne_doip_buffer_t in;
    ne_doip_buffer_t out;
}ne_doip_connection_t;


ne_doip_connection_t *ne_doip_connection_create(int fd);

void ne_doip_connection_destroy(ne_doip_connection_t *connection);

int ne_doip_connection_write(ne_doip_connection_t *connection);

int ne_doip_connection_write_raw(int fd, char* buffer, unsigned int buffer_size);

int ne_doip_connection_read(ne_doip_connection_t *connection);

int ne_doip_connection_sendto(ne_doip_connection_t *connection, struct sockaddr *addr, int addr_len);

int ne_doip_connection_recvfrom(ne_doip_connection_t *connection, struct sockaddr *addr, socklen_t *size);

#endif // NE_DOIP_CONNECTION_H
/* EOF */
