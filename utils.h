/*
 *
 *
 *  utils.h
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

#ifndef __GEANYLSP_UTILS__
#define __GEANYLSP_UTILS__

#include <geanyplugin.h>
#include <json-glib/json-glib.h>
#define CNF_FILE "geany-lsp.json"
#define GENERIC_LANGUAGE "text/plain"
#define CLIKE_FILE "C-Like"
typedef struct {
    gint64 line;
    gint64 column;
} Position;
typedef struct {
	gchar *text;
	Position begin;
	Position end;
} TextEdit;
typedef struct {
	gint version;
	//gchar *open_content;
	gchar *uri;

} DocumentTracking;

const gchar *get_file_type_name(gchar *file_name);
void read_lsp_config_file(GeanyData *geany_data, JsonParser *cnf_parser, gboolean is_project);
void ready_lsp_json_node(GeanyData *geany_data, JsonNode *cnf_node, JsonParser *cnf_parser);

#endif
