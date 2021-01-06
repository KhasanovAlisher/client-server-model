// 1. Deletion object from table
// 2. Insert object to the table
// 3. Update

#include <stdio.h>
#include <stdlib.h>
#include "sql_manipulations.h"

// checks if the client_id is valid (logged in) and updates result if not logged in
int is_logged_in(int client_id, json_object *result)
{
    if (client_id < 0) {
        json_object_object_add(result, "success", json_object_new_boolean(FALSE));
        json_object_object_add(result, "message", json_object_new_string("client not logged in"));
        return 0;
    }
    return 1;
}


// --- PUBLIC FUNCTIONS ---

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
    if (json_array == NULL) {
        return 0;
    }

    json_object *obj = json_object_new_object();
    for (int i = 0; i < size; ++i) {
        json_object_object_add(obj, columns[i], json_object_new_string(data[i] ? data[i] : "NULL"));
    }
    json_object_array_add(json_array, obj);
    return 0;
}

void login(sqlite3 *db, int *client_id, json_object *result)
{
    if (*client_id >= 0) {
        json_object_object_add(result, "success", json_object_new_boolean(FALSE));
        json_object_object_add(result, "message", json_object_new_string("client already logged in"));
        return;
    }

    const char *login = strtok(NULL, " ");
    const char *pass = strtok(NULL, " ");
    char query[100];
    sprintf(query, "select id, pwd from users where name='%s'", login);
    json_object *clients = json_object_new_array();
    if (execute_query(db, query, clients)) {
        json_object_put(clients);
        json_object_object_add(result, "success", json_object_new_boolean(FALSE));
        json_object_object_add(result, "message", json_object_new_string("server error: sql query failed"));
        return;
    }

    if (json_object_array_length(clients) > 0) {
        json_object *client = json_object_array_get_idx(clients, 0);
        int captured_id = -1;
        json_object_object_foreach(client, key, val) {
            if (strcmp(key, "id") == 0) {
                captured_id = atoi(json_object_get_string(val));
            } else if (strcmp(key, "pwd") == 0) {
                if (strcmp(pass, json_object_get_string(val)) == 0) {
                    *client_id = captured_id;
                    json_object_object_add(result, "success", json_object_new_boolean(TRUE));
                    json_object_object_add(result, "message", json_object_new_string("successfully logged in client"));
                } else {
                    json_object_object_add(result, "success", json_object_new_boolean(FALSE));
                    json_object_object_add(result, "message", json_object_new_string("incorrect password"));
                }
            }
        }
    } else {
        json_object *maxId_array = json_object_new_array();
        if (execute_query(db, "select max(id) as max_id from users", maxId_array)) {
            json_object_put(maxId_array);
            json_object_object_add(result, "success", json_object_new_boolean(FALSE));
            json_object_object_add(result, "message", json_object_new_string("server error: sql query failed"));
            return;
        }

        int maxId = 0;
        if (json_object_array_length(maxId_array) > 0) {
            json_object *maxId_object = json_object_array_get_idx(maxId_array, 0);
            json_object_object_foreach(maxId_object, key, val) {
                if (strcmp(key, "max_id") == 0) {
                    maxId = atoi(json_object_get_string(val)) + 1;
                }
            }
        }

        // registering new client
        memset(query, 0, sizeof(query));
        sprintf(query, "insert into users (id, name, pwd) values(%d, '%s', %s)", maxId, login, pass);
        if (execute_query(db, query, NULL)) {
            json_object_object_add(result, "success", json_object_new_boolean(FALSE));
            json_object_object_add(result, "message", json_object_new_string("could not add client"));
        } else {
            *client_id = maxId;
            json_object_object_add(result, "success", json_object_new_boolean(TRUE));
            json_object_object_add(result, "message", json_object_new_string("successfully registered client"));
        }
    }

    json_object_put(clients);
}

