#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "z_net_lib/z_net_server.h"
#include "sql_manipulations.h"

#define DB_NAME "os_db.db"

// receive callback function
void on_receive(int session_id, int *client_id, const char *msg, void *param);


// optionally first command-line argument can be provided to be port otherwise use default port
int main(int argc, char *argv[])
{
    // parsing command-line arguments
    int port = DEFAULT_PORT;
    if (argc >= 2) {
        port = atoi(argv[1]);
    }

    // opening database
    sqlite3 *db;
    if (sqlite3_open_v2(DB_NAME, &db, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) {
        fprintf(stderr, "Couldn't open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // starting server
    int exit_code = start_server(port, on_receive, db);    // starting to listen for possible client connections

    // closing database
    sqlite3_close(db);
    return exit_code;
}


// --- functions to handle commands ----

void login(sqlite3 *db, int *client_id, json_object **result)
{
    if (*client_id >= 0) {
        *result = json_object_new_object();
        json_object_object_add(*result, "error", json_object_new_string("client already logged in"));
        return;
    }

    const char *login = strtok(NULL, " ");
    const char *pass = strtok(NULL, " ");
    char query[100];
    sprintf(query, "select id, pwd from users where name='%s'", login);
    json_object *clients = json_object_new_array();
    if (execute_query(db, query, clients)) {
        json_object_put(clients);
        *result = json_object_new_object();
        json_object_object_add(*result, "error", json_object_new_string("server error: sql query failed"));
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
                    *result = json_object_new_object();
                    json_object_object_add(*result, "info", json_object_new_string("successfully logged in client"));
                } else {
                    *result = json_object_new_object();
                    json_object_object_add(*result, "error", json_object_new_string("incorrect password"));
                }
            }
        }
    } else {
        memset(query, 0, sizeof(query));
        strcpy(query, "select max(id) as max_id from users");
        json_object *maxId_array = json_object_new_array();

        if (execute_query(db, query, maxId_array)) {
            json_object_put(maxId_array);
            *result = json_object_new_object();
            json_object_object_add(*result, "error", json_object_new_string("server error: sql query failed"));
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
        *result = json_object_new_array();
        if (execute_query(db, query, *result)) {
            json_object_put(*result);
            *result = json_object_new_object();
            json_object_object_add(*result, "error", json_object_new_string("could not add client"));
        } else {
            *client_id = maxId;
            json_object_put(*result);
            *result = json_object_new_object();
            json_object_object_add(*result, "info", json_object_new_string("successfully registered client"));
        }
    }

    json_object_put(clients);
}

void build_list(sqlite3 *db, json_object **result)
{
    *result = json_object_new_array();
    if (execute_query(db, "select * from houses", *result)) {
        json_object_put(*result);
        *result = json_object_new_object();
        json_object_object_add(*result, "error", json_object_new_string("server error: sql query failed"));
    }
}

void build_client_list(int client_id, sqlite3 *db, json_object **result)
{
    if (client_id >= 0) {
        // build up a list of client's rooms
    } else {
        *result = json_object_new_object();
        json_object_object_add(*result, "error", json_object_new_string("client not logged in"));
    }
}

void create_room(int client_id, sqlite3 *db, json_object **result)
{
    // create a room
}

void update_room(int client_id, sqlite3 *db, json_object **result)
{
    // update a room
}

void remove_room(int client_id, sqlite3 *db, json_object **result)
{
    // delete a room
}

void rent_room(int client_id, sqlite3 *db, json_object **result)
{
    // rent a room
}

// ---- end ----


// callback function called when request is received from client, simply resends message back to client
void on_receive(int session_id, int *client_id, const char *msg, void *param)
{
    sqlite3 *db = (sqlite3 *)param;

    printf("[Client %d]: %s\n", session_id, msg);

    // caching message
    char *cache = (char *)malloc(strlen(msg) + 1);
    strcpy(cache, msg);

    // test constructing json object
    json_object *result = NULL;

    // parsing request
    char *token = strtok(cache, " ");
    if (strcmp(token, "login") == 0) {
        login(db, client_id, &result);
    } else if (strcmp(token, "list") == 0) {
        build_list(db, &result);
    } else if (strcmp(token, "my_list") == 0) {
        build_client_list(*client_id, db, &result);
    } else if (strcmp(token, "create") == 0) {
        create_room(*client_id, db, &result);
    } else if (strcmp(token, "update") == 0) {
        update_room(*client_id, db, &result);
    } else if (strcmp(token, "delete") == 0) {
        remove_room(*client_id, db, &result);
    } else if (strcmp(token, "rent") == 0) {
        rent_room(*client_id, db, &result);
    } else {
        printf("Request command was not recognized: %s!\n", token);
        result = json_object_new_object();
        json_object_object_add(result, "error", json_object_new_string("command not recognized"));
    }

    // sending reply
    printf("Result:\n%s\n", json_object_to_json_string_ext(result, JSON_C_TO_STRING_PRETTY)); // DEBUG
    send_message(session_id, json_object_to_json_string(result));

    // cleanup
    json_object_put(result);
    free(cache);
}
