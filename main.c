#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "livesplit.h"
#include "ratchetron.h"

#define MAX_SPLIT_ROUTE_LEN 256

typedef struct {
    char destination;
    char planet;
    char loading_screen;
    int game_state;
    float nefarious_health_bar;
    int nefarious_phase;
    int chunk;
} game_vars;

// game state
game_vars current, old;

// autosplitter state
bool biobliterator = false;
bool ll_ignore[37];
char origin_planet = 0;
bool entered_ldf = false;

// livesplit state
bool timer_running = false;

// split route
char split_route[MAX_SPLIT_ROUTE_LEN];
int split_route_len;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Missing arguments, expected 2. First is ip address, second is path to USR\n");
        return 1;
    }

    //printf("hello\n");
    FILE *sr_file = fopen(argv[1], "r");
    if (sr_file == 0)
    {
        printf("Couldn't open split route file: %s\n", strerror(errno));
        return 1;
    }

    split_route_len = fread(split_route, 1, MAX_SPLIT_ROUTE_LEN, sr_file);
    fclose(sr_file);

    if (livesplit_connect())
    {
        printf("Failed to connect to LiveSplit server");
        return 1;
    }

    if (ratchetron_connect(argv[0]))
    {
        printf("Failed to connect to game server");
        return 1;
    }

    ratchetron_listen(0xEE9314 + 3, 1, &current.destination);
    ratchetron_listen(0xC1E438 + 3, 1, &current.planet);
    ratchetron_listen(0xD99114 + 3, 1, &current.loading_screen);
    ratchetron_listen(0xEE9334, 4, &current.game_state);
    ratchetron_listen(0xC4DF80, 4, &current.nefarious_health_bar);
    ratchetron_listen(0xDA50FC, 4, &current.nefarious_phase);
    ratchetron_listen(0xF08100, 4, &current.chunk);

    ll_ignore[20] = true;
    ll_ignore[26] = true;
    ll_ignore[27] = true;
    ll_ignore[28] = true;
    ll_ignore[29] = true;

    while (1)
    {

        memcpy(&old, &current, sizeof(game_vars));
        ratchetron_read_events();

        // update stuff
        if (current.planet != old.planet && current.destination != 0)
            origin_planet = old.planet;
        else if (current.planet == old.planet && current.destination == 0)
            origin_planet = current.planet;

        // check long loads I think
        if (current.loading_screen == 1 && old.loading_screen != 1
            && !ll_ignore[current.destination]
            && !ll_ignore[origin_planet])
            livesplit_add_loading_time();

        if (!biobliterator && current.game_state == 0 && current.planet == 20 
            && current.nefarious_phase % 2 == 1 && current.nefarious_health_bar == 1)
            biobliterator = true;

        // Make it reset on title screen instead of loading veldin
        // this will cause an issue in ng+ cats and 100%
        if (current.planet == -1 && old.planet != -1)
            livesplit_reset();

        if (current.planet == 1 && old.game_state == 6 && current.game_state == 0)
            livesplit_start();

        else 
        {
            // ldf entry split
            if (!entered_ldf && current.planet == 4
                && current.chunk == 1 && old.chunk != 1)
                livesplit_split();

            // biobliterator split
            else if (current.nefarious_health_bar == 0 && biobliterator && current.planet == 20)
            {
                livesplit_split();
                biobliterator = false;
            }
            
            // planet split
            else if (current.destination != old.destination && current.planet != current.destination
                && current.destination != 0 && current.planet != 0)
            {
                for (int i = 0; i < split_route_len; i += 2)
                {
                    if (split_route[i] == current.planet && split_route[i+1] == current.destination)
                    {
                        livesplit_split();
                        break;
                    }
                }
            }
        }
        
    }

    ratchetron_disconnect();
    livesplit_disconnect();
}