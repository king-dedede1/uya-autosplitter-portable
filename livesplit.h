#ifndef _LIVESPLIT_H
#define _LIVESPLIT_H

int livesplit_connect(void);
void livesplit_disconnect(void);
void livesplit_start(void);
void livesplit_split(void);
void livesplit_reset(void);
void livesplit_add_loading_time(void);

#endif