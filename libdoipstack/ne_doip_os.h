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
 * @file ne_doip_os.h
 * @brief 1. Provide a common interface 2. System API adaptation for multiple platforms
 */

#ifndef NE_DOIP_OS_H
#define NE_DOIP_OS_H

// Create an epoll and return a file descriptor
int ne_doip_os_epoll_create(void);

// Create a socket
int ne_doip_os_socket(int domain, int type, int protocol);

// Listening link
int ne_doip_os_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

// connect to server
int ne_doip_os_connect(int sockfd, struct sockaddr *addr, socklen_t addrlen);

// get ifname by ip address
void ne_doip_os_get_ifname(char* ifname, int fd, char *ip);

// get ip address by ifname
int ne_doip_os_get_ip(const char *ifname, char *ip, int iplen);

#endif // NE_DOIP_OS_H
/* EOF */