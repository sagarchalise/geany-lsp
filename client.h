/*
 * client.h
 *
 * Copyright 2020 Sagar Chalise <sagar@lf-debian>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 *
 */
#include <geanyplugin.h>
#include <gio/gio.h>
#include <jsonrpc-glib.h>

#ifndef __GEANYLSP_CLIENT_
#define __GEANYLSP_CLIENT_
#define CLIENT_NAME "Geany"
extern GeanyData *geany_data;

typedef struct ClientManager{
    JsonrpcClient *rpc_client;
    GVariant *server_capabilities;
    //GVariant *client_capabilities;
    //GVariant *initialization_options;
} ClientManager;

void initialize_lsp_client(ClientManager *client_manager, GIOStream *iostream, GeanyData *geany_data);
GVariant *get_server_capability_for_key(GVariant *server_capabilities, const gchar *key, const GVariantType *gv);
void shutdown_lsp_client (JsonrpcClient *rpc_client);
gboolean check_by_flag_on_server(GVariant *server_capabilities, const gchar *key);

#endif
