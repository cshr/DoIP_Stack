#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <pthread.h>
#include "../libdoipstack/ne_doip_util.h"

int
tcp_connect()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(13400);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    const int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt)) {
        close(fd);
        NE_DOIP_PRINT("setsockopt error code:%d, message:%s\n", errno, strerror(errno));
        return -1;
    }

    if (connect(fd, (struct sockaddr *) &addr, sizeof addr) < 0) {
        NE_DOIP_PRINT("connect error code:%d, message:%s\n", errno, strerror(errno));
        close(fd);
        return -1;
    }

    char buffout[] = {"test equipment request"};
    int num = write(fd, buffout, sizeof buffout);
    if (num != sizeof buffout) {
        NE_DOIP_PRINT("write error code:%d, message:%s\n", errno, strerror(errno));
    }

    char buffin[1024];

    while(1) {
        memset(buffin, 0, 1024);

        num = read(fd, buffin, 1024);
        if (num < 0) {
            NE_DOIP_PRINT("read error code:%d, message:%s\n", errno, strerror(errno));
        }
        else if (num == 0) {
            NE_DOIP_PRINT("read eof\n");
            break;
        }

        NE_DOIP_PRINT("read contents:%s\n", buffin);
    }



    return 0;
}

int main()
{
    tcp_connect();

    sleep(10000);

    return 0;
}
