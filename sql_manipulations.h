#ifndef SQL_MANIPULATIONS_H
#define SQL_MANIPULATIONS_H

#include <sqlite3.h>
#include <json-c/json.h>

// executes sqlite statement, while handling errors
int execute_query(sqlite3 *db, const char *query, json_object *json_array);

// callback for query result
int process_result(void *param, int size, char **data, char **columns);

int db_remove(sqlite3 *db, json_object *json_array);
int db_update(sqlite3 *db, json_object *json_array);
int db_insert(sqlite3 *db, json_object *json_array);

#endif // SQL_MANIPULATIONS_H
