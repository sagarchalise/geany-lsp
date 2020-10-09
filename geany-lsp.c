/*
 *      demoplugin.c - this file is part of Geany, a fast and lightweight IDE
 *
 *      Copyright 2007 The Geany contributors
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along
 *      with this program; if not, write to the Free Software Foundation, Inc.,
 *      51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * Demo plugin - example of a basic plugin for Geany. Adds a menu item to the
 * Tools menu.
 *
 * Note: This is not installed by default, but (on *nix) you can build it as follows:
 * cd plugins
 * make demoplugin.so
 *
 * Then copy or symlink the plugins/demoplugin.so file to ~/.config/geany/plugins
 * - it will be loaded at next startup.
 */

#include "utils.h"
#include "client.h"
#include "lspfeature.h"
#include "lspdoc.h"
#include <Scintilla.h>	/* for the SCNotification struct */

GeanyData *geany_data;
JsonObject *lsp_json_cnf;
JsonParser *cnf_parser;
JsonNode *cnf_node;
GHashTable *process_map;
GHashTable *docs_versions;

static gboolean
has_client_in_map (GeanyDocument *doc, gpointer user_data)
{
	if(!DOC_VALID(doc)){ return FALSE; }
	const gchar *file_type_name = get_file_type_name(doc->file_type->name);
	if(!json_object_has_member(lsp_json_cnf, file_type_name)){
        return FALSE;
     }
	ClientManager *client_manager;
	if(g_hash_table_contains(process_map, file_type_name)){
		client_manager = g_hash_table_lookup(process_map, file_type_name);
	}
	else{
		JsonObject *cur_node = json_object_get_object_member(lsp_json_cnf, file_type_name);
		gboolean only_project = FALSE;
		if(json_object_has_member(cur_node, "only_project")){
			only_project = json_object_get_boolean_member(cur_node, "only_project");
		}
		if(only_project && geany_data->app->project == NULL){
			return FALSE;
		}
		JsonObject *env_members = NULL;
		if(json_object_has_member(cur_node, "env")){
			env_members = json_object_get_object_member(cur_node, "env");
		}
		GSubprocessLauncher *subprocess_launcher;
		GSubprocess *subprocess;
		GIOStream *iostream;
		subprocess_launcher = g_subprocess_launcher_new (G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDIN_PIPE);
		if(geany_data->app->project != NULL){
			g_subprocess_launcher_set_cwd(subprocess_launcher, geany_data->app->project->base_path);
		}
		const gchar*  pth = "PATH";
		g_subprocess_launcher_setenv(subprocess_launcher, "HOME", g_get_home_dir(), TRUE);
		g_subprocess_launcher_setenv(subprocess_launcher, pth, g_getenv(pth), TRUE);;
		if (env_members != NULL){
			JsonObjectIter iter;
			const gchar *member_name;
			JsonNode *member_node;
			json_object_iter_init (&iter, env_members);
			while (json_object_iter_next (&iter, &member_name, &member_node))
			{
				g_subprocess_launcher_setenv(subprocess_launcher, member_name, json_node_get_string(member_node), TRUE);
    		}
		}
		subprocess = g_subprocess_launcher_spawnv(subprocess_launcher, (const gchar**)g_strsplit(json_object_get_string_member(cur_node, "cmd"), " ", -1),NULL);
		if (subprocess == NULL){
			msgwin_status_add_string("Error in process spawn");
		}
		else{
			GOutputStream *stdout = g_subprocess_get_stdin_pipe (subprocess);
			GInputStream *stdin = g_subprocess_get_stdout_pipe (subprocess);
			iostream = g_simple_io_stream_new (stdin,stdout);
			if(G_IS_IO_STREAM (iostream)){
				client_manager = g_new0(ClientManager, 1);
				initialize_lsp_client(client_manager, iostream, geany_data);
			}
		}
    }
    if(client_manager->rpc_client != NULL){
		g_hash_table_insert(process_map, file_type_name, client_manager);
		return TRUE;
	}
	else{
		g_hash_table_remove(process_map, file_type_name);
		return FALSE;
	}
}
static gboolean on_editor_notify(GObject *object, GeanyEditor *editor,
								 SCNotification *nt, gpointer data)
{
	/* data == GeanyPlugin because the data member of PluginCallback was set to NULL
	 * and this plugin has called geany_plugin_set_data() with the GeanyPlugin pointer as
	 * data */
	 GeanyDocument *doc = editor->document;
	gboolean ret = FALSE;
	 if(!has_client_in_map(doc, data)){
        return ret;
     }
	gint lexer, pos, style;
	/* For detailed documentation about the SCNotification struct, please see
	 * http://www.scintilla.org/ScintillaDoc.html#Notifications. */
       pos = sci_get_current_position(editor->sci);
	if (G_UNLIKELY(pos < 2))
		return ret;
    lexer = sci_get_lexer(editor->sci);
	style = sci_get_style_at(editor->sci, pos - 2);

	/* don't autocomplete in comments and strings */
	if (!highlighting_is_code_style(lexer, style))
		return ret;


	/* For detailed documentation about the SCNotification struct, please see
	 * http://www.scintilla.org/ScintillaDoc.html#Notifications. */
	DocumentTracking *doc_track = g_hash_table_lookup(docs_versions, GUINT_TO_POINTER(doc->id));
	ClientManager *client_manager = g_hash_table_lookup(process_map, get_file_type_name(doc->file_type->name));
	g_autoptr(GVariant) completion_capabilities = get_server_capability_for_key(client_manager->server_capabilities, "completionProvider", NULL);
	g_autoptr(GVariant) signature_capabilities = get_server_capability_for_key(client_manager->server_capabilities, "signatureHelpProvider", NULL);
    if(completion_capabilities == NULL && signature_capabilities == NULL){
        msgwin_status_add_string("No editor capabilities.");
        return ret;
    }
	g_auto(GStrv) completion_trigger_chars = NULL;
    gboolean completion_success;
	completion_success = JSONRPC_MESSAGE_PARSE (completion_capabilities,
		"triggerCharacters", JSONRPC_MESSAGE_GET_STRV (&completion_trigger_chars)
	);
	g_auto(GStrv) signature_trigger_chars = NULL;
	gboolean sig_success;
	sig_success = JSONRPC_MESSAGE_PARSE (signature_capabilities,
		"triggerCharacters", JSONRPC_MESSAGE_GET_STRV (&signature_trigger_chars)
	);

	GeanyPlugin *plugin = data;
	gboolean call = FALSE;
	TriggerWithPos *tp = g_new0(TriggerWithPos, 1);
	tp->trigger = -5;
	tp->pos = pos;
	switch (nt->nmhdr.code)
	{
		case SCN_CHARADDED:
			if(sig_success){
				for (guint j = 0; signature_trigger_chars[j]; j++)
				{
					const gchar *trigger = signature_trigger_chars[j];

					if(nt->ch == g_utf8_get_char(trigger)){
						tp->trigger = TRIGGER_CHARACTER;
						break;
					}
				}
			}
			if(tp->trigger == TRIGGER_CHARACTER){
				lsp_ask_signature_help(client_manager, doc, doc_track->uri, tp);
			}
			else{
				if(completion_success){
					for (guint i = 0; completion_trigger_chars[i]; i++)
					{
						const gchar *trigger = completion_trigger_chars[i];

						if(nt->ch == g_utf8_get_char(trigger)){
							tp->trigger = TRIGGER_CHARACTER;
							break;

						}
					}
				}
				if(tp->trigger != TRIGGER_CHARACTER){
					switch(nt->ch){
						case '\r':
						case '\n':
						case '/':
						case ')':
						case '{':
						case '[':
						case '"':
						case '\'':
						case '}':
						case '=':
						case ' ':
							break;
						default:
							tp->trigger = TRIGGER_INVOKED;
							break;
					}
				}
				if (tp->trigger > 0){
					lsp_completion_on_doc(client_manager, doc, doc_track->uri, tp);
				}
			}
			break;
		// case SCN_AUTOCCOMPLETED:
            // if (editor_find_snippet(editor, nt->text) != NULL){
                // keybindings_send_command(GEANY_KEY_GROUP_EDITOR, GEANY_KEYS_EDITOR_COMPLETESNIPPET );
			// }
			// else{
				// //lsp_ask_doc(client_manager->rpc_client, plugin->geany_data, nt->position);
				// //ls_completion_ask_resolve(client_manager->rpc_client, plugin->geany_data, nt->text);
			// }
             // break;
        // case SCN_AUTOCSELECTION:
			// lsp_ask_detail(client_manager->rpc_client, plugin->geany_data, nt->position);
			// break;
            //self.get_jedi_doc_and_signatures(editor, pos, text=nt_text, doc=False)
		case SCN_DWELLSTART:
			tp->trigger = TRIGGER_CHANGE;
            tp->pos = nt->position;
			lsp_ask_detail( client_manager, doc, doc_track->uri, tp->pos);
            lsp_ask_signature_help(client_manager, doc, doc_track->uri, tp);
			break;
        case SCN_DWELLEND:
            sci_send_command(editor->sci, SCI_CALLTIPCANCEL);
			msgwin_clear_tab(MSG_COMPILER);
            break;
        default:
			break;
		}
	g_free(tp);
	return ret;
}

