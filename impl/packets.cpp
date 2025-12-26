/* ----------------------------------------------------------------------------
 * Copyright (c) 2025 jhlayt
 * See LICENSE.txt for copyright and licensing details. (GPL3.0)
 * ----------------------------------------------------------------------------
 * */
#include "../packets.h"
#include <cstring>

u32
make_packet(enum PACKET_TYPE type, void *packet, char *dst)
{
  switch (type)
  {
    case ACK:
      {

      } break;
    case LOGIN:
      {
        memcpy(dst, (char*)packet, sizeof(login_packet));
        return (sizeof(login_packet));
      } break;

    case HEARTBEAT:
      {

      } break;
  }
  return (FALSE);
}
