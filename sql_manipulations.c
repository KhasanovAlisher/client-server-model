// 1. Deletion object from table
// 2. Insert object to the table
// 3. Update

#include <stdio.h>
#include <stdlib.h>
#include "sql_manipulations.h"

// wrapper for executing sql queries
int execute_query(sqlite3 *db, const char *query, json_object *json_array)
{
    char *errmsg = 0; // initializing into non-NULL
    int err_code = sqlite3_exec(db, query, process_result, json_array, &errmsg);
    if (err_code != SQLITE_OK && errmsg != NULL) {
        fprintf(stderr, "Couldn't execute query: %s\n", errmsg);
    }
    sqlite3_free(errmsg);
    return err_code;
}

// callback for query result
// size - Number of columns
// data - Tuple atributes
// columns - column names
int process_result(void *param, int size, char **data, char **columns)
{
    json_object *json_array = (json_object *)param;
    json_object *obj = json_object_new_object();
    for (int i = 0; i < size; ++i) {
        json_object_object_add(obj, columns[i], json_object_new_string(data[i] ? data[i] : "NULL"));
    }
    json_object_array_add(json_array, obj);
    return 0;
}
