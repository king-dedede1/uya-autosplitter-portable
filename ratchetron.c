#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "ratchetron.h"

#define RATCHETRON_PORT 9671
int LOCAL_PORT = 4000;

int tcp_sock;
int udp_sock;

struct sockaddr_in server_addr, host_addr;

int game_pid;

typedef struct _subinfo_t {
    int size;
    void *addr;
} subinfo_t;

subinfo_t subs_table[32];

int reverseByteOrder(int w)
{
    int k = 0;
    k |= (w & 0x000000FF) << 24;
    k |= (w & 0x0000FF00) << 8;
    k |= (w & 0x00FF0000) >> 8;
    k |= (w & 0xFF000000) >> 24;
    return k;
}

int ratchetron_connect(const char *ip)
{
    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock == -1)
    {
        printf("Error: couldn't create TCP socket.");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(RATCHETRON_PORT);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(tcp_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        printf("Error: couldn't connect TCP socket.");
        return 1;
    }

    char connect_msg[6];
    recv(tcp_sock, connect_msg, 6, MSG_WAITALL);

    if (connect_msg[0] != 1 || connect_msg[5] < 2)
    {
        printf("Error: server returned invalid connection message.");
        return 1;
    }
    printf("Connected to server (%s)\n", ip);

    char enable_debug = 0xD;
    send(tcp_sock, &enable_debug, 1, 0);

    // get game PID - command 0x3
    char get_pid = 3;
    send(tcp_sock, &get_pid, 1, 0);

    char pid_buffer[64];
    recv(tcp_sock, pid_buffer, 64, 0);
    memcpy(&game_pid, pid_buffer + 8, 4);

    printf("Game PID: 0x%x\n", reverseByteOrder(game_pid));

    //open UDP data channel
    // create UDP socket on local port X
    // send command with 0x9 [X]
    udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_sock == -1)
    {
        printf("Error: couldn't create UDP socket.\n");
        return 1;
    }

    host_addr.sin_family = AF_INET;
    host_addr.sin_addr.s_addr = INADDR_ANY;

    for (;LOCAL_PORT < 30000; LOCAL_PORT ++)
    {
        host_addr.sin_port = htons(LOCAL_PORT);
        if (bind(udp_sock, (const struct sockaddr*)&host_addr, sizeof(host_addr)) < 0)
        {
            printf("Warning: tried to bind UDP socket at port %d and couldn't; trying again...\n",LOCAL_PORT);
        }
        else
        {
            printf("Bound UDP socket at port %d\n", LOCAL_PORT);
            break;
        }

    }

    char open_udp[5];
    open_udp[0] = 9;
    int udp_port = reverseByteOrder(LOCAL_PORT);
    memcpy(open_udp+1, &udp_port, 4);
    send(tcp_sock, &open_udp, 5, 0);

    unsigned char udp_return_code;
    recv(tcp_sock, &udp_return_code, 1, 0);

    if (udp_return_code != 128)
    {
        printf("Error: server error enabling UDP stream (%d)\n", udp_return_code);
        return 1;
    }
    return 0;
}

void ratchetron_disconnect()
{
    close(udp_sock);
    close(tcp_sock);
}

subid_t ratchetron_listen(int game_addr, int size, void *local_addr)
{
    char cmd[14 + size];
    cmd[0] = 10;

    int _addr = reverseByteOrder(game_addr);
    int _size = reverseByteOrder(size);

    // no need to reverse game_pid, it was never reversed in the first place
    memcpy(cmd + 1, &game_pid, 4);
    memcpy(cmd + 5, &_addr, 4);
    memcpy(cmd + 9, &_size, 4);
    cmd[13] = 2;
    memset(cmd + 14, 0, size);

    send(tcp_sock, cmd, 14 + size, 0);

    int sub_id;
    recv(tcp_sock, &sub_id, 4, 0);
    sub_id = reverseByteOrder(sub_id);
    
    subs_table[sub_id].addr = local_addr;
    subs_table[sub_id].size = size;

    return sub_id;
}

void ratchetron_release(subid_t id)
{
    char cmd[5];
    cmd[0] = 0xC;
    int _id = reverseByteOrder(id);
    memcpy(cmd + 1 , &_id, 4);

    send(tcp_sock, cmd, 5, 0);

    char result;
    recv(tcp_sock, &result, 1, 0);

    // we're ignoring the results because yolo
}

void ratchetron_read_events()
{
    char message[64];
    int received = recvfrom(udp_sock, message, 64, 0, 0, 0);
    char command = message[0];
    if (command == 6)
    {
        subid_t id;
        memcpy(&id, message + 1, 4);
        id = reverseByteOrder(id);
        memcpy(subs_table[id].addr, message + 13, subs_table[id].size);

        if (subs_table[id].size == 4)
        {
            int *v = subs_table[id].addr;
            *v = reverseByteOrder(*v);
        }
    }
    else if (command == 8)
    {
        // we're not freezing anything
    }
    else
    {
        printf("Server says: %s",message);
    }
}