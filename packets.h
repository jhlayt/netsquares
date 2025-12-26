/* ----------------------------------------------------------------------------
 * Copyright (c) 2025 jhlayt
 * See LICENSE.txt for copyright and licensing details. (GPL3.0)
 * ----------------------------------------------------------------------------
 * */
#ifndef PACKETS_H
#define PACKETS_H

#include "defines.h"

typedef enum PACKET_TYPE : u8
{
  ACK,
  LOGIN,
  ESTABLISH_COMMS,
  HEARTBEAT,
  WORLD_STATE,
  CLIENT_STATE,
} PACKET_TYPE;

typedef struct net_header net_header;
struct net_header
{
  u8 _type;
  u8 _padd[3];
  u32 _client_id;
  u32 _size; //NOTE (23:08PM 250709): includes the size of this net_header.
};

typedef struct ping_packet ping_packet;
struct ping_packet
{
  net_header _header;
  u64 _timestamp;
};
typedef struct establish_comms_packet establish_comms_packet;
struct establish_comms_packet
{
  net_header _header;
  u32 _uid;
};

typedef struct login_packet login_packet;
struct login_packet
{
  net_header _header;
  char _username[32];
  char _password[32];
};

typedef struct world_state_packet world_state_packet;
struct world_state_packet
{
  net_header _header;
  u8 _food_count;
  v2i _food_pos[FOOD_MAX];
  CLIENT_ID _client_ids[MAX_CLIENTS];
  u32 _scores[MAX_CLIENTS];
  v2i _player_pos[MAX_CLIENTS];
};

typedef struct client_state_packet client_state_packet;
struct client_state_packet
{
  net_header _header;
  v2i _player_pos;
  u32 _client_id;
};


/*
 * TODO (23:09PM 250709):
 *   _make_packet()_ is just pass-thru, input packet to dst for now, later
 * it will include conversions to ensure network byte order.
 **/
u32 make_packet(enum PACKET_TYPE type, void *packet, char *dst);

#endif// PACKETS_H
