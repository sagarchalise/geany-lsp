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
        msgwin_status_add("No capabilities for %s.", key);
        return FALSE;
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
}

static
void lsp_completion_cb(GObject      *object,
                                         GAsyncResult *result,
                                         gpointer      user_data)
{
    JsonrpcClient *rpc_client = (JsonrpcClient *)object;
    GeanyDocument *doc = document_get_current();
    GeanyData *geany_data = user_data;
    g_autoptr(GError) error = NULL;
  g_autoptr(GVariant) reply = NULL;
  if (!jsonrpc_client_call_finish (rpc_client, result, &reply, &error))
    {
      /* translators: %s is replaced with the error message */
      msgwin_status_add("Failed to run completions: %s", error->message);
      return;
    }
    // if((sci_get_current_position(doc->editor->sci)-pos) > 0){
      // return;
    // }
    gint pos = sci_get_current_position(doc->editor->sci);
    g_autofree gchar *word_at_pos = editor_get_word_at_pos(doc->editor, pos, NULL);
    gint rootlen = 0;
    if(word_at_pos != NULL){
        rootlen = strlen(word_at_pos);
    }
    GVariantIter iter;
    g_autoptr(GVariant) items= NULL;
    if (g_variant_is_of_type (reply, G_VARIANT_TYPE_VARDICT) &&
      (items = g_variant_lookup_value (reply, "items", NULL)))
    {
        GString *words = g_string_sized_new(130);
        g_autoptr(GVariant) node = NULL;
        g_variant_iter_init (&iter, items);
        gint count = 0;
        gint kind = 0;
        const gchar *snippet = NULL;
        while (g_variant_iter_loop (&iter, "v", &node))
        {
            const gchar *label;
            if(!g_variant_lookup (node, "label", "&s", &label)){
            continue;
            }
            if(word_at_pos != NULL){
                // snippet = editor_find_snippet(doc->editor, word_at_pos);
                // if(count == 0 && snippet != NULL){
                    // g_string_append(words, snippet);
                    // g_string_append_c(words, '\n');
                // }

                 if(!g_str_has_suffix(label, word_at_pos)){
                      continue;
                }
            }
            g_string_append(words, label);
            if (count > 0){
                g_string_append_c(words, '\n');
            }
            if (count == geany_data->editor_prefs->autocompletion_max_entries)
			{
				g_string_append(words, "...");
				break;
			}
            g_variant_lookup(node, "kind", "i", &kind);
            //msgwin_compiler_add(COLOR_BLACK,"%s, %d", label, kind);
            switch(kind){
                case 1:
                case 5:
                case 9:
                case 15:
                    g_string_append(words, "?2");
                    break;
                default:
                    g_string_append(words, "?1");
                    break;
            }

            count++;


        }

    //msgwin_clear_tab(MSG_COMPILER);
   /* From Geany */
	/* hide autocompletion if only option is already typed */
    //sci_send_command(doc->editor->sci, SCI_AUTOCCANCEL);
	if (rootlen >= words->len ||
		(words->str[rootlen] == '?' && rootlen >= words->len - 2))
	{
        g_string_free(words, TRUE);
		return;
	}
    msgwin_compiler_add_string(COLOR_BLACK, words->str);
    // if(scintilla_send_message(doc->editor->sci, SCI_AUTOCACTIVE, 0, 0)){
        // sci_send_command(doc->editor->sci, SCI_AUTOCCANCEL);
    // }
	scintilla_send_message(doc->editor->sci, SCI_AUTOCSHOW, rootlen, (sptr_t) words->str);
    g_string_free(words, TRUE);
    }
}
void
lsp_completion_on_doc (JsonrpcClient *rpc_client, GeanyData *geany_data, gint pos)
{
  GeanyDocument *doc = document_get_current();
  g_return_if_fail(DOC_VALID(doc));
  g_autoptr(GVariant) params = NULL;
  g_autofree gchar *uri = NULL;
  gint trigger_kind = 1 ;
  ScintillaObject *sci;
    sci = doc->editor->sci;
    gint line = sci_get_current_line(sci);
    GFile *dir;
	dir = g_file_new_for_path(doc->real_path);
	uri = g_file_get_uri(dir);
    gint column = sci_get_col_from_position(sci, pos);
  params = JSONRPC_MESSAGE_NEW (
    "textDocument", "{",
      "uri", JSONRPC_MESSAGE_PUT_STRING (uri),
    "}",
    "position", "{",
      "line", JSONRPC_MESSAGE_PUT_INT32 (line),
      "character", JSONRPC_MESSAGE_PUT_INT32 (column),
    "}",
    "context", "{",
      "triggerKind", JSONRPC_MESSAGE_PUT_INT32 (trigger_kind),
    "}"
  );

    g_autoptr(GError) error = NULL;
    g_autoptr(GVariant) reply = NULL;
    jsonrpc_client_call_async (rpc_client,
                              "textDocument/completion",
                              params,
                              NULL,
                              lsp_completion_cb,
                              geany_data);
}
void lsp_completion_ask_resolve(JsonrpcClient *rpc_client, GeanyData *geany_data, gchar *text){
    GeanyDocument *doc = document_get_current();
  g_return_if_fail(DOC_VALID(doc));
  g_autoptr(GVariant) params = NULL;
  g_autofree gchar *uri = NULL;
  gint line, column;
    gint pos;
    ScintillaObject *sci;
    sci = doc->editor->sci;
    pos = sci_get_current_position(sci);
    line = sci_get_current_line(sci);
    GFile *dir;
	dir = g_file_new_for_path(doc->real_path);
	uri = g_file_get_uri(dir);
    column = sci_get_col_from_position(sci, pos);
    params = JSONRPC_MESSAGE_NEW (
    "textDocument", "{",
      "uri", JSONRPC_MESSAGE_PUT_STRING (uri),
    "}",
    "position", "{",
      "line", JSONRPC_MESSAGE_PUT_INT32 (line),
      "character", JSONRPC_MESSAGE_PUT_INT32 (column),
    "}"
  );
  g_autoptr(GVariant) reply = NULL;
  g_autoptr(GError) error = NULL;
  jsonrpc_client_call (rpc_client,
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
    gint pos = GPOINTER_TO_INT(user_data);
  g_autoptr(GError) error = NULL;
  g_autoptr(GVariant) reply = NULL;
  if (!jsonrpc_client_call_finish (rpc_client, result, &reply, &error))
    {
      /* translators: %s is replaced with the error message */
      msgwin_status_add("Failed to run completions: %s", error->message);
      return;
    }
    const gchar *message = NULL;
    msgwin_clear_tab(MSG_MESSAGE);

    if(JSONRPC_MESSAGE_PARSE (reply, "contents", JSONRPC_MESSAGE_GET_STRING (&message)) ||
        JSONRPC_MESSAGE_PARSE (reply, "contents",
            "{",
                "value", JSONRPC_MESSAGE_GET_STRING (&message),
            "}") ){
        msgwin_msg_add(COLOR_BLACK, sci_get_line_from_position(doc->editor->sci, pos), doc, "Doc:\n\n%s", message );
        msgwin_switch_tab(MSG_MESSAGE, FALSE);
    }
}
void lsp_ask_detail(ClientManager *client_manager, GeanyDocument *doc, gchar *uri, gint pos){
    if(!check_capability_feature_flag(client_manager->server_capabilities, "hoverProvider", NULL)){
        msgwin_status_add("LSP no hover feature.");
        return;
    }
    g_autoptr(GVariant) params = NULL;
    gint line, column;
    ScintillaObject *sci;
    sci = doc->editor->sci;
    line = sci_get_line_from_position(sci, pos);
    column = sci_get_col_from_position(sci, pos);
    params = JSONRPC_MESSAGE_NEW (
    "textDocument", "{",
      "uri", JSONRPC_MESSAGE_PUT_STRING (uri),
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
                             GINT_TO_POINTER(pos));
}


static void
lsp_signature_cb (GObject      *object,
                                         GAsyncResult *result,
                                         gpointer      user_data)
{
    JsonrpcClient *rpc_client = (JsonrpcClient *)object;
    GeanyDocument *doc = document_get_current();
    gint pos = GPOINTER_TO_INT(user_data);
  g_autoptr(GError) error = NULL;
  g_autoptr(GVariant) reply = NULL;
  if (!jsonrpc_client_call_finish (rpc_client, result, &reply, &error))
    {
      /* translators: %s is replaced with the error message */
      msgwin_status_add("Failed to run completions: %s", error->message);
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
        gint offset = 0;
        while (g_variant_iter_loop (&iter, "v", &node))
        {
            const gchar *label;
            if(!g_variant_lookup (node, "label", "&s", &label)){
            continue;
            }
            g_string_append(signatures, label);
            if (count > 0){
                g_string_append_c(signatures, '\n');
            }
            count++;
        }
        sci_send_command(doc->editor->sci, SCI_CALLTIPCANCEL);
        if(signatures->len > 1 ){
            scintilla_send_message(doc->editor->sci, SCI_CALLTIPSHOW, pos, (sptr_t) signatures->str);
        }
        g_string_free(signatures, TRUE);
    }
}
void lsp_ask_signature_help(ClientManager *client_manager, GeanyDocument *doc, gchar *uri, gint pos, gint kind){
    if(!check_capability_feature_flag(client_manager->server_capabilities, "signatureHelpProvider", NULL)){
        msgwin_status_add("LSP no signature feature.");
        return;
    }
    g_autoptr(GVariant) params = NULL;
    ScintillaObject *sci;
    sci = doc->editor->sci;
    gint line = sci_get_line_from_position(sci, pos);
    gint column = sci_get_col_from_position(sci, pos);
    params = JSONRPC_MESSAGE_NEW (
    "textDocument", "{",
      "uri", JSONRPC_MESSAGE_PUT_STRING (uri),
    "}",
    "position", "{",
      "line", JSONRPC_MESSAGE_PUT_INT32 (line),
      "character", JSONRPC_MESSAGE_PUT_INT32 (column),
    "}",
    "context", "{",
        "triggerKind", JSONRPC_MESSAGE_PUT_INT32 (kind),
    "}"
  );
  jsonrpc_client_call_async (client_manager->rpc_client,
                             "textDocument/signatureHelp",
                             params,
                             NULL,
                             lsp_signature_cb,
                             GINT_TO_POINTER(pos));

}
