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
#include <unistd.h>
#include <errno.h>

#include "ne_doip_connection.h"
#include "ne_doip_util.h"

ne_doip_connection_t *
ne_doip_connection_create(int fd)
{
    ne_doip_connection_t *connection;

    connection = malloc(sizeof *connection);
    memset(connection, 0, sizeof *connection);
    connection->fd = fd;
    
    return connection;
}

void
ne_doip_connection_destroy(ne_doip_connection_t *connection)
{
    if (connection == NULL) {
        return;
    }

    shutdown(connection->fd, SHUT_RDWR);
    close(connection->fd);
}

int
ne_doip_connection_write(ne_doip_connection_t *connection)
{
    errno = 0;
    int num = write(connection->fd, connection->out.data, connection->out.data_size);
    if ((unsigned int)num != connection->out.data_size) {
        NE_DOIP_PRINT("ne_doip_connection_write code:%d, message:%s, fd:%d \n", errno, strerror(errno), connection->fd);
        NE_DOIP_PRINT("ne_doip_connection_write write count:%u,num:%d\n", connection->out.data_size, num);
    }
    NE_DOIP_PRINT("ne_doip_connection_write num:[%d], fd:[%d] \n", num, connection->fd);

    return num;
}

int
ne_doip_connection_write_raw(int fd, char* buffer, unsigned int buffer_size)
{
    errno = 0;
    int num = write(fd, buffer, buffer_size);
    if ((unsigned int)num != buffer_size) {
        NE_DOIP_PRINT("ne_doip_connection_write_raw code:%d, message:%s, fd:%d \n", errno, strerror(errno), fd);
        NE_DOIP_PRINT("ne_doip_connection_write_raw write count:%u,num:%d\n", buffer_size, num);
    }
    NE_DOIP_PRINT("ne_doip_connection_write_raw num:[%d], fd:[%d] \n", num, fd);

    return num;
}

int
ne_doip_connection_read(ne_doip_connection_t *connection)
{
    errno = 0;
    memset(&connection->in, 0, sizeof connection->in);

    int num = read(connection->fd, connection->in.data, sizeof connection->in.data);
    if (num < 0) {
        NE_DOIP_PRINT("ne_doip_connection_read num < 0 code:%d, message:%s, fd:%d \n", errno, strerror(errno), connection->fd);
    }
    else if (num == 0) {
         NE_DOIP_PRINT("ne_doip_connection_read num == 0 code:%d, message:%s, fd:%d \n", errno, strerror(errno), connection->fd);
    }
    connection->in.data_size = num;
    NE_DOIP_PRINT("ne_doip_connection_read num:[%d], fd:%d \n", num, connection->fd);

    return num;
}

int
ne_doip_connection_sendto(ne_doip_connection_t *connection, struct sockaddr *addr, int addr_len)
{
    errno = 0;
    int num = sendto(connection->fd, connection->out.data, connection->out.data_size, 0, addr, addr_len);
    if ((unsigned int)num != connection->out.data_size) {
        NE_DOIP_PRINT("ne_doip_connection_sendto code:%d, message:%s\n", errno, strerror(errno));
    }
    NE_DOIP_PRINT("ne_doip_connection_sendto count:%u,num:%d\n", connection->out.data_size, num);

    return num;
}

int
ne_doip_connection_recvfrom(ne_doip_connection_t *connection, struct sockaddr *addr, socklen_t *size)
{
    errno = 0;
    memset(&connection->in, 0, sizeof connection->in);

    int num = recvfrom(connection->fd, connection->in.data, sizeof connection->in.data, 0, addr, size);
    if (num < 0) {
        NE_DOIP_PRINT("ne_doip_connection_recvfrom num < 0 code:%d, message:%s\n", errno, strerror(errno));
    }
    else if (num == 0) {
        NE_DOIP_PRINT("ne_doip_connection_recvfrom num == 0 code:%d, message:%s\n", errno, strerror(errno));
    }
    connection->in.data_size = num;
    NE_DOIP_PRINT("ne_doip_connection_recvfrom num:[%d]\n", num);

    return num;
}
/* EOF */