static
void add_doc_to_tracking(GeanyDocument *doc){
	if(g_hash_table_contains(docs_versions, GUINT_TO_POINTER(doc->id)))
	{
		return;
	}
	DocumentTracking *doc_track;
	doc_track = g_new0(DocumentTracking, 1);
	doc_track->version = 0;
	g_autoptr(GFile) dir = g_file_new_for_path(doc->real_path);
	doc_track->uri = g_file_get_uri(dir);
	g_hash_table_insert(docs_versions, GUINT_TO_POINTER(doc->id), doc_track);
}


static void on_document_open(GObject *obj, GeanyDocument *doc, gpointer user_data){
	if(!has_client_in_map(doc, user_data)){
		msgwin_status_add("No LSP Server for %s file type", get_file_type_name(doc->file_type->name));
        return;
	}
	add_doc_to_tracking(doc);
	ClientManager *client_manager = g_hash_table_lookup(process_map, get_file_type_name(doc->file_type->name));
	DocumentTracking *doc_track = g_hash_table_lookup(docs_versions, GUINT_TO_POINTER(doc->id));
	lsp_doc_opened(client_manager, doc, doc_track);
}
static void on_document_save(GObject *obj, GeanyDocument *doc, gpointer user_data){
	if(!has_client_in_map(doc, user_data)){
		//msgwin_status_add("No LSP Server for %s file type", get_file_type_name(doc->file_type->name));
        return;
	}
	DocumentTracking *doc_track = g_hash_table_lookup(docs_versions, GUINT_TO_POINTER(doc->id));
	ClientManager *client_manager = g_hash_table_lookup(process_map, get_file_type_name(doc->file_type->name));
	lsp_doc_will_save(client_manager, doc, doc_track->uri);
	lsp_doc_saved(client_manager, doc, doc_track->uri);
	//lsp_doc_format(client_manager, doc, doc_track->uri);
}
static void on_document_before_save(GObject *obj, GeanyDocument *doc, gpointer user_data){
	if(!has_client_in_map(doc, user_data)){
		//msgwin_status_add("No LSP Server for %s file type", get_file_type_name(doc->file_type->name));
        return;
	}
        else if(!doc->changed){
            return;
        }
	ClientManager *client_manager = g_hash_table_lookup(process_map, get_file_type_name(doc->file_type->name));
	DocumentTracking *doc_track = g_hash_table_lookup(docs_versions, GUINT_TO_POINTER(doc->id));
	doc_track->version += 1;
	msgwin_status_add("heelo");
	lsp_doc_changed(client_manager, doc, doc_track);

}
static void on_document_close(GObject *obj, GeanyDocument *doc, gpointer user_data){
	if(!has_client_in_map(doc, user_data)){
		//msgwin_status_add("No LSP Server for %s file type", get_file_type_name(doc->file_type->name));
        return;
	}
	ClientManager *client_manager = g_hash_table_lookup(process_map, get_file_type_name(doc->file_type->name));
	DocumentTracking *doc_track = g_hash_table_lookup(docs_versions, GUINT_TO_POINTER(doc->id));
	lsp_doc_closed(client_manager, doc, doc_track->uri);
	g_free(doc_track->uri);
	g_free(doc_track);
}
static void destroy_everything(){
	GHashTableIter iter;
	gpointer key, value;
	g_hash_table_iter_init (&iter, process_map);
	while (g_hash_table_iter_next (&iter, &key, &value))
	{
		ClientManager *cm = (ClientManager *)value;
		shutdown_lsp_client(cm->rpc_client);
		g_object_unref(cm->server_capabilities);
		g_object_unref(cm->rpc_client);
		g_free(cm);
	}
	g_hash_table_iter_init (&iter, docs_versions);
	while (g_hash_table_iter_next (&iter, &key, &value))
	{
		DocumentTracking *dt = (DocumentTracking *)value;
		g_free(dt->uri);
		g_free(dt);
	}
}
static void on_project_open(GObject *obj, GKeyFile *config, gpointer user_data){
	override_cnf(geany_data, lsp_json_cnf);
}

