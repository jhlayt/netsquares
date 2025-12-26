/* ----------------------------------------------------------------------------
 * Copyright (c) 2025 jhlayt
 * See LICENSE.txt for copyright and licensing details. (GPL3.0)
 * ----------------------------------------------------------------------------
 * */
#include "../defines.h"

bool8 client_connect();
void poll_server(void *);
bool8 send_to_server(char* dat, u32 size);

u32 get_client_id();
