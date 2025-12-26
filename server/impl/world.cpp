/* ----------------------------------------------------------------------------
 * Copyright (c) 2025 jhlayt
 * See LICENSE.txt for copyright and licensing details. (GPL3.0)
 * ----------------------------------------------------------------------------
 * */
#include "../world.h"
#include "../server.h"
#include "../../packets.h"
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

internal u32 food_count;
internal v2i food_pos[FOOD_MAX];
internal bool8 food_alive[FOOD_MAX];

typedef struct player player;
struct player
{
  CLIENT_ID _client_id;
  u32 _score;
  v2i _pos;
};
internal player players[MAX_CLIENTS];

void
player_update(CLIENT_ID client_id, v2i pos)
{
  for (u32 i = 0; i < MAX_CLIENTS; i++)
  {
    if (players[i]._client_id == client_id)
    {
      players[i]._pos = pos;
      return;
    }
  }
}
void
player_join(CLIENT_ID client_id)
{
  for (u32 i = 0; i < MAX_CLIENTS; i++)
  {
    if (players[i]._client_id == 0)
    {
      memset(&players[i], 0, sizeof(player));
      players[i]._client_id = client_id;
      return;
    }
  }
}
void
player_leave(CLIENT_ID client_id)
{
  for (u32 i = 0; i < MAX_CLIENTS; i++)
  {
    if (players[i]._client_id == client_id)
    {
      players[i]._client_id = 0;
      return;
    }
  }
  assert(FALSE && "_player_leave()_ player not found.");
}
bool8
spawn_food(v2i pos)
{
  for (u32 i = 0; i < FOOD_MAX; i++)
  {
    if (!food_alive[i])
    {
      food_alive[i] = TRUE;
      food_pos[i] = pos;
      food_count++;
      return (TRUE);
    }
  }
  return (FALSE);
}

void
world_update()
{
  for (u32 i = 0; i < MAX_CLIENTS; i++)
  {
    v2i ppos = players[i]._pos;
    if (players[i]._client_id != 0)
    {
      for (u32 j = 0; j < FOOD_MAX; j++)
      {
        if (food_alive[j])
        {
          v2i fpos = food_pos[j];
          if (abs(ppos._x - fpos._x) <= PLAYER_SIZE*2 &&
              abs(ppos._y - fpos._y) <= PLAYER_SIZE*2)
          {
            food_alive[j] = FALSE;
            food_count--;
            players[i]._score++;
          }
        }
      }
    }
  }


  world_state_packet world_state;
  world_state._header._type = WORLD_STATE;
  world_state._header._size = sizeof(world_state_packet);
  world_state._food_count = food_count;
  for (u32 i = 0; i < MAX_CLIENTS; i++)
  {
    world_state._client_ids[i] = players[i]._client_id;
    world_state._player_pos[i] = players[i]._pos;
    world_state._scores[i] = players[i]._score;
  }
  u32 iter = 0;
  for (u32 j = 0; j < FOOD_MAX; j++)
  {
    if (food_alive[j])
    {
      world_state._food_pos[iter++] = food_pos[j];
    }
  }
  send_packet_to_all_clients(&world_state, sizeof(world_state));

  if (rand() % 100 < 10)
  {
    spawn_food({(u32)rand() % WINDOW_WIDTH, (u32)rand() % WINDOW_HEIGHT});
  }
}


void
time_mark_start()
{
}



