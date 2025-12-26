/* ----------------------------------------------------------------------------
 * Copyright (c) 2025 jhlayt
 * See LICENSE.txt for copyright and licensing details. (GPL3.0)
 * ----------------------------------------------------------------------------
 * */
#ifndef SERVER_WORLD_H
#define SERVER_WORLD_H

#include "../defines.h"
bool8 spawn_food(v2i pos);
void world_update();

void player_update(CLIENT_ID client_id, v2i pos);
void player_join(CLIENT_ID client_id);
void player_leave(CLIENT_ID client_id);

#endif// SERVER_WORLD_H
