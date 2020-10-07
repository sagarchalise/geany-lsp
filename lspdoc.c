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
#include "lspdoc.h"
#define OPENCLOSE_KEY "openClose"
#define WILLSAVE "willSave"
#define SAVE "save"
#define CHANGE "change"
#define INCLUDE_TEXT "includeText"
#define DOC_SYNC "textDocumentSync"
#define DOC_FORMATTING "documentFormattingProvider"

static
gboolean check_text_capability_flag(GVariant *server_capabilities, const gchar *key){
    g_autoptr(GVariant) capabilities = get_server_capability_for_key(server_capabilities, DOC_SYNC, NULL);
    if(capabilities == NULL){
        msgwin_status_add("No text capabilities for %s.", key);
        return FALSE;
    }
    if(g_variant_is_of_type(capabilities, G_VARIANT_TYPE_INT64)){
        return TRUE;
    }
    gboolean flag = FALSE;
    if(!JSONRPC_MESSAGE_PARSE (capabilities,
        key, JSONRPC_MESSAGE_GET_BOOLEAN (&flag)
        )
    ){
        return FALSE;
    }
    return flag;
}

void lsp_doc_opened (ClientManager *client_manager, GeanyDocument *doc, DocumentTracking *doc_track)
{
    if(!check_text_capability_flag(client_manager->server_capabilities, OPENCLOSE_KEY)){
        return;
    }
  g_autoptr(GVariant) params = NULL;
  g_autofree gchar *content = sci_get_contents(doc->editor->sci, -1);
  params = JSONRPC_MESSAGE_NEW (
    "textDocument", "{",
      "uri", JSONRPC_MESSAGE_PUT_STRING (doc_track->uri),
      "languageId", JSONRPC_MESSAGE_PUT_STRING (doc->file_type->name),
      "text", JSONRPC_MESSAGE_PUT_STRING (content),
      "version", JSONRPC_MESSAGE_PUT_INT64 (doc_track->version),
    "}"
  );
    msgwin_status_add_string("LSP didOpen calling.");
  jsonrpc_client_send_notification_async (client_manager->rpc_client,
                                          "textDocument/didOpen",
                                          params,
                                          NULL, NULL, NULL);

}

void
lsp_doc_closed (ClientManager *client_manager, GeanyDocument *doc, gchar *uri)
{
    if(!check_text_capability_flag(client_manager->server_capabilities, OPENCLOSE_KEY)){
        return;
    }
    msgwin_status_add_string("LSP didClose calling.");
    g_autoptr(GVariant) params = NULL;
  params = JSONRPC_MESSAGE_NEW (
    "textDocument", "{",
      "uri", JSONRPC_MESSAGE_PUT_STRING (uri),
    "}"
  );

  jsonrpc_client_send_notification_async (client_manager->rpc_client,
                                          "textDocument/didClose",
                                          params,
                                          NULL, NULL, NULL);
}

void
lsp_doc_changed(ClientManager *client_manager, GeanyDocument *doc, DocumentTracking *doc_track){
    g_autoptr(GVariant) capabilities = get_server_capability_for_key(client_manager->server_capabilities, DOC_SYNC, NULL);
    if(capabilities == NULL){
        msgwin_status_add("No text capabilities for %s.", DOC_SYNC);
        return;
    }
    gint64 sync_kind = 0;
    if(!JSONRPC_MESSAGE_PARSE (capabilities,
        CHANGE, JSONRPC_MESSAGE_GET_INT64 (&sync_kind)
        )
    )
    {
        return;
    }
    // if(sync_kind == 2){
        // return;
    // }
  g_autoptr(GVariant) params = NULL;
  g_autofree gchar *content = sci_get_contents(doc->editor->sci, -1);
  params = JSONRPC_MESSAGE_NEW (
    "textDocument", "{",
      "uri", JSONRPC_MESSAGE_PUT_STRING (doc_track->uri),
      "version", JSONRPC_MESSAGE_PUT_INT64 (doc_track->version),
    "}",
    "contentChanges", "[",
        "{","text", JSONRPC_MESSAGE_PUT_STRING (content),"}",
    "]"
  );
msgwin_status_add_string("LSP didChange calling.");
  jsonrpc_client_send_notification_async (client_manager->rpc_client,
                                          "textDocument/didChange",
                                          params,
                                          NULL, NULL, NULL);


}

