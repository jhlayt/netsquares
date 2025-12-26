/* ----------------------------------------------------------------------------
 * Copyright (c) 2025 jhlayt
 * See LICENSE.txt for copyright and licensing details. (GPL3.0)
 * ----------------------------------------------------------------------------
 * */
#include "../../defines.h"
#include "../client.h"
#include "../../netsquares.h"
#include "../../packets.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

internal SOCKET udp_socket;
internal struct addrinfo *server_addr_info;
internal bool8 has_connection;
internal u32 client_id;
internal u64 server_last_timestamp;
internal bool8 server_ping_expected;

u32
get_client_id()
{
  return (client_id);
}

internal void
bind_for_udp()
{
  struct addrinfo hints;
  struct addrinfo *address_info;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  if (getaddrinfo("127.0.0.1", GAME_PORT, &hints, &address_info) != 0)
  {
    assert(FALSE && "_getaddrinfo()_ fail\n");
  }

  udp_socket = socket(address_info->ai_family,
      address_info->ai_socktype, address_info->ai_protocol);
  if (udp_socket == INVALID_SOCKET)
  {
    assert(FALSE && "udp socket fail.");
  }

  /*
  if (bind(udp_socket, address_info->ai_addr, address_info->ai_addrlen)
      == SOCKET_ERROR)
  {
    wchar_t *s = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&s, 0, NULL);
    fprintf(stdout, "%S\n", s);
    assert(FALSE && "udp bind failed.\n");
  }
  */
  freeaddrinfo(address_info);
}

bool8
client_connect()
{
  WSADATA wsa_data;
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
  {
    assert(FALSE && "Fail startup.");
  }

  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo("127.0.0.1", LOGIN_PORT, &hints, &server_addr_info) != 0)
  {
    assert(FALSE && "Fail");
  }

  SOCKET s = socket(server_addr_info->ai_family, server_addr_info->ai_socktype,
      server_addr_info->ai_protocol);
  if (s == INVALID_SOCKET)
  {
    assert(FALSE && "Socket failed\n");
  }

  for (;;)
  {
    if (connect(s, server_addr_info->ai_addr, (int)server_addr_info->ai_addrlen) ==
        SOCKET_ERROR)
    {
    }
    else
    {
      fprintf(stdout, "Server connection found.\n");
      for (;;)
      {
        login_packet p;
        p._header._type = LOGIN;
        sprintf(p._username, "user1");
        sprintf(p._password, "pass123");
        char buf[1024];
        u32 size = make_packet(LOGIN, (void*)&p, buf);
        send(s, buf, size, 0);
        memset(buf, 0, 1024);
        for (;;)
        {
          if (recv(s, buf, 1024, 0) > 0)
          {
            establish_comms_packet *ecp = (establish_comms_packet*)buf;
            if (ecp->_header._type == ESTABLISH_COMMS)
            {
              client_id = ecp->_header._client_id;
              fprintf(stdout, "Login accepted.\n");
              if (closesocket(s) == SOCKET_ERROR)
              {
                assert(FALSE && "Failed to close socket on client connect.");
              }
              bind_for_udp();

              struct sockaddr_in addr;
              addr.sin_family = AF_INET;
              addr.sin_port = htons(atoi(GAME_PORT));
              addr.sin_addr.s_addr = inet_addr("127.0.0.1");
              s32 sz = sizeof(sockaddr_in);
              s32 r = sendto(udp_socket, (char*)ecp, ecp->_header._size, 0, (sockaddr*)&addr, sz);
              if (r == SOCKET_ERROR)
              {
                wchar_t *s = NULL;
                FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
                    NULL, WSAGetLastError(),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPWSTR)&s, 0, NULL);
                fprintf(stdout, "%S\n", s);
              }
              else if (r > 0)
              {
                fprintf(stdout, "Sent login ack to switch to udp.\n");
              }
              else
              {
                assert(FALSE && "Fail send...");
              }
              has_connection = TRUE;
              return (TRUE);
            }
          }
        }
      }
    }
  }
  return (FALSE);
}

void
poll_server(void *)
{
  for (;;)
  {
    char buf[2048];
    struct sockaddr_storage their_addr;
    socklen_t sz = sizeof(sockaddr_storage);
    s32 r = recvfrom(udp_socket, buf, 2048-1, 0, (sockaddr*)&their_addr,
        &sz);
    if (r > 0 && r != SOCKET_ERROR)
    {
      net_header *nh = (net_header*)buf;
      switch (nh->_type)
      {
        case HEARTBEAT:
          {
            ping_packet *ping = (ping_packet*)buf;
            server_last_timestamp = ping->_timestamp;
            server_ping_expected = TRUE;

            if (send_to_server((char*)ping, ping->_header._size))
            {
              server_ping_expected = FALSE;
            }

            fprintf(stdout, "Heartbeat packet received, %lld.\n", ping->_timestamp);
          } break;
        case WORLD_STATE:
          {
            fprintf(stdout, "World state packet received.\n");
            set_world_state((world_state_packet*)buf);
          } break;
        default:
          {
            //Not handled.
          } break;
      }
    }
    else
    {
      wchar_t *s = NULL;
      FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
          NULL, WSAGetLastError(),
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          (LPWSTR)&s, 0, NULL);
      fprintf(stdout, "%S\n", s);
      assert(FALSE && "Recvfrom fail.");
    }
  }
}

bool8
send_to_server(char* dat, u32 size)
{

  /* FIX (23:49PM 250710):
   *   TEMP code to set client_id for the net header, unsafe.
   * */
  net_header *n = (net_header*)dat;
  n->_client_id = client_id;
  //TEMP



  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(GAME_PORT));
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  s32 sz = sizeof(sockaddr_in);
  s32 r = sendto(udp_socket, dat, size, 0, (sockaddr*)&addr, sz);
  if (r > 0 && r != SOCKET_ERROR)
    return (TRUE);
  fprintf(stdout, "Failed to send to server.\n");
  return (FALSE);
}