void build_all_houses_list(int client_id, sqlite3 *db, json_object *result)
{
    if (client_id == 0) {
        json_object *query_result = json_object_new_array();
        if (execute_query(db, "select * from houses", query_result)) {
            json_object_put(query_result);
            json_object_object_add(result, "success", json_object_new_boolean(FALSE));
            json_object_object_add(result, "message", json_object_new_string("server error: sql query failed"));
        } else {
            json_object_object_add(result, "success", json_object_new_boolean(TRUE));
            json_object_object_add(result, "message", json_object_new_string("successfully retrieved query result"));
            json_object_object_add(result, "result", query_result);
        }
    } else {
        json_object_object_add(result, "success", json_object_new_boolean(FALSE));
        json_object_object_add(result, "message", json_object_new_string("current client is not admin or not logged in"));
    }
}

void build_users_list(int client_id, sqlite3 *db, json_object *result)
{
    if (client_id == 0) {
        json_object *query_result = json_object_new_array();
        if (execute_query(db, "select id, name from users where id<>0", query_result)) {
            json_object_put(query_result);
            json_object_object_add(result, "success", json_object_new_boolean(FALSE));
            json_object_object_add(result, "message", json_object_new_string("server error: sql query failed"));
        } else {
            json_object_object_add(result, "success", json_object_new_boolean(TRUE));
            json_object_object_add(result, "message", json_object_new_string("successfully retrieved query result"));
            json_object_object_add(result, "result", query_result);
        }
    } else {
        json_object_object_add(result, "success", json_object_new_boolean(FALSE));
        json_object_object_add(result, "message", json_object_new_string("current client is not admin or not logged in"));
    }
}

void remove_user(int client_id, sqlite3 *db, json_object *result)
{
    if (client_id == 0) {
        char *user_id = strtok(NULL, " ");
        char query[200];
        sprintf(query, "delete from users where id=%s", user_id);
        if (execute_query(db, query, NULL)) {
            json_object_object_add(result, "success", json_object_new_boolean(FALSE));
            json_object_object_add(result, "message", json_object_new_string("server error: sql query failed"));
        } else {
            json_object_object_add(result, "success", json_object_new_boolean(TRUE));
            json_object_object_add(result, "message", json_object_new_string("successfully removed the user"));
        }
    } else {
        json_object_object_add(result, "success", json_object_new_boolean(FALSE));
        json_object_object_add(result, "message", json_object_new_string("current client is not admin or not logged in"));
    }
}

void build_not_busy_houses_list(int client_id, sqlite3 *db, json_object *result)
{
    json_object *query_result = json_object_new_array();
    if (client_id < 0) {
        if (execute_query(db, "select * from houses where isBusy=0", query_result)) {
            json_object_put(query_result);
            json_object_object_add(result, "success", json_object_new_boolean(FALSE));
            json_object_object_add(result, "message", json_object_new_string("server error: sql query failed"));
            return;
        }
    } else {
        char query[200];
        sprintf(query, "select * from houses where isBusy=0 and user_id<>%d", client_id);
        if (execute_query(db, query, query_result)) {
            json_object_put(query_result);
            json_object_object_add(result, "success", json_object_new_boolean(FALSE));
            json_object_object_add(result, "message", json_object_new_string("server error: sql query failed"));
            return;
        }
    }

    json_object_object_add(result, "success", json_object_new_boolean(TRUE));
    json_object_object_add(result, "message", json_object_new_string("successfully retrieved query result"));
    json_object_object_add(result, "result", query_result);
}

void build_client_houses_list(int client_id, sqlite3 *db, json_object *result)
{
    if (!is_logged_in(client_id, result)) {
        return;
    }

    char query[200];
    sprintf(query, "select * from houses where user_id=%d", client_id);
    json_object *query_result = json_object_new_array();
    if (execute_query(db, query, query_result)) {
        json_object_put(query_result);
        json_object_object_add(result, "success", json_object_new_boolean(FALSE));
        json_object_object_add(result, "message", json_object_new_string("server error: sql query failed"));
    } else {
        json_object_object_add(result, "success", json_object_new_boolean(TRUE));
        json_object_object_add(result, "message", json_object_new_string("successfully retrieved query result"));
        json_object_object_add(result, "result", query_result);
    }
}

