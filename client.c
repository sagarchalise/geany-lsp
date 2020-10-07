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
#include "client.h"


enum {
  SEVERITY_ERROR       = 1,
  SEVERITY_WARNING     = 2,
  SEVERITY_INFORMATION = 3,
  SEVERITY_HINT        = 4,
};

gboolean
lsp_client_handle_call (JsonrpcClient *self,
               gchar         *method,
               GVariant      *id,
               GVariant      *params,
               gpointer       user_data){
               msgwin_status_add("Handle: %s", method);
               //msgwin_status_add_string(g_variant_get_string (params, NULL));
               return TRUE;
               }

static void
add_diagnostics_to_cache(GVariant *params)
{
    GeanyDocument *doc = document_get_current();
  g_autoptr(GVariantIter) json_diagnostics = NULL;
  scintilla_send_message(doc->editor->sci,
            SCI_MARKERDELETEALL,
            SC_MARK_BACKGROUND,
            SC_MARK_BACKGROUND
        );
    msgwin_clear_tab(MSG_COMPILER);
  const gchar *uri = NULL;
  gboolean success;
  success = JSONRPC_MESSAGE_PARSE (params,
    "uri", JSONRPC_MESSAGE_GET_STRING (&uri),
    "diagnostics", JSONRPC_MESSAGE_GET_ITER (&json_diagnostics)
  );
    uri = uri+7;

    //msgwin_status_add("%s %s %d %d", doc->real_path, uri, success, utils_str_equal(doc->real_path, uri));
  if (utils_str_equal(doc->real_path, uri) && success)
    {
        scintilla_send_message(
                doc->editor->sci,
                SCI_ANNOTATIONSETVISIBLE,
                ANNOTATION_BOXED,
                ANNOTATION_INDENTED
            );
        g_autoptr(GVariant) value=NULL;
        sci_send_command(doc->editor->sci, SCI_ANNOTATIONCLEARALL);
        editor_indicator_clear(doc->editor, 33);
        editor_indicator_clear(doc->editor, 34);
    while (g_variant_iter_loop (json_diagnostics, "v", &value))
    {

       g_autoptr(GVariant) range = NULL;
      const gchar *message = NULL;
      const gchar *source = NULL;
      gint64 severity = 0;
      struct {
        gint64 line;
        gint64 column;
      } begin, end;

      // /* Mandatory fields */
      if (!JSONRPC_MESSAGE_PARSE (value,
                                  "range", JSONRPC_MESSAGE_GET_VARIANT (&range),
                                  "message", JSONRPC_MESSAGE_GET_STRING (&message)))
        continue;

      // /* Optional Fields */
      JSONRPC_MESSAGE_PARSE (value, "severity", JSONRPC_MESSAGE_GET_INT64 (&severity));
      JSONRPC_MESSAGE_PARSE (value, "source", JSONRPC_MESSAGE_GET_STRING (&source));

      // /* Extract location information */
      success = JSONRPC_MESSAGE_PARSE (range,
        "start", "{",
          "line", JSONRPC_MESSAGE_GET_INT64 (&begin.line),
          "character", JSONRPC_MESSAGE_GET_INT64 (&begin.column),
        "}",
        "end", "{",
          "line", JSONRPC_MESSAGE_GET_INT64 (&end.line),
          "character", JSONRPC_MESSAGE_GET_INT64 (&end.column),
        "}"
      );

       if (!success)
        continue;

        scintilla_send_message(doc->editor->sci, SCI_ANNOTATIONSETTEXT, end.line, (sptr_t) message);
        switch (severity)
            {
            case SEVERITY_ERROR:
                severity = 13;
                break;

        case SEVERITY_WARNING:
          severity = 11;
          break;

        case SEVERITY_INFORMATION:
            severity = 10;
            break;

        case SEVERITY_HINT:
        default:
          severity = 12;
          break;
        }
        scintilla_send_message(doc->editor->sci, SCI_ANNOTATIONSETSTYLE, end.line, severity);
        editor_indicator_set_on_range(doc->editor, 33, sci_get_position_from_line(doc->editor->sci, begin.line)+begin.column, sci_get_position_from_line(doc->editor->sci, end.line)+end.column);
        editor_indicator_set_on_range(doc->editor, 34, sci_get_position_from_line(doc->editor->sci, begin.line)+begin.column, sci_get_position_from_line(doc->editor->sci, end.line)+end.column);

    }
    }

}
void
lsp_client_handle_notification (JsonrpcClient *rpc_client,
               gchar         *method,
               GVariant      *params,
               gpointer       user_data)
{
    if (params != NULL)
    {
      if (g_str_equal (method, "textDocument/publishDiagnostics"))
        {
            msgwin_status_add("Notification: %s Handling.", method);
            add_diagnostics_to_cache(params);
        }
      if (g_str_equal (method, "window/logMessages"))
        {
            const gchar *message = NULL;
            if(JSONRPC_MESSAGE_PARSE (params, "message", JSONRPC_MESSAGE_GET_STRING (&message))){
                msgwin_status_add("Notification: %s Method: %s", method, message);
            }
        }
    }
}

