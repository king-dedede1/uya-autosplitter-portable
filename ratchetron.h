#ifndef _RATCHETRON_H
#define _RATCHETRON_H

typedef int subid_t;

int ratchetron_connect(const char *ip);
void ratchetron_disconnect();
subid_t ratchetron_listen(int game_addr, int size, void *local_addr);
void ratchetron_release(subid_t id);
void ratchetron_read_events();

#endif