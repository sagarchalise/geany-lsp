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
#ifndef __GEANYLSP_DOCS__
#define __GEANYLSP_DOCS__

#include <geanyplugin.h>
#include <jsonrpc-glib.h>
#include "client.h"
#include "utils.h"

extern GeanyData *geany_data;

void lsp_doc_opened(ClientManager *client_manager, GeanyDocument *doc, DocumentTracking *doc_track);
void lsp_doc_closed(ClientManager *client_manager, GeanyDocument *doc, gchar *uri);
void lsp_doc_saved(ClientManager *client_manager, GeanyDocument *doc, gchar *uri);
void lsp_doc_will_save(ClientManager *client_manager, GeanyDocument *doc, gchar *uri);
void lsp_doc_changed(ClientManager *client_manager, GeanyDocument *doc, DocumentTracking *doc_track);
void lsp_doc_format(ClientManager *client_manager, GeanyDocument *doc, gchar *uri);

#endif