void create_room(int client_id, sqlite3 *db, json_object *result)
{
    if (!is_logged_in(client_id, result)) {
        return;
    }

    char *comment = strtok(NULL, " ");
    char *price = strtok(NULL, " ");
    char *floor = strtok(NULL, " ");
    char *no_rooms = strtok(NULL, " ");

    // processing commment: replacing '_' with ' '
    int comment_len = strlen(comment);
    for (int i = 0; i < comment_len; ++i) {
        if (comment[i] == '_') {
            comment[i] = ' ';
        }
    }

    char query[200];
    sprintf(query, "insert into houses (user_id, isBusy, comment, price, floor, no_rooms) "
                   "values(%d, %s, '%s', %s, %s, %s)", client_id, "0", comment, price, floor, no_rooms);
    if (execute_query(db, query, NULL)) {
        json_object_object_add(result, "success", json_object_new_boolean(FALSE));
        json_object_object_add(result, "message", json_object_new_string("could not add house"));
    } else {
        json_object_object_add(result, "success", json_object_new_boolean(TRUE));
        json_object_object_add(result, "message", json_object_new_string("successfully added house"));
    }
}

void update_room(int client_id, sqlite3 *db, json_object *result)
{
    if (!is_logged_in(client_id, result)) {
        return;
    }

    char *house_id = strtok(NULL, " ");
    char *comment = strtok(NULL, " ");
    char *price = strtok(NULL, " ");
    char *floor = strtok(NULL, " ");
    char *no_rooms = strtok(NULL, " ");

    // processing commment: replacing '_' with ' '
    int comment_len = strlen(comment);
    for (int i = 0; i < comment_len; ++i) {
        if (comment[i] == '_') {
            comment[i] = ' ';
        }
    }

    char query[200];
    sprintf(query, "update houses set comment='%s', price=%s, floor=%s, no_rooms=%s where house_id=%s",
            comment, price, floor, no_rooms, house_id);
    if (execute_query(db, query, NULL)) {
        json_object_object_add(result, "success", json_object_new_boolean(FALSE));
        json_object_object_add(result, "message", json_object_new_string("could not update house"));
    } else {
        json_object_object_add(result, "success", json_object_new_boolean(TRUE));
        json_object_object_add(result, "message", json_object_new_string("successfully updated house"));
    }
}

void remove_room(int client_id, sqlite3 *db, json_object *result)
{
    if (!is_logged_in(client_id, result)) {
        return;
    }

    char *house_id = strtok(NULL, " ");

    char query[200];
    sprintf(query, "delete from houses where house_id=%s", house_id);
    if (execute_query(db, query, NULL)) {
        json_object_object_add(result, "success", json_object_new_boolean(FALSE));
        json_object_object_add(result, "message", json_object_new_string("could not remove house"));
    } else {
        json_object_object_add(result, "success", json_object_new_boolean(TRUE));
        json_object_object_add(result, "message", json_object_new_string("successfully removed house"));
    }
}

void rent_room(int client_id, sqlite3 *db, json_object *result)
{
    if (!is_logged_in(client_id, result)) {
        return;
    }

    char *house_id = strtok(NULL, " ");

    char query[200];
    sprintf(query, "update houses set isBusy=1 where house_id=%s", house_id);
    if (execute_query(db, query, NULL)) {
        json_object_object_add(result, "success", json_object_new_boolean(FALSE));
        json_object_object_add(result, "message", json_object_new_string("could not rent house"));
    } else {
        json_object_object_add(result, "success", json_object_new_boolean(TRUE));
        json_object_object_add(result, "message", json_object_new_string("successfully rented house"));
    }
}
