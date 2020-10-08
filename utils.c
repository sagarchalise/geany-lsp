/*
 *
 *  utils.c
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "utils.h"

const gchar *get_file_type_name(gchar *file_name){
    if(g_str_equal(file_name, "C") || g_str_equal(file_name, "C++") || g_str_equal(file_name, "CPP")){
        return CLIKE_FILE;
    }
    return file_name;
}

void read_lsp_config_file(GeanyData *geany_data, JsonParser *cnf_parser, gboolean is_project){
	g_autoptr(GError) error=NULL;
	const gchar *cnf_file = NULL;
	if(!is_project){
		cnf_file =  g_build_path(G_DIR_SEPARATOR_S, geany_data->app->configdir, "plugins", CNF_FILE, NULL);
	}
	else{
		cnf_file =  g_build_path(G_DIR_SEPARATOR_S, geany_data->app->project->base_path, "."CNF_FILE, NULL);
	}
	json_parser_load_from_file (cnf_parser, cnf_file, &error);
	if(error != NULL){
		msgwin_status_add("LSP Unable to parse: %s %s", error->message, cnf_file);
	}
}
static
void override_each_data(JsonObject *root_obj, JsonObject *override_obj){
	JsonObjectIter iter;
	const gchar *member_name;
	JsonNode *member_node;
	json_object_iter_init (&iter, root_obj);
	while (json_object_iter_next (&iter, &member_name, &member_node))
	{
		if(!json_object_has_member(override_obj, member_name) || !JSON_NODE_HOLDS_OBJECT(member_node)){
			json_object_set_member(override_obj, member_name, json_object_dup_member(root_obj, member_name));
			continue;
		}
		override_each_data(json_node_get_object(member_node), json_object_get_object_member(override_obj, member_name));
	}
}

void override_cnf(GeanyData *geany_data, JsonObject *lsp_json_cnf){
	g_autoptr(JsonParser) proj_json_parser=NULL;
	proj_json_parser = json_parser_new();
	read_lsp_config_file(geany_data, proj_json_parser, TRUE);
	if(proj_json_parser == NULL){
		return;
	}
	g_autoptr(JsonObject) proj_obj = json_node_get_object(json_parser_get_root(proj_json_parser));
	override_each_data(proj_obj, lsp_json_cnf);
}

