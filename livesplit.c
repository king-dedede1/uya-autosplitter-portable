#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "livesplit.h"

int sock;
struct sockaddr_in ls_addr;

int livesplit_connect(void)
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        return 1;
    }

    memset(&ls_addr, 0, sizeof(ls_addr));
    ls_addr.sin_family = AF_INET;
    ls_addr.sin_port = htons(16834);
    ls_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(sock, (struct sockaddr*)&ls_addr, sizeof(ls_addr)))
    {
        return 1;
    }
    return 0;
}

void livesplit_disconnect(void)
{
    close(sock);
    sock = 0;
}

void livesplit_start(void)
{
    send(sock, "start\r\n", 7, 0);
    printf("Speedrun starting!\n");
}

void livesplit_split(void)
{
    send(sock, "split\r\n", 7, 0);
    printf("Split\n");
}

void livesplit_reset(void)
{
    send(sock, "reset\r\n", 7, 0);
}

void livesplit_add_loading_time(void)
{
    send(sock, "addloadingtimes 1\r\n", 19, 0);
    printf("Removed long load\n");
}
