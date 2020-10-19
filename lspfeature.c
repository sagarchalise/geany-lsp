/*
 * lspfeature.c
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
#include "lspfeature.h"

static
gboolean check_capability_feature_flag(GVariant *server_capabilities, const gchar *root_key, const gchar *key){
    g_autoptr(GVariant) capabilities = get_server_capability_for_key(server_capabilities, root_key, NULL);
    if(capabilities == NULL){
        return TRUE;
        msgwin_status_add("No capabilities for %s.", key);
    }
    if(key == NULL){
        if(g_variant_is_of_type(capabilities, G_VARIANT_TYPE_BOOLEAN)){
            return g_variant_get_boolean(capabilities);
        }
        if(g_variant_is_of_type(capabilities, G_VARIANT_TYPE_VARDICT)){
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

static
void lsp_completion_cb(GObject      *object,
                                         GAsyncResult *result,
                                         gpointer      user_data)
{
    JsonrpcClient *rpc_client = (JsonrpcClient *)object;
    DocumentTracking *doc_track = user_data;
    g_autoptr(GError) error = NULL;
  g_autoptr(GVariant) reply = NULL;
  if (!jsonrpc_client_call_finish (rpc_client, result, &reply, &error))
    {
      /* translators: %s is replaced with the error message */
      msgwin_status_add("Failed to run completions: %s", error->message);
      return;
    }
    GeanyDocument *doc = document_get_current();
    GVariantIter iter;
    g_autoptr(GVariant) items= NULL;
    if (g_variant_is_of_type (reply, G_VARIANT_TYPE_VARDICT) &&
      (items = g_variant_lookup_value (reply, "items", NULL)))
    {
        g_autoptr(GVariant) node = NULL;
        g_variant_iter_init (&iter, items);
        gint kind = 0;
    gint count = 0;
        const gchar *detail;
        const gchar *label;
        while (g_variant_iter_loop (&iter, "v", &node))
        {
            if(!g_variant_lookup (node, "label", "&s", &label)){
                    continue;
            }
            JSONRPC_MESSAGE_PARSE (node, "kind", JSONRPC_MESSAGE_GET_INT32 (&kind));
            JSONRPC_MESSAGE_PARSE (node, "detail", JSONRPC_MESSAGE_GET_STRING (&detail));
            if(g_hash_table_contains(doc_track->completions, label)){
                continue;
            }
            CompletionInfo *cinfo = g_new0(CompletionInfo, 1);
            cinfo->label = g_strdup(label);
            cinfo->detail = g_strdup(detail);
            cinfo->kind = kind;
            g_hash_table_insert(doc_track->completions, cinfo->label, cinfo);

			/* for now, tag types don't all follow C, so just look at arglist */
    }
    GString *words = g_string_sized_new(150);
    GHashTableIter iter;
    gpointer key, value;
    count = 0;
    g_hash_table_iter_init (&iter, doc_track->completions);
    while (g_hash_table_iter_next (&iter, &key, &value))
    {
        gchar *label = (gchar *)key;
        msgwin_status_add("%s", label);
        if(doc_track->word_at_pos != NULL && !g_str_has_prefix(label, doc_track->word_at_pos)){
                continue;
        }
        g_string_append(words, label);
        if(count > 0){
            g_string_append_c(words, '\n');
        }
        count++;
    }
    // //msgwin_clear_tab(MSG_COMPILER);
   // /* From Geany */
	// /* hide autocompletion if only option is already typed */
    // //sci_send_command(doc->editor->sci, SCI_AUTOCCANCEL);
	if (sci_get_current_position(doc->editor->sci) > doc_track->cur_pos+1 || words->len == 0 || doc_track->rootlen >= words->len ||
		(words->str[doc_track->rootlen] == '?' && doc_track->rootlen >= words->len - 2))
	{
        g_string_free(words, TRUE);
		return;
	}
    //msgwin_compiler_add_string(COLOR_BLACK, words->str);
    if(scintilla_send_message(doc->editor->sci, SCI_AUTOCACTIVE, 0, 0)){
        sci_send_command(doc->editor->sci, SCI_AUTOCCANCEL);
    }
	scintilla_send_message(doc->editor->sci, SCI_AUTOCSETORDER, SC_ORDER_PERFORMSORT, 0);
	scintilla_send_message(doc->editor->sci, SCI_AUTOCSHOW, doc_track->rootlen, (sptr_t) words->str);
    g_string_free(words, TRUE);
}
}
void
lsp_completion_on_doc (ClientManager *client_manager, GeanyDocument *doc, DocumentTracking *doc_track)
{
    g_free(doc_track->word_at_pos);
    doc_track->word_at_pos = editor_get_word_at_pos(doc->editor, doc_track->cur_pos, NULL);
    doc_track->rootlen = 0;
    if(doc_track->word_at_pos != NULL){
        doc_track->rootlen = strlen(doc_track->word_at_pos);
    }
    if(doc_track->trigger != TRIGGER_CHARACTER && doc_track->rootlen <= 2){
        return;
    }
  g_autoptr(GVariant) params = NULL;
  ScintillaObject *sci;
    sci = doc->editor->sci;
    gint line = sci_get_line_from_position(sci, doc_track->cur_pos);
   gint column = sci_get_col_from_position(sci, doc_track->cur_pos);
  params = JSONRPC_MESSAGE_NEW (
    "textDocument", "{",
      "uri", JSONRPC_MESSAGE_PUT_STRING (doc_track->uri),
    "}",
    "position", "{",
      "line", JSONRPC_MESSAGE_PUT_INT32 (line),
      "character", JSONRPC_MESSAGE_PUT_INT32 (column),
    "}",
    "context", "{",
      "triggerKind", JSONRPC_MESSAGE_PUT_INT32 (doc_track->trigger),
      //"triggerCharacter", JSONRPC_MESSAGE_PUT_STRING(doc_track->ch),
    "}"
  );
    //msgwin_status_add("%d", doc_track->trigger);
    jsonrpc_client_call_async (client_manager->rpc_client,
                              "textDocument/completion",
                              params,
                              NULL,
                              lsp_completion_cb,
                              doc_track);


}
void lsp_completion_ask_resolve(ClientManager *client_manager, GeanyDocument *doc, DocumentTracking *doc_track){
    g_autoptr(GVariant) params = NULL;
    gint line, column;
    ScintillaObject *sci;
    sci = doc->editor->sci;
    line = sci_get_current_line(sci);
    column = sci_get_col_from_position(sci, doc_track->cur_pos);
    params = JSONRPC_MESSAGE_NEW (
    "textDocument", "{",
      "uri", JSONRPC_MESSAGE_PUT_STRING (doc_track->uri),
    "}",
    "position", "{",
      "line", JSONRPC_MESSAGE_PUT_INT32 (line),
      "character", JSONRPC_MESSAGE_PUT_INT32 (column),
    "}"
  );
  g_autoptr(GVariant) reply = NULL;
  g_autoptr(GError) error = NULL;
  jsonrpc_client_call (client_manager->rpc_client,
                              "completionItem/resolve",
                              params,
                              NULL,
                              &reply,
                              &error);

}
static void
lsp_detail_cb (GObject  *object, GAsyncResult *result, gpointer user_data)
{
    JsonrpcClient *rpc_client = (JsonrpcClient *)object;
    GeanyDocument *doc = document_get_current();
  g_autoptr(GError) error = NULL;
  g_autoptr(GVariant) reply = NULL;
  if (!jsonrpc_client_call_finish (rpc_client, result, &reply, &error))
    {
      /* translators: %s is replaced with the error message */
      msgwin_status_add("Failed to run details: %s", error->message);
      return;
    }
    const gchar *message = NULL;
    msgwin_clear_tab(MSG_COMPILER);

    if(JSONRPC_MESSAGE_PARSE (reply, "contents", JSONRPC_MESSAGE_GET_STRING (&message)) ||
        JSONRPC_MESSAGE_PARSE (reply, "contents",
            "{",
                "value", JSONRPC_MESSAGE_GET_STRING (&message),
            "}") ){
        if(g_str_equal(message, "") || message == NULL){
            return;
        }
        msgwin_compiler_add(COLOR_BLACK, "Doc:\n\n%s", message );
        msgwin_switch_tab(MSG_COMPILER, FALSE);
    }
}
void lsp_ask_detail(ClientManager *client_manager, GeanyDocument *doc, DocumentTracking *doc_track){
    // if(!check_capability_feature_flag(client_manager->server_capabilities, "hoverProvider", NULL)){
        // msgwin_status_add("LSP no hover feature.");
        // return;
    // }
    g_autoptr(GVariant) params = NULL;
    gint line, column;
    ScintillaObject *sci;
    sci = doc->editor->sci;
    line = sci_get_line_from_position(sci, doc_track->cur_pos);
    column = sci_get_col_from_position(sci, doc_track->cur_pos);
    params = JSONRPC_MESSAGE_NEW (
    "textDocument", "{",
      "uri", JSONRPC_MESSAGE_PUT_STRING (doc_track->uri),
    "}",
    "position", "{",
      "line", JSONRPC_MESSAGE_PUT_INT32 (line),
      "character", JSONRPC_MESSAGE_PUT_INT32 (column),
    "}"
    );

    jsonrpc_client_call_async (client_manager->rpc_client,
                             "textDocument/hover",
                             params,
                             NULL,
                             lsp_detail_cb,
                             NULL);
}


