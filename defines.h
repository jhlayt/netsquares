/* ----------------------------------------------------------------------------
 * Copyright (c) 2025 jhlayt
 * See LICENSE.txt for copyright and licensing details. (GPL3.0)
 * ----------------------------------------------------------------------------
 * */
#ifndef NETSQUARES_DEFINES_H
#define NETSQUARES_DEFINES_H

#define WIN32_LEAN_AND_MEAN //to stop using old version of berkeley sockets.
#include <stdint.h>
#include <assert.h>

#define LOGIN_PORT "6665"
#define GAME_PORT "6666"

#define MAX_LOGIN_CONNECTIONS 16
#define MAX_CLIENTS 32

#define CLIENT_ID u32

#define TRUE 1
#define FALSE 0

#define WINDOW_WIDTH 256
#define WINDOW_HEIGHT 256

#define MOVE_SPEED 5

#define SQUARE_SIZE 32
#define FOOD_MAX 64
#define FOOD_SIZE 5.0f
#define PLAYER_SIZE 10.0f

#define WORLD_UPDATE_FREQ 30

#define internal static
#define local_persist static

typedef uint32_t bool32;
typedef uint8_t bool8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef float f32;
typedef double f64;

typedef struct v2f v2f;
struct v2f
{
  f32 _x, _y;
};
typedef struct v2i v2i;
struct v2i
{
  s32 _x, _y;
};


#endif// NETSQUARES_DEFINES_H
