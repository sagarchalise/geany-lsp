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
#define CNF_FILE "lsp.json"
#define GENERIC_LANGUAGE "text/plain"
#define CLIKE_FILE "C-Like"
typedef struct {
	gint version;
	gchar *uri;
	gint cur_pos;
	gint trigger;
	gchar *word_at_pos;
	GHashTable *completions;
	gint rootlen;

} DocumentTracking;

const gchar *get_file_type_name(gchar *file_name);
void read_lsp_config_file(GeanyData *geany_data, JsonParser *cnf_parser, gboolean is_project);
void override_cnf(GeanyData *geany_data, JsonObject *lsp_json_cnf, gboolean from_project);
JsonObject *get_child_node_if_not_disabled(JsonObject *lsp_json_cnf, const gchar *key);
#endif
