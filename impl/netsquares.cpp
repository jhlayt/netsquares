/* ----------------------------------------------------------------------------
 * Copyright (c) 2025 jhlayt
 * See LICENSE.txt for copyright and licensing details. (GPL3.0)
 * ----------------------------------------------------------------------------
 * */
#include "../defines.h"
#include "../win32.h"
#include "../netsquares.h"
#include "../client/client.h"

#include <stdio.h>
#include <windows.h>
#include <process.h>
#include <timeapi.h>

typedef struct player player;
struct player
{
  u32 _client_id;
  u32 _score;
  v2i _pos;
};

internal u32 food_count;
internal v2i food[FOOD_MAX];
internal bool8 food_alive[FOOD_MAX];
internal RECT temp[FOOD_MAX];

internal player players[MAX_CLIENTS]; //0 is this client's player.

internal void
update()
{
  client_state_packet pack;
  pack._header._type = CLIENT_STATE;
  pack._header._size = sizeof(pack);
  pack._player_pos = players[0]._pos;
  pack._client_id = players[0]._client_id;

  send_to_server((char*)&pack, sizeof(client_state_packet));
}

void
set_world_state(world_state_packet *packet)
{
  u32 cid = get_client_id();
  food_count = packet->_food_count;
  memcpy(food, packet->_food_pos, sizeof(v2i) * FOOD_MAX);
  memset(food_alive, 0, sizeof(bool8) * FOOD_MAX);
  u32 pid = 1;
  for (u32 i = 0; i < MAX_CLIENTS; i++)
  {
    if (packet->_client_ids[i] == cid)
    {
      players[0]._score = packet->_scores[i];
    }
    else
    {
      players[pid]._client_id = packet->_client_ids[i];
      players[pid]._score = packet->_scores[i];
      players[pid]._pos = packet->_player_pos[i];
      pid++;
    }
  }
  for (u32 i = 0; i < food_count; i++)
  {
    food_alive[i] = TRUE;
    v2i p = food[i];
    temp[i] = {(s64)(p._x - FOOD_SIZE*0.5f), 
      (s64)(p._y + FOOD_SIZE*0.5f),
      (s64)(p._x + FOOD_SIZE*0.5f),
      (s64)(p._y - FOOD_SIZE*0.5f)};
  }
}
u32
get_scores(u32 *scores)
{
  u32 count = 0;
  u32 j = 0;
  for (u32 i = 0; i < MAX_CLIENTS; i++)
  {
    if (players[i]._client_id != 0)
    {
      scores[i] = players[i]._score;
      count++;
    }
  }
  return (count);
}
u32
get_client_ids(u32 *cids)
{
  u32 count = 0;
  u32 j = 0;
  for (u32 i = 0; i < MAX_CLIENTS; i++)
  {
    if (players[i]._client_id != 0)
    {
      cids[i] = players[i]._client_id;
      count++;
    }
  }
  return (count);
}

u32
get_players(RECT *rects)
{
  u32 count = 0;
  u32 j = 0;
  for (u32 i = 0; i < MAX_CLIENTS; i++)
  {
    if (players[i]._client_id != 0)
    {
      v2i p = players[i]._pos;
      rects[j++] = {
        (s64)(p._x - PLAYER_SIZE*0.5f), 
        (s64)(p._y + PLAYER_SIZE*0.5f),
        (s64)(p._x + PLAYER_SIZE*0.5f),
        (s64)(p._y - PLAYER_SIZE*0.5f)};
      count++;
    }
  }
  return (count);
}
void
move_player(v2f by)
{
  players[0]._pos._x += by._x;
  players[0]._pos._y += by._y;
}
u32
get_world_rects(RECT *world_rects)
{
  u32 start = 0;
  for (u32 i = 0; i < food_count; i++)
  {
    for (u32 j = start; j < FOOD_MAX; j++)
    {
      if (food_alive[j])
      {
        world_rects[i] = temp[j];
        start++;
        break;
      }
    }
  }
  return (food_count);
}

internal LARGE_INTEGER timing_first, timing_last, timing_freq;
internal void
dt_start()
{
  QueryPerformanceCounter(&timing_first);
}
internal f32
dt_end()
{
  QueryPerformanceCounter(&timing_last);
  f32 dt = (((f64)timing_last.QuadPart -
        (f64)timing_first.QuadPart)) / (f64)timing_freq.QuadPart;
  return (dt);
}

int main(int argc, char **argv)
{
  win32_initialise();
  for (;;)
  {
    if (client_connect())
    {
      fprintf(stdout, "Client connected.\n");
      break;
    }
  }
  QueryPerformanceFrequency(&timing_freq);
  timeBeginPeriod(1);

  players[0]._client_id = get_client_id();
  HANDLE poll_thread = (HANDLE)_beginthread(poll_server, 0, NULL);
  for (;;)
  {
    update();
    win32_update();

    dt_start();
    f32 dt = dt_end();
    f32 cap_delta = WORLD_UPDATE_FREQ / 1000.0f;
    if (dt < cap_delta)
    {
      Sleep((cap_delta-dt) * 1000);
    }
  }
  timeEndPeriod(1);
  return (0);
}
