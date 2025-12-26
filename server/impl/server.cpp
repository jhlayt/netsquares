/* ----------------------------------------------------------------------------
 * Copyright (c) 2025 jhlayt
 * See LICENSE.txt for copyright and licensing details. (GPL3.0)
 * ----------------------------------------------------------------------------
 * */
#include "../server.h"
#include "../world.h"
#include "../../packets.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <stdio.h>
#include <cstdlib>
#include <timeapi.h>

internal u64 uptime;

typedef struct login_connection login_connection;
struct login_connection
{
  SOCKET _socket;
  sockaddr_storage _sockaddr_storage;
};

internal login_connection login_connections[MAX_LOGIN_CONNECTIONS];
internal SOCKET udp_socket;

typedef struct client_connection client_connection;
struct client_connection
{
  u32 _session_id;
  u32 _client_id;
  sockaddr_storage _address_info;
};

typedef struct client_connection_tracker client_connection_tracker;
struct client_connection_tracker
{
  client_connection _client_connection;
  u64 _client_last_timestamp;
  u32 _temp_establish_uid;
  bool8 _alive;
};

internal client_connection_tracker connected_clients[MAX_CLIENTS];

internal void
winsock_err(const char *msg)
{
  wchar_t *s = NULL;
  FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS, NULL, WSAGetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&s, 0, NULL);
  fprintf(stdout, "From %s, failed with : %S\n", msg, s);
  assert(FALSE && "");
}

internal u32
dumb_hash(const char *c)
{
  u32 hash = 0;
  char *t = &((char*)c)[0];
  while (*t != '\0')
  {
    hash += *t;
    t++;
  }
  return (hash);
}
internal u32
random_u32()
{
  /*
   * FIX (12:27PM 250707):
   *   Dumb temp code.
   * */
  local_persist u32 counter;
  counter += 17;
  return (counter);
}

internal
s32 get_empty_login_socket_index()
{
  for (u32 i = 0; i < MAX_LOGIN_CONNECTIONS; i++)
  {
    if (login_connections[i]._socket == INVALID_SOCKET)
    {
      return (i);
    }
  }
  return (-1);
}

//TEMP passing username here.
internal bool8
connect_client(u32 login_connections_index, const char *username)
{
  login_connection *lc = &login_connections[login_connections_index];
  client_connection cc;
  cc._client_id = random_u32();//dumb_hash(username);
  cc._session_id = random_u32();

  for (u32 i = 0; i < MAX_CLIENTS; i++)
  {
    if (!connected_clients[i]._alive)
    {
      connected_clients[i]._alive = TRUE;
      connected_clients[i]._client_last_timestamp = uptime;
      establish_comms_packet ecp;
      ecp._header._type = ESTABLISH_COMMS;
      ecp._header._size = sizeof(ecp);
      ecp._uid = rand();
      ecp._header._client_id = cc._client_id;
      connected_clients[i]._temp_establish_uid = ecp._uid;
      send(lc->_socket, (char*)&ecp, ecp._header._size, 0);
      connected_clients[i]._client_connection = cc;

      if (closesocket(lc->_socket) == SOCKET_ERROR)
      {
        assert(FALSE && "Failed to close socket on client connect.");
      }
      else
      {
        fprintf(stdout, "Client logged in.");
        return (TRUE);
      }
    }
  }
  assert(FALSE && "Ran out of connection bandwidth.");
  return (FALSE);
}

bool8
send_packet_to_all_clients(void *packet, u32 size)
{
  for (u32 i = 0; i < MAX_CLIENTS; i++)
  {
    if (connected_clients[i]._alive)
    {
      client_connection_tracker *c = &connected_clients[i];

      s32 sz = sizeof(sockaddr_storage);
      s32 r = sendto(udp_socket, (char*)packet, size, 0,
          (sockaddr*)&c->_client_connection._address_info,
          sz);
      if (r == SOCKET_ERROR)
      {
        //winsock_err("udp sendto()");
      }
      else
      {
      }

      sockaddr_in *sin = (sockaddr_in*)&c->_client_connection._address_info;
      char hname[128];
      getnameinfo((sockaddr*)&c->_client_connection._address_info,
          sizeof(c->_client_connection._address_info), hname, 128, NULL, 0,
          0);
      fprintf(stdout, "packet sent to: %s on %s:%d\n", 
          hname, inet_ntoa(sin->sin_addr), sin->sin_port);
    }
  }
  return (FALSE);
}