static void
lsp_signature_cb (GObject      *object,
                                         GAsyncResult *result,
                                         gpointer      user_data)
{
    JsonrpcClient *rpc_client = (JsonrpcClient *)object;
    GeanyDocument *doc = document_get_current();
    DocumentTracking *doc_track = user_data;
  g_autoptr(GError) error = NULL;
  g_autoptr(GVariant) reply = NULL;
  if (!jsonrpc_client_call_finish (rpc_client, result, &reply, &error))
    {
      /* translators: %s is replaced with the error message */
      msgwin_status_add("Failed to run signature help: %s", error->message);
      return;
    }
    g_autoptr(GVariant) items= NULL;
    if (g_variant_is_of_type (reply, G_VARIANT_TYPE_VARDICT) &&
      (items = g_variant_lookup_value (reply, "signatures", NULL)))
    {
        GVariantIter iter;
        g_variant_iter_init (&iter, items);
        g_autoptr(GVariant) node = NULL;
        GString *signatures = g_string_new("");
        gint count = 0;
        while (g_variant_iter_loop (&iter, "v", &node))
        {
            const gchar *label;
            if(!g_variant_lookup (node, "label", "&s", &label)){
            continue;
            }
            if(g_str_equal(label, "") || label == NULL){
                continue;
            }
            g_string_append(signatures, label);
            if (count > 0){
                // if(doc_track->trigger == TRIGGER_CHARACTER){
                    // break;
                // }
                g_string_append_c(signatures, '\n');
            }
            count++;
        }
        sci_send_command(doc->editor->sci, SCI_CALLTIPCANCEL);
        if(signatures->len > 1 ){
            scintilla_send_message(doc->editor->sci, SCI_CALLTIPSHOW, (doc_track->trigger != TRIGGER_CHARACTER)?scintilla_send_message(doc->editor->sci, SCI_WORDSTARTPOSITION, doc_track->cur_pos, TRUE):doc_track->cur_pos, (sptr_t) signatures->str);
            scintilla_send_message(doc->editor->sci, SCI_CALLTIPSETHLT, 0, signatures->len);
        }
        g_string_free(signatures, TRUE);
    }
}
void lsp_ask_signature_help(ClientManager *client_manager, GeanyDocument *doc, DocumentTracking *doc_track){
    // if(!check_capability_feature_flag(client_manager->server_capabilities, "signatureHelpProvider", NULL)){
        // msgwin_status_add("LSP no signature feature.");
        // return;
    // }
    g_autoptr(GVariant) params = NULL;
    ScintillaObject *sci;
    sci = doc->editor->sci;
    gint line = sci_get_line_from_position(sci, doc_track->cur_pos);
    gint column = sci_get_col_from_position(sci, doc_track->cur_pos);
    params = JSONRPC_MESSAGE_NEW (
    "textDocument", "{",
      "uri", JSONRPC_MESSAGE_PUT_STRING (doc_track->uri),
    "}",
    "position", "{",
      "line", JSONRPC_MESSAGE_PUT_INT32 (line),
      "character", JSONRPC_MESSAGE_PUT_INT32 (column),
    "}",
    "context", "{",
        "triggerKind", JSONRPC_MESSAGE_PUT_INT32 (doc_track->trigger),
    "}"
  );

  jsonrpc_client_call_async (client_manager->rpc_client,
                             "textDocument/signatureHelp",
                             params,
                             NULL,
                             lsp_signature_cb,
                             doc_track);

}
void lsp_ask_for_action(ClientManager *client_manager, GeanyDocument *doc, DocumentTracking *doc_track, gint action_type){
    // if(!check_capability_feature_flag(client_manager->server_capabilities, "signatureHelpProvider", NULL)){
        // msgwin_status_add("LSP no signature feature.");
        // return;
    // }
    const gchar *action_name = NULL;
    g_autoptr(GVariant) params = NULL;
    ScintillaObject *sci;
    sci = doc->editor->sci;
    gint line = sci_get_line_from_position(sci, doc_track->cur_pos);
    gint column = sci_get_col_from_position(sci, doc_track->cur_pos);
    switch(action_type){
        case IMPLEMENTION:
            action_name = "textDocument/implementation";
            break;
        case DECLERATION:
            action_name = "textDocument/declaration";
            break;
        case DEFINITION:
            action_name = "textDocument/definition";
            break;
        case TYPE_DEFINITION:
            action_name = "textDocument/typeDefinition";
            break;
        case REFERENCE:
            action_name = "textDocument/references";
            params = JSONRPC_MESSAGE_NEW (
    "textDocument", "{",
      "uri", JSONRPC_MESSAGE_PUT_STRING (doc_track->uri),
    "}",
    "position", "{",
      "line", JSONRPC_MESSAGE_PUT_INT32 (line),
      "character", JSONRPC_MESSAGE_PUT_INT32 (column),
    "}", "context", "{", "includeDeclaration", JSONRPC_MESSAGE_PUT_BOOLEAN (line),"}"
        );
            break;
    }
    if(action_name == NULL){
        return;
    }
    if(params == NULL){
    params = JSONRPC_MESSAGE_NEW (
    "textDocument", "{",
      "uri", JSONRPC_MESSAGE_PUT_STRING (doc_track->uri),
    "}",
    "position", "{",
      "line", JSONRPC_MESSAGE_PUT_INT32 (line),
      "character", JSONRPC_MESSAGE_PUT_INT32 (column),
    "}"
        );
    }
    g_autoptr(GError) error = NULL;
  g_autoptr(GVariant) reply = NULL;
  jsonrpc_client_call (client_manager->rpc_client,
                             action_name,
                             params,
                             NULL,
                             &reply,
                             &error);
    Position begin;
    Position end;
  const gchar *uri = NULL;
  if(g_variant_is_of_type(reply, G_VARIANT_TYPE_ARRAY)){
    GVariantIter iter;
    g_variant_iter_init (&iter, reply);
    g_autoptr(GVariant) node = NULL;
    while (g_variant_iter_loop (&iter, "v", &node))
        {
            if(JSONRPC_MESSAGE_PARSE (node,
        "uri", JSONRPC_MESSAGE_GET_STRING (&uri),
        "range", "{",
        "start", "{",
          "line", JSONRPC_MESSAGE_GET_INT64 (&begin.line),
          "character", JSONRPC_MESSAGE_GET_INT64 (&begin.column),
        "}",
        "end", "{",
          "line", JSONRPC_MESSAGE_GET_INT64 (&end.line),
          "character", JSONRPC_MESSAGE_GET_INT64 (&end.column),
        "}", "}"
      )){
           msgwin_status_add("%s %d, %d, %s, %d, %d", action_name, begin.line, begin.column, uri, end.line, end.column);
    }

  }
  }
  else {
    if (JSONRPC_MESSAGE_PARSE (reply,
        "uri", JSONRPC_MESSAGE_GET_STRING (&uri),
        "range", "{",
        "start", "{",
          "line", JSONRPC_MESSAGE_GET_INT64 (&begin.line),
          "character", JSONRPC_MESSAGE_GET_INT64 (&begin.column),
        "}",
        "end", "{",
          "line", JSONRPC_MESSAGE_GET_INT64 (&end.line),
          "character", JSONRPC_MESSAGE_GET_INT64 (&end.column),
        "}", "}"
      )){
           msgwin_status_add("%s, %d, %d, %s, %d, %d", action_name, begin.line, begin.column, uri, end.line, end.column);
    }
  }
}
