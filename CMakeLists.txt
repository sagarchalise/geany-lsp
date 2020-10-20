cmake_minimum_required (VERSION 2.8.1)

project(GEANY-LSP)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1) # For ycmd completing
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(FindPkgConfig)
pkg_search_module(GEANY REQUIRED geany)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(JSONRPC_GLIB REQUIRED jsonrpc-glib-1.0)
pkg_search_module(JSON_GLIB REQUIRED json-glib-1.0)

include_directories(
	${GEANY_INCLUDE_DIRS}
	${GLIB_INCLUDE_DIRS}
	${JSONRPC_GLIB_INCLUDE_DIRS}
	${JSON_GLIB_INCLUDE_DIRS}
)
add_library(geany-lsp SHARED
	geany-lsp.c
	utils.c
	client.c
	lspdoc.c
	lspfeature.c
)
message(${GEANY_LIBRARIES})

target_link_libraries(geany-lsp
	${GEANY_LIBRARIES}
	${GLIB_LIBRARIES}
	${JSONRPC_GLIB_LIBRARIES}
	${JSON_GLIB_LIBRARIES}
)

set_target_properties(geany-lsp PROPERTIES PREFIX "")

string(REGEX REPLACE ";" " " GEANY_CFLAGS_OTHER "${GEANY_CFLAGS_OTHER}") # Not really sure how/why semicolons got into the output...

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GEANY_CFLAGS_OTHER}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${GEANY_LDFLAGS_OTHER}")
set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.config/geany")

install(TARGETS geany-lsp DESTINATION plugins)
