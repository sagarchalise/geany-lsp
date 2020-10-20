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

gboolean is_closed;

void lsp_client_failed(JsonrpcClient *rpc_client,
               gpointer user_data){
    GIOStream *iostream = user_data;
    if(g_io_stream_is_closed(iostream)){
        is_closed = TRUE;
    }
}


gboolean lsp_client_handle_call (JsonrpcClient *rpc_client,
               gchar         *method,
               GVariant      *id,
               GVariant      *params,
               gpointer       user_data){

                if(is_closed){
                    is_closed=FALSE;
                    return TRUE;
                }
                // if(g_str_equal(method, "client/registerCapability")){
                    // GVariantIter capabilities;
                    // if(JSONRPC_MESSAGE_PARSE (params,
                    // "registrations", JSONRPC_MESSAGE_GET_ITER (&capabilities))){
                    // g_autoptr(GVariant) value = NULL;
                    // while (g_variant_iter_loop (capabilities, "v", &value)){
                        // const gchar *id = NULL;
                        // const gchar *method = NULL;
                        // JSONRPC_MESSAGE_PARSE (value, "id", JSONRPC_MESSAGE_GET_STRING (&id));
                        // JSONRPC_MESSAGE_PARSE (value, "method", JSONRPC_MESSAGE_GET_STRING (&method));
                        // msgwin_status_add("%s, %s", id, method);
                        // //jsonrpc_client_call_async(self, method, NULL, NULL, NULL, NULL);

                    // }
               // }
            // else{
            msgwin_status_add("Handle: %s", method);
               // }
               return TRUE;
    }
