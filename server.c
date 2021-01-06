#include <stdio.h>
#include <stdlib.h>
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
    } else if (strcmp(token, "all") == 0) {
        build_all_list(*client_id, db, &result);
    } else if (strcmp(token, "users") == 0) {
        build_users_list(*client_id, db, &result);
    } else if (strcmp(token, "list") == 0) {
        build_not_busy_list(*client_id, db, &result);
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