internal void
disconnect_client(u32 index)
{
  client_connection_tracker *c = &connected_clients[index];
  sockaddr_in *sin = (sockaddr_in*)&c->_client_connection._address_info;
  char hname[128];
  getnameinfo((sockaddr*)&c->_client_connection._address_info,
      sizeof(c->_client_connection._address_info), hname, 128, NULL, 0,
      0);
  fprintf(stdout, "Server disconnected client: %s on %s:%d\n", 
      hname, inet_ntoa(sin->sin_addr), sin->sin_port);
  player_leave(c->_client_connection._client_id);
  memset(&connected_clients[index], 0, sizeof(client_connection_tracker));
}

internal void
server_maintain_connections(void*)
{
  ping_packet ping;
  ping._header._type = HEARTBEAT;
  ping._header._size = sizeof(ping_packet);
  for (;;)
  {
    Sleep(1000);
    ping._timestamp = uptime;
    for (u32 i = 0; i < MAX_CLIENTS; i++)
    {
      if (connected_clients[i]._alive)
      {
        client_connection_tracker *c = &connected_clients[i];

        s32 sz = sizeof(sockaddr_storage);
        s32 r = sendto(udp_socket, (char*)&ping, sizeof(ping), 0,
              (sockaddr*)&c->_client_connection._address_info,
              sz);
        if (r == SOCKET_ERROR)
        {
          //winsock_err("udp sendto()"); //FIX dunno why this is failing, on
          //multiple client connects?
        }
        else
        {
        }

        sockaddr_in *sin = (sockaddr_in*)&c->_client_connection._address_info;
        char hname[128];
        getnameinfo((sockaddr*)&c->_client_connection._address_info,
            sizeof(c->_client_connection._address_info), hname, 128, NULL, 0,
            0);
        fprintf(stdout, "ping sent to: %s on %s:%d\n", 
            hname, inet_ntoa(sin->sin_addr), sin->sin_port);

        if ((uptime - connected_clients[i]._client_last_timestamp) > 5000)
        {
          disconnect_client(i);
        }
      }
    }
  }
}

internal void
server_handle_logins(void *)
{
  for (;;)
  {
    Sleep(1000);
    for (u32 i = 0; i < MAX_LOGIN_CONNECTIONS; i++)
    {
      if (login_connections[i]._socket != INVALID_SOCKET)
      {
        char buf[1024];
        u32 c = recv(login_connections[i]._socket, buf, 1024, 0);
        if (c > 0)
        {
          login_packet p = *(login_packet*)buf;
          if (db_lookup(p._username, p._password))
          {
            if (connect_client(i, p._username))
            {
              login_connections[i]._socket = INVALID_SOCKET;
            }
            else
            {
              assert(FALSE && "Failed client connection.");
            }
          }
        }
      }
    }
  }
}

internal void
server_listen_to_clients(void*)
{
  char buf[2048];
  struct sockaddr_storage their_addr;
  socklen_t sz = sizeof(their_addr);

  for (;;)
  {
    s32 r = recvfrom(udp_socket, buf, 2048-1, 0, (sockaddr*)&their_addr,
        &sz);
    if (r > 0)
    {
      net_header *header = (net_header*)&buf;
      switch (header->_type)
      {
        case CLIENT_STATE:
          {
            client_state_packet *p = (client_state_packet*)buf;
            player_update(p->_client_id, p->_player_pos);
          } break;
        case ESTABLISH_COMMS:
          {
            establish_comms_packet *ecp = (establish_comms_packet*)buf;
            for (u32 i = 0; i < MAX_CLIENTS; i++)
            {
              if (connected_clients[i]._temp_establish_uid == ecp->_uid)
              {
                fprintf(stdout, "Established:: %s\n", buf);
                player_join(connected_clients[i]._client_connection._client_id);
                connected_clients[i]._client_connection._address_info =
                  *(sockaddr_storage*)&their_addr;
                break;
              }
            }
          } break;
        case HEARTBEAT:
          {
            ping_packet *p = (ping_packet*)buf;
            for (u32 i = 0; i < MAX_CLIENTS; i++)
            {
              if (connected_clients[i]._client_connection._client_id
                  == p->_header._client_id)
              {
                fprintf(stdout, "timestamp updated.. %lld, %lld\n",
                    p->_timestamp, uptime);
                connected_clients[i]._client_last_timestamp = p->_timestamp;
                break;
              }
            }

          } break;
        default:
          {
            //not handled.
          } break;
      }
    }
  }
}