void
lsp_doc_saved (ClientManager *client_manager, GeanyDocument *doc, gchar *uri)
{
    g_autoptr(GVariant) capabilities = get_server_capability_for_key(client_manager->server_capabilities, DOC_SYNC, NULL);
    if(capabilities == NULL){
        msgwin_status_add("No text capabilities for %s.", DOC_SYNC);
        return;
    }
    gint64 sync_kind = 0;
    g_autoptr(GVariant) value = NULL;
    gboolean include_text = TRUE;
    if(JSONRPC_MESSAGE_PARSE (capabilities,
        CHANGE, JSONRPC_MESSAGE_GET_INT64 (&sync_kind),
        SAVE, JSONRPC_MESSAGE_GET_VARIANT (&value)
        )
    ){
        if (g_variant_is_of_type (value, G_VARIANT_TYPE_BOOLEAN)){
            include_text = g_variant_get_boolean(value);
        }
        else{
            JSONRPC_MESSAGE_PARSE (value, INCLUDE_TEXT, JSONRPC_MESSAGE_GET_BOOLEAN(&include_text));
        }
    }
    if(sync_kind > 1 && !include_text){
        include_text = TRUE;
    }
    msgwin_status_add("LSP calling didSave. Including text: %d", include_text);
    g_autoptr(GVariant) params = NULL;
    if (include_text){
        g_autofree gchar *content = NULL;
        content = sci_get_contents(doc->editor->sci, -1);
        params = JSONRPC_MESSAGE_NEW (
            "textDocument", "{",
                "uri", JSONRPC_MESSAGE_PUT_STRING (uri),
                "text", JSONRPC_MESSAGE_PUT_STRING (content),
            "}"
        );
    }
    else{
        params = JSONRPC_MESSAGE_NEW (
            "textDocument", "{",
                "uri", JSONRPC_MESSAGE_PUT_STRING (uri),
            "}"
        );
    }
    jsonrpc_client_send_notification_async (client_manager->rpc_client,
                                          "textDocument/didSave",
                                          params,
                                          NULL, NULL, NULL);

}
void
lsp_doc_will_save(ClientManager *client_manager, GeanyDocument *doc, gchar *uri)
{
    if(!check_text_capability_flag(client_manager->server_capabilities, WILLSAVE)){
        return;
    }
    msgwin_status_add("LSP calling willSave.");
    g_autoptr(GVariant) params = NULL;
    params = JSONRPC_MESSAGE_NEW (
        "textDocument", "{",
            "uri", JSONRPC_MESSAGE_PUT_STRING (uri),
        "}",
        "reason", JSONRPC_MESSAGE_PUT_INT32 (1)
    );
    jsonrpc_client_send_notification_async (client_manager->rpc_client,
                                          "textDocument/"WILLSAVE,
                                          params,
                                          NULL, NULL, NULL);
}

static void
lsp_format_doc_cb (GObject      *object,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
    JsonrpcClient *rpc_client = (JsonrpcClient *)object;

    GeanyDocument *old_doc= user_data;
  g_autoptr(GError) error = NULL;
  g_autoptr(GVariant) reply = NULL;
  if (!jsonrpc_client_call_finish (rpc_client, result, &reply, &error))
    {
      /* translators: %s is replaced with the error message */
      msgwin_status_add("Failed to run formatting: %s", error->message);
      return;
    }
    GeanyDocument *doc = document_get_current();
    if(!utils_str_equal(doc->real_path, old_doc->real_path)){
        return;
    }
  GVariantIter iter;
    g_variant_iter_init (&iter, reply);

  while (g_variant_iter_loop (&iter, "v", &reply))
    {
      const gchar *new_text = NULL;
      gboolean success;
      Position begin;
      Position end;

      success = JSONRPC_MESSAGE_PARSE (reply,
        "range", "{",
          "start", "{",
            "line", JSONRPC_MESSAGE_GET_INT64 (&begin.line),
            "character", JSONRPC_MESSAGE_GET_INT64 (&begin.column),
          "}",
          "end", "{",
            "line", JSONRPC_MESSAGE_GET_INT64 (&end.line),
            "character", JSONRPC_MESSAGE_GET_INT64 (&end.column),
          "}",
        "}",
        "newText", JSONRPC_MESSAGE_GET_STRING (&new_text)
      );
      if (!success)
        {
        continue;
        }
        if(new_text != NULL){
            gint line_cnt = sci_get_line_count(doc->editor->sci);
            gint cur_line = sci_get_current_line(doc->editor->sci);
            sci_set_text(doc->editor->sci, new_text);
            gint new_line_cnt = sci_get_line_count(doc->editor->sci);
            sci_set_current_position(doc->editor->sci, sci_get_position_from_line(doc->editor->sci, (cur_line + (new_line_cnt - line_cnt))), TRUE);
        }
    }
}

void lsp_doc_format(ClientManager *client_manager, GeanyDocument *doc, gchar *uri){
    if(!check_text_capability_flag(client_manager->server_capabilities, DOC_FORMATTING)){
        msgwin_status_add_string("LSP no formatting support.");
        return;
    }
    const GeanyIndentPrefs *indent_prefs = editor_get_indent_prefs(doc->editor);
    g_autoptr(GVariant) params = NULL;
    params = JSONRPC_MESSAGE_NEW (
    "textDocument", "{",
      "uri", JSONRPC_MESSAGE_PUT_STRING (uri),
    "}",
    "options", "{",
      "tabSize", JSONRPC_MESSAGE_PUT_INT32 (indent_prefs->width),
      "insertSpaces", JSONRPC_MESSAGE_PUT_BOOLEAN ((indent_prefs->type == GEANY_INDENT_TYPE_TABS)?FALSE:TRUE),
    "}"
  );
  jsonrpc_client_call_async (client_manager->rpc_client,
                                  "textDocument/formatting",
                                  params,
                                  NULL,
                                  lsp_format_doc_cb,
                                  doc);

}