static PluginCallback demo_callbacks[] =
{
	/* Set 'after' (third field) to TRUE to run the callback @a after the default handler.
	 * If 'after' is FALSE, the callback is run @a before the default handler, so the plugin
	 * can prevent Geany from processing the notification. Use this with care. */
	 {"document-open", (GCallback) & on_document_open, FALSE, NULL},
	 {"document-close", (GCallback) & on_document_close, FALSE, NULL},
	 {"document-save", (GCallback) & on_document_save, FALSE, NULL},
	 {"document-before-save", (GCallback) & on_document_before_save, FALSE, NULL},
	 {"project-open", (GCallback) & on_project_open, FALSE, NULL},
	 //{"document-activate", (GCallback) & on_document_activate, FALSE, NULL},
	{ "editor-notify", (GCallback) &on_editor_notify, FALSE, NULL },
	{ NULL, NULL, FALSE, NULL }
};


/* Called by Geany to initialize the plugin */
static gboolean demo_init(GeanyPlugin *plugin, gpointer data)
{
	geany_data = plugin->geany_data;
	process_map = g_hash_table_new(g_str_hash, g_str_equal);
	docs_versions = g_hash_table_new(g_direct_hash, g_direct_equal);
	cnf_parser = json_parser_new();
	read_lsp_config_file(geany_data, cnf_parser, FALSE);
	read_lsp_config_file(geany_data, cnf_parser, FALSE);
	if(cnf_parser != NULL){
		cnf_node = json_parser_get_root(cnf_parser);
	}
	if(cnf_node != NULL){
		lsp_json_cnf = json_node_get_object(cnf_node);
	}
	guint i;
	foreach_document(i)
	{
		GeanyDocument *doc = documents[i];
		if(has_client_in_map(doc, data)){
			add_doc_to_tracking(doc);
		}
	}
	geany_plugin_set_data(plugin, plugin, NULL);
	return TRUE;
}


