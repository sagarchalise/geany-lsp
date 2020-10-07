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
	const gchar *cnf_file =  g_build_path(G_DIR_SEPARATOR_S, geany_data->app->configdir, "plugins", CNF_FILE, NULL);
	json_parser_load_from_file (cnf_parser, cnf_file, &error);
	if(error != NULL){
		msgwin_status_add("Unable to parse json for LSP: %s", error->message);
	}
}

void ready_lsp_json_node(GeanyData *geany_data, JsonNode *cnf_node, JsonParser *cnf_parser){
	read_lsp_config_file(geany_data, cnf_parser, FALSE);
	if(cnf_parser != NULL){
		cnf_node = json_parser_get_root(cnf_parser);
	}
}

void override_cnf(gchar *base_path, const gchar *file_name){
	g_autoptr(GError) error=NULL;
	const gchar *cnf_file =  g_build_path(G_DIR_SEPARATOR_S, base_path, "."CNF_FILE, NULL);
	//json_parser_load_from_file (cnf_parser, cnf_file, &error);
	// JsonParser *proj_json;
	// JsonObject *cur_cnf;
	// JsonNode *cur_json;
	// if(geany_data->project == NULL){
		// return;
	// }
	// const gchar *proj_cnf =  g_build_path(G_DIR_SEPARATOR_S, geany_data->app->project->base_path, "."CNF_FILE, NULL);
	// if(g_file_test(proj_cnf, G_FILE_TEST_EXISTS) && json_parser_load_from_file (proj_json, proj_cnf, NULL)){
		// cur_json = json_parser_get_root(cnf_parser);
		// if(!json_object_has_member(cur_json, doc->file_type->name)){
			// return;
		// }
		// return json_node_get_object(json_object_get_member(lsp_json, doc->file_type->name));
	// }
}

