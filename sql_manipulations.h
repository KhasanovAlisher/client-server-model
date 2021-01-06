#ifndef SQL_MANIPULATIONS_H
#define SQL_MANIPULATIONS_H

#include <string.h>
#include <sqlite3.h>
#include <json-c/json.h>

// executes sqlite statement, while handling errors
int execute_query(sqlite3 *db, const char *query, json_object *json_array);

// callback for query result
int process_result(void *param, int size, char **data, char **columns);

void login(sqlite3 *db, int *client_id, json_object **result);
void build_all_list(int client_id, sqlite3 *db, json_object **result);
void build_users_list(int client_id, sqlite3 *db, json_object **result);
void build_not_busy_list(int client_id, sqlite3 *db, json_object **result);
void build_client_list(int client_id, sqlite3 *db, json_object **result);
void create_room(int client_id, sqlite3 *db, json_object **result);
void update_room(int client_id, sqlite3 *db, json_object **result);
void remove_room(int client_id, sqlite3 *db, json_object **result);
void rent_room(int client_id, sqlite3 *db, json_object **result);

#endif // SQL_MANIPULATIONS_H