// /* Callback connected in demo_configure(). */
// static void
// on_configure_response(GtkDialog *dialog, gint response, gpointer user_data)
// {
	// /* catch OK or Apply clicked */
	// if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY)
	// {
		/* We only have one pref here, but for more you would use a struct for user_data */
		// GtkWidget *entry = GTK_WIDGET(user_data);

		// g_free(welcome_text);
		// welcome_text = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
		/* maybe the plugin should write here the settings into a file
		 * (e.g. using GLib's GKeyFile API)
		 * all plugin specific files should be created in:
		 * geany->app->configdir G_DIR_SEPARATOR_S plugins G_DIR_SEPARATOR_S pluginname G_DIR_SEPARATOR_S
		 * e.g. this could be: ~/.config/geany/plugins/Demo/, please use geany->app->configdir */
	// }
//}

/* Called by Geany to show the plugin's configure dialog. This function is always called after
 * demo_init() was called.
 * You can omit this function if the plugin doesn't need to be configured.
 * Note: parent is the parent window which can be used as the transient window for the created
 *       dialog. */
// static GtkWidget *demo_configure(GeanyPlugin *plugin, GtkDialog *dialog, gpointer data)
// {
	// GtkWidget *label, *entry, *vbox;

	// /* example configuration dialog */
	// vbox = gtk_vbox_new(FALSE, 6);

	// /* add a label and a text entry to the dialog */
	// label = gtk_label_new("Welcome text to show:");
	// gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	// entry = gtk_entry_new();
	// if (welcome_text != NULL)
		// gtk_entry_set_text(GTK_ENTRY(entry), welcome_text);

	// gtk_container_add(GTK_CONTAINER(vbox), label);
	// gtk_container_add(GTK_CONTAINER(vbox), entry);

	// gtk_widget_show_all(vbox);

	// /* Connect a callback for when the user clicks a dialog button */
	// g_signal_connect(dialog, "response", G_CALLBACK(on_configure_response), entry);
	// return vbox;
// }


/* Called by Geany before unloading the plugin.
 * Here any UI changes should be removed, memory freed and any other finalization done.
 * Be sure to leave Geany as it was before demo_init(). */
static void demo_cleanup(GeanyPlugin *plugin, gpointer data)
{

	g_object_unref(cnf_parser);
	json_node_free(cnf_node);
	json_object_unref(lsp_json_cnf);
	destroy_everything();
	g_hash_table_destroy(process_map);
	g_hash_table_destroy(docs_versions);
}

void geany_load_module(GeanyPlugin *plugin)
{
	//main_locale_init(LOCALEDIR, GETTEXT_PACKAGE);
	plugin->info->name = "GeanyLSP";
	plugin->info->description = "LSP Client for geany.";
	plugin->info->version = "0.1";
	plugin->info->author =  "Sagar Chalise <chalisesagarATgmailDOTcom>";

	plugin->funcs->init = demo_init;
	plugin->funcs->configure = NULL;
	plugin->funcs->help = NULL; /* This demo has no help but it is an option */
	plugin->funcs->cleanup = demo_cleanup;
	plugin->funcs->callbacks = demo_callbacks;

	GEANY_PLUGIN_REGISTER(plugin, 225);
}
