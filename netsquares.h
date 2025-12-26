/* ----------------------------------------------------------------------------
 * Copyright (c) 2025 jhlayt
 * See LICENSE.txt for copyright and licensing details. (GPL3.0)
 * ----------------------------------------------------------------------------
 * */
#ifndef NETSQUARES_H
#define NETSQUARES_H
#include <windows.h>
#include "packets.h"

u32 get_players(RECT *rects);
u32 get_world_rects(RECT *world_rects);
u32 get_scores(u32 *scores);
u32 get_client_ids(u32 *cids);
void move_player(v2f by);
void set_world_state(world_state_packet *packet);

#endif// NETSQUARES_H