static void
add_diagnostics_to_cache(GVariant *params)
{
    GeanyDocument *doc = document_get_current();
  scintilla_send_message(doc->editor->sci,
            SCI_MARKERDELETEALL,
            SC_MARK_BACKGROUND,
            SC_MARK_BACKGROUND
        );
    msgwin_clear_tab(MSG_COMPILER);
  const gchar *uri = NULL;
  gboolean success;
  g_autoptr(GVariantIter) json_diagnostics = NULL;
  success = JSONRPC_MESSAGE_PARSE (params,
    "uri", JSONRPC_MESSAGE_GET_STRING (&uri),
    "diagnostics", JSONRPC_MESSAGE_GET_ITER (&json_diagnostics)
  );
  if(!success){
    return;
  }
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
      Position begin;
      Position end;

      // /* Mandatory fields */
      if (!JSONRPC_MESSAGE_PARSE (value,
                                  "range", JSONRPC_MESSAGE_GET_VARIANT (&range),
                                  "message", JSONRPC_MESSAGE_GET_STRING (&message)))
        continue;

    if(g_str_equal(message, "") || message == NULL){
        continue;
    }
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
    if(is_closed){
        is_closed = FALSE;
        return;
    }
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

static void
initialize_client_cb (GObject      *object,
                            GAsyncResult *result,
                            gpointer      user_data)
{
  JsonrpcClient *rpc_client = (JsonrpcClient *)object;
  g_autoptr(GError) error = NULL;
  g_autoptr(GVariant) reply = NULL;

  if (jsonrpc_client_call_finish (rpc_client, result, &reply, &error)){
    g_autoptr(GVariant) initialized_param = NULL;
    JsonNode *server_capabilities = user_data;
    g_autoptr(GVariant) capabilities = NULL;
    capabilities = g_variant_lookup_value (reply, "capabilities", G_VARIANT_TYPE_VARDICT);
    if(capabilities != NULL){
        server_capabilities =json_gvariant_serialize(capabilities);
    }
    initialized_param = JSONRPC_MESSAGE_NEW ("initializedParams", "{", "}");

    jsonrpc_client_send_notification(rpc_client,
                                          "initialized",
                                          initialized_param,
                                          NULL,
                                          NULL);
  }
  if(error != NULL){
    msgwin_status_add("LSP initialization error: %s", error->message);
  }
}

void initialize_lsp_client(ClientManager *client_manager, GIOStream *iostream, gchar *root_uri, gchar *root_path, GVariant *initialization_options){

    /*
   * The first thing we need to do is initialize the client with information
   * about our project. So that we will perform asynchronously here. It will
   * also start our read loop.
   */
   g_autoptr(GVariant) params=NULL;
   g_autoptr(GError) error = NULL;
  params = JSONRPC_MESSAGE_NEW (
    "processId", JSONRPC_MESSAGE_PUT_INT64 (getpid ()),
    "clientInfo", "{", "name", JSONRPC_MESSAGE_PUT_STRING(CLIENT_NAME), "}",
    "rootUri", JSONRPC_MESSAGE_PUT_STRING (root_uri),
    "rootPath", JSONRPC_MESSAGE_PUT_STRING (root_path),
    "trace", JSONRPC_MESSAGE_PUT_STRING ("off"),
    //"initializationOptions", (initialization_options == NULL)?"null":initialization_options,
    "capabilities", "{",
        "textDocument", "{",
            "synchronization", "{",
                "dynamicRegistration", JSONRPC_MESSAGE_PUT_BOOLEAN (FALSE),
                "willSave", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
                "willSaveWaitUntil", JSONRPC_MESSAGE_PUT_BOOLEAN (FALSE),
                "didSave", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
            "}",
            "formatting","{", "dynamicRegistration", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE), "}",
            "rangeFormatting", "{", "dynamicRegistration", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE), "}",
            "hover", "{",
                "dynamicRegistration", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
                "contentFormat", "[", "plaintext", "]",
            "}",
            "signatureHelp", "{",
                "dynamicRegistration", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
                "signatureInformation", "{"
                    "documentationFormat", "[", "plaintext", "]",
                "}",
                "contextSupport", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
            "}",
            "publishDiagnostics", "{",
                "relatedInformation", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
                "versionSupport", JSONRPC_MESSAGE_PUT_BOOLEAN (FALSE),
                "tagSupport", JSONRPC_MESSAGE_PUT_BOOLEAN (FALSE),
            "}",
            "doumentSymbol", "{",
                "dynamicRegistration", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
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
            "completion", "{",
                "dynamicRegistration", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
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
        "definition", "{",
            "dynamicRegistration", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
            "linkSupport", JSONRPC_MESSAGE_PUT_BOOLEAN (FALSE),
        "}",
        "declaration", "{",
            "dynamicRegistration", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
            "linkSupport", JSONRPC_MESSAGE_PUT_BOOLEAN (FALSE),
        "}",
        "typeDefinition", "{",
            "dynamicRegistration", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
            "linkSupport", JSONRPC_MESSAGE_PUT_BOOLEAN (FALSE),
        "}",
        "references", "{",
            "dynamicRegistration", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
            "linkSupport", JSONRPC_MESSAGE_PUT_BOOLEAN (FALSE),
        "}",
        "implementation", "{",
            "dynamicRegistration", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
            "linkSupport", JSONRPC_MESSAGE_PUT_BOOLEAN (FALSE),
        "}",
        "workspace", "{",
            "applyEdit", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
            "configuration", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
            "workspaceFolders", JSONRPC_MESSAGE_PUT_BOOLEAN (FALSE),
        "}",
        "window", "{",
            "workDoneProgress", JSONRPC_MESSAGE_PUT_BOOLEAN (TRUE),
        "}",
    "}"
  );

    if(iostream != NULL){
        is_closed = FALSE;
        client_manager->rpc_client = jsonrpc_client_new(iostream);
        g_signal_connect(client_manager->rpc_client, "notification",
                    G_CALLBACK(lsp_client_handle_notification),
                    client_manager->server_capabilities);
        g_signal_connect(client_manager->rpc_client, "failed", G_CALLBACK(lsp_client_failed), client_manager->iostream);

        g_signal_connect(client_manager->rpc_client, "handle-call",
                           G_CALLBACK(lsp_client_handle_call),
                           client_manager->server_capabilities);
    }
     g_autoptr(GVariant) reply = NULL;
    jsonrpc_client_call_async (client_manager->rpc_client,
                            "initialize",
                              params,
                              NULL,
                              initialize_client_cb,
                              client_manager);

}
GVariant *get_server_capability_for_key(GVariant *server_capabilities, const gchar *key, const GVariantType *gv){
    if(!g_variant_is_of_type (server_capabilities, G_VARIANT_TYPE_VARDICT)){
        msgwin_status_add("Server Capabilities not found");
        return NULL;
    }
    return g_variant_lookup_value (server_capabilities, key, gv);
}
gboolean check_by_flag_on_server(GVariant *server_capabilities, const gchar *key){
    g_autoptr(GVariant) flagval = get_server_capability_for_key(server_capabilities, key, NULL);
    if(flagval == NULL){
        return FALSE;
    }
    if(g_variant_is_of_type(flagval, G_VARIANT_TYPE_BOOLEAN)){
        return g_variant_get_boolean(flagval);
    }
    gboolean flag;
    if(!JSONRPC_MESSAGE_PARSE(flagval, "workDoneProgress",  JSONRPC_MESSAGE_GET_BOOLEAN(&flag))){
        return FALSE;
    };
    return flag;
}
static void
lsp_client_shutdown_cb (GObject      *object,
                            GAsyncResult *result,
                            gpointer      user_data)
{
  JsonrpcClient *client = (JsonrpcClient *)object;
  g_autoptr(GError) error = NULL;

  if (jsonrpc_client_call_finish (client, result, NULL, &error)){
    jsonrpc_client_send_notification(client, "exit", NULL, NULL, &error);
    jsonrpc_client_close (client, NULL, &error);
  }
  if(error != NULL){
    msgwin_status_add("LSP shutdown error: %s", error->message);
  }
    ClientManager *client_manager = user_data;
    //g_object_unref(client_manager->server_capabilities);
    g_object_unref(client_manager->rpc_client);
    g_io_stream_close(client_manager->iostream, NULL, &error);
    g_subprocess_force_exit(client_manager->process);
    g_object_unref(client_manager->iostream);
    g_object_unref(client_manager->process);
    g_object_unref(client_manager->launcher);
    g_free(client_manager);
}

void
shutdown_lsp_client (ClientManager *client_manager)
{
  jsonrpc_client_call_async (client_manager->rpc_client,
                                 "shutdown",
                                 NULL,
                                 NULL,
                                 lsp_client_shutdown_cb,
                                 client_manager);
}
