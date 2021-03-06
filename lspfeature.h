/*
 * lspfeature.h
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
 #ifndef __GEANYLSP_FEATS_
 #define __GEANYLSP_FEATS_
#include <geanyplugin.h>
#include <jsonrpc-glib.h>
#include "client.h"
#include "utils.h"

extern GeanyData *geany_data;
typedef struct {
    gint kind;
    gchar *label;
    gchar *detail;
} CompletionInfo;

typedef enum{
    TRIGGER_INVOKED = 1,
    TRIGGER_CHARACTER = 2,
    TRIGGER_CHANGE = 3,
} TriggerKind;

typedef enum
{
DEFINITION,
DECLERATION,
TYPE_DEFINITION,
IMPLEMENTION,
REFERENCE,
} ActionType;

void lsp_completion_on_doc (ClientManager *client_manager, GeanyDocument *doc, DocumentTracking *doc_track);
void lsp_completion_ask_resolve(ClientManager *client_manager, GeanyDocument *doc, DocumentTracking *doc_track);
void lsp_ask_detail(ClientManager *client_manager, GeanyDocument *doc, DocumentTracking *doc_track);
void lsp_ask_signature_help(ClientManager *client_manager, GeanyDocument *doc, DocumentTracking *doc_track);
void lsp_ask_for_action(ClientManager *client_manager, GeanyDocument *doc, DocumentTracking *doc_track, gint action_type);

#endif