internal void
server_listen_for_new_connections(void*)
{
  WSADATA wsa_data;
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
  {
    assert(FALSE && "_WSAStartup()_ failed.");
  }
  struct addrinfo hints;
  struct addrinfo *address_info;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo("127.0.0.1", LOGIN_PORT, &hints, &address_info) != 0)
  {
    winsock_err("getaddrinfo()");
  }

  SOCKET tcp_socket = socket(address_info->ai_family, address_info->ai_socktype,
      address_info->ai_protocol);
  if (tcp_socket == INVALID_SOCKET)
  {
    winsock_err("socket()");
  }

  if (bind(tcp_socket, address_info->ai_addr, address_info->ai_addrlen) == SOCKET_ERROR)
  {
    winsock_err("bind()");
  }

  if (listen(tcp_socket, 5) == SOCKET_ERROR)
  {
    winsock_err("listen()");
  }

  freeaddrinfo(address_info);
  fprintf(stdout, "Listening for client connections.\n");
  for (;;)
  {
    struct sockaddr_storage their_addr;
    socklen_t sz = sizeof(sockaddr_storage);
    SOCKET connection_socket;

    if ((connection_socket = accept(tcp_socket, (sockaddr*)&their_addr, &sz))
        != INVALID_SOCKET)
    {
      s32 sid = get_empty_login_socket_index();
      if (sid != -1)
      {
        char hname[128];
        sockaddr_in *sin = (sockaddr_in*)&their_addr;
        getnameinfo((sockaddr*)&their_addr, sizeof(their_addr), hname, 128,
            NULL, 0, 0);
        fprintf(stdout, "New connection: %s on %s:%d\n", 
            hname, inet_ntoa(sin->sin_addr), sin->sin_port);
        login_connections[sid]._socket = connection_socket;
        login_connections[sid]._sockaddr_storage = their_addr;
      }
      else
      {
        winsock_err("accept()");
      }
    }
    else
    {
    }
  }
}

internal HANDLE server_function_threads[4];

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

int
main (int argc, char **argv)
{
  QueryPerformanceFrequency(&timing_freq);
  timeBeginPeriod(1);

  WSADATA wsa_data;
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
  {
    assert(FALSE && "Fail startup.");
  }
  struct addrinfo hints;
  struct addrinfo *address_info;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  if (getaddrinfo("127.0.0.1", GAME_PORT, &hints, &address_info) != 0)
  {
    winsock_err("getaddrinfo()");
  }
  udp_socket = socket(address_info->ai_family,
      address_info->ai_socktype, address_info->ai_protocol);
  if (udp_socket == INVALID_SOCKET)
  {
    winsock_err("udp socket()");
  }
  if (bind(udp_socket, address_info->ai_addr, address_info->ai_addrlen)
      == SOCKET_ERROR)
  {
    winsock_err("udp bind()");
  }

  freeaddrinfo(address_info);
  for (u32 i = 0; i < MAX_LOGIN_CONNECTIONS; i++)
  {
    login_connections[i]._socket = INVALID_SOCKET;
  }
  server_function_threads[0] = (HANDLE)_beginthread(
     server_listen_for_new_connections, 0, NULL);
  if (!server_function_threads[0])
  {
    assert(FALSE && "Failed to create listen_for_new_connections thread.");
  }
  server_function_threads[1] = (HANDLE)_beginthread(
     server_handle_logins, 0, NULL);
  if (!server_function_threads[1])
  {
    assert(FALSE && "Failed to create server_handle_logins thread.");
  }

  server_function_threads[2] = (HANDLE)_beginthread(
     server_maintain_connections, 0, NULL);
  if (!server_function_threads[2])
  {
    assert(FALSE && "Failed to create server_maintain thread.");
  }

  server_function_threads[3] = (HANDLE)_beginthread(
     server_listen_to_clients, 0, NULL);
  if (!server_function_threads[3])
  {
    assert(FALSE && "Failed to create server_listen_to_clients thread.");
  }


  spawn_food({100, 100});
  spawn_food({190, 190});
  spawn_food({150, 100});

  for (;;)
  {
    dt_start();
    world_update();
    f32 dt = dt_end();
    f32 cap_delta = WORLD_UPDATE_FREQ / 1000.0f;
    if (dt < cap_delta)
    {
      Sleep((cap_delta-dt) * 1000);
    }
    else
    {
      cap_delta *= 0; //so it doesn't get added to _uptime if Sleep() wasn't
                      //called.
    }
    f32 add = ((cap_delta + dt) * 1000.0f);
    f64 ut = (f64)uptime + add;
    f64 days = (f64)(ut / 1000.0f / 60.0f / 60.0f / 24.0f);
    f64 hr = (days - (f64)((u32)days)) * 24.0f;
    f64 min = (hr - (f64)((u32)hr)) * 60.0f;
    f64 sec = (min - (f64)((u32)min)) * 60.0f;

    uptime += (u64)add;
    /*
    fprintf(stdout, "uptime: %d days, %d hrs, %d mins, %d sec\n",
        (u32)days, (u32)hr, (u32)min, (u32)sec);
        */
  }

  WaitForMultipleObjects(4, server_function_threads, TRUE, INFINITE);

  timeEndPeriod(1);
  return (0);
}