void initialize_lsp_client(ClientManager *client_manager, GIOStream *iostream, GeanyData *geany_data){

    /*
   * The first thing we need to do is initialize the client with information
   * about our project. So that we will perform asynchronously here. It will
   * also start our read loop.
   */
   g_autoptr(GVariant) params=NULL;
   g_autoptr(GError) error = NULL;
   g_autofree gchar *root_uri = NULL;
   if(geany_data->app->project!=NULL){
       g_autoptr(GFile) workdir = g_file_new_for_path(geany_data->app->project->base_path);
        root_uri = g_file_get_uri (workdir);
   }
  params = JSONRPC_MESSAGE_NEW (
    "processId", JSONRPC_MESSAGE_PUT_INT64 (getpid ()),
    "clientInfo", "{", "name", JSONRPC_MESSAGE_PUT_STRING(CLIENT_NAME), "}",
    "rootUri", JSONRPC_MESSAGE_PUT_STRING (root_uri),
    //"rootPath", JSONRPC_MESSAGE_PUT_STRING (root_path),
    "trace", JSONRPC_MESSAGE_PUT_STRING ("off"),
    "capabilities", "{",
        "textDocument", "{",
            "synchronization", "{",
                "dynamicRegistration", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
                "willSave", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
                "willSaveWaitUntil", JSONRPC_MESSAGE_PUT_BOOLEAN (FALSE),
                "didSave", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
            "}",
        "}",
        "publishDiagnostics", "{",
            "relatedInformation", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
            "versionSupport", JSONRPC_MESSAGE_PUT_BOOLEAN (FALSE),
        "}",
        "workspace", "{",
            "applyEdit", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
            "configuration", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
        "doumentSymbol", "{",
          "SymbolKind", "{",
            "valueSet", "[",
              JSONRPC_MESSAGE_PUT_INT64 (1), /* File */
              JSONRPC_MESSAGE_PUT_INT64 (2), /* Module */
              JSONRPC_MESSAGE_PUT_INT64 (3), /* Namespace */
              JSONRPC_MESSAGE_PUT_INT64 (4), /* Package */
              JSONRPC_MESSAGE_PUT_INT64 (5), /* Class */
              JSONRPC_MESSAGE_PUT_INT64 (6), /* Method */
              JSONRPC_MESSAGE_PUT_INT64 (7), /* Property */
              JSONRPC_MESSAGE_PUT_INT64 (8), /* Field */
              JSONRPC_MESSAGE_PUT_INT64 (9), /* Constructor */
              JSONRPC_MESSAGE_PUT_INT64 (10), /* Enum */
              JSONRPC_MESSAGE_PUT_INT64 (11), /* Interface */
              JSONRPC_MESSAGE_PUT_INT64 (12), /* Function */
              JSONRPC_MESSAGE_PUT_INT64 (13), /* Variable */
              JSONRPC_MESSAGE_PUT_INT64 (14), /* Constant */
              JSONRPC_MESSAGE_PUT_INT64 (15), /* String */
              JSONRPC_MESSAGE_PUT_INT64 (16), /* Number */
              JSONRPC_MESSAGE_PUT_INT64 (17), /* Boolean */
              JSONRPC_MESSAGE_PUT_INT64 (18), /* Array */
              JSONRPC_MESSAGE_PUT_INT64 (19), /* Object */
              JSONRPC_MESSAGE_PUT_INT64 (20), /* Key */
              JSONRPC_MESSAGE_PUT_INT64 (21), /* Null */
              JSONRPC_MESSAGE_PUT_INT64 (22), /* EnumMember */
              JSONRPC_MESSAGE_PUT_INT64 (23), /* Struct */
              JSONRPC_MESSAGE_PUT_INT64 (24), /* Event */
              JSONRPC_MESSAGE_PUT_INT64 (25), /* Operator */
              JSONRPC_MESSAGE_PUT_INT64 (26), /* TypeParameter */
            "]",
          "}",
        "}",
      "}",
        "completion", "{",
          "contextSupport", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
          "completionItem", "{",
            "snippetSupport", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
            "documentationFormat", "[",
              "plaintext",
            "]",
            "deprecatedSupport", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
          "}",
          "completionItemKind", "{",
            "valueSet", "[",
              JSONRPC_MESSAGE_PUT_INT64 (1),
              JSONRPC_MESSAGE_PUT_INT64 (2),
              JSONRPC_MESSAGE_PUT_INT64 (3),
              JSONRPC_MESSAGE_PUT_INT64 (4),
              JSONRPC_MESSAGE_PUT_INT64 (5),
              JSONRPC_MESSAGE_PUT_INT64 (6),
              JSONRPC_MESSAGE_PUT_INT64 (7),
              JSONRPC_MESSAGE_PUT_INT64 (8),
              JSONRPC_MESSAGE_PUT_INT64 (9),
              JSONRPC_MESSAGE_PUT_INT64 (10),
              JSONRPC_MESSAGE_PUT_INT64 (11),
              JSONRPC_MESSAGE_PUT_INT64 (12),
              JSONRPC_MESSAGE_PUT_INT64 (13),
              JSONRPC_MESSAGE_PUT_INT64 (14),
              JSONRPC_MESSAGE_PUT_INT64 (15),
              JSONRPC_MESSAGE_PUT_INT64 (16),
              JSONRPC_MESSAGE_PUT_INT64 (17),
              JSONRPC_MESSAGE_PUT_INT64 (18),
              JSONRPC_MESSAGE_PUT_INT64 (19),
              JSONRPC_MESSAGE_PUT_INT64 (20),
              JSONRPC_MESSAGE_PUT_INT64 (21),
              JSONRPC_MESSAGE_PUT_INT64 (22),
              JSONRPC_MESSAGE_PUT_INT64 (23),
              JSONRPC_MESSAGE_PUT_INT64 (24),
              JSONRPC_MESSAGE_PUT_INT64 (25),
            "]",
          "}",
        "}",
      "}",
      "window", "{",
        "workDoneProgress", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
      "}",
    "}"
  );

    if(iostream != NULL){
        client_manager->rpc_client = jsonrpc_client_new(iostream);
        g_signal_connect(client_manager->rpc_client, "notification",
                    G_CALLBACK(lsp_client_handle_notification),
                    client_manager->server_capabilities);

        g_signal_connect(client_manager->rpc_client, "handle-call",
                           G_CALLBACK(lsp_client_handle_call),
                           client_manager->server_capabilities);
    }
     g_autoptr(GVariant) reply = NULL;
    jsonrpc_client_call (client_manager->rpc_client,
                            "initialize",
                              params,
                              NULL,
                              &reply,
                              &error);
    if(error != NULL){
        msgwin_status_add_string(error->message);
        return;
    }
    g_autoptr(GVariant) initialized_param = NULL;

    if (g_variant_is_of_type (reply, G_VARIANT_TYPE_VARDICT))
        client_manager->server_capabilities = g_variant_lookup_value (reply, "capabilities", G_VARIANT_TYPE_VARDICT);
    initialized_param = JSONRPC_MESSAGE_NEW ("initializedParams", "{", "}");

    jsonrpc_client_send_notification_async (client_manager->rpc_client,
                                          "initialized",
                                          initialized_param,
                                          NULL,
                                          NULL, NULL);

}
GVariant *get_server_capability_for_key(GVariant *server_capabilities, const gchar *key, const GVariantType *gv){
    if(!g_variant_is_of_type (server_capabilities, G_VARIANT_TYPE_VARDICT)){
        msgwin_status_add("Server Capabilities not found");
        return NULL;
    }
    return g_variant_lookup_value (server_capabilities, key, gv);
}
