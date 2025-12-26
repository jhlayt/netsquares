/* ----------------------------------------------------------------------------
 * Copyright (c) 2025 jhlayt
 * See LICENSE.txt for copyright and licensing details. (GPL3.0)
 * ----------------------------------------------------------------------------
 * */
#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H
#include "../defines.h"

void update_time();

bool8 send_packet_to_all_clients(void *packet, u32 size);

bool8 db_add_register_new(char *name, char *pass);
bool8 db_delete_account(char *name);
bool8 db_lookup(char *name, char *pass);
#endif// SERVER_SERVER_H
