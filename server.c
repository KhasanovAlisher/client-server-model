#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "z_net_lib/z_net_server.h"
#include "sql_manipulations.h"

#define DB_NAME "os_db.db"

// receive callback function
void on_receive(int client_id, const char *msg, void *param);


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

    on_receive(1, "list", db); // DEBUG

    // starting server
    int exit_code = start_server(port, on_receive, db);    // starting to listen for possible client connections

    // closing database
    sqlite3_close(db);
    return exit_code;
}


// callback function called when request is received from client, simply resends message back to client
void on_receive(int client_id, const char *msg, void *param)
{
    sqlite3 *db = (sqlite3 *)param;

    printf("[Client %d]: %s\n", client_id, msg);

    // caching message
    char *cache = (char *)malloc(strlen(msg) + 1);
    strcpy(cache, msg);

    // test constructing json object
    json_object *json_array = json_object_new_array();

    // parsing request
    char *token = strtok(cache, " ");
    int failed = 0;
    if (strcmp(token, "list") == 0) {
        failed = execute_query(db, "select * from houses", json_array);
    } else if (strcmp(token, "my_list") == 0) {
        // build up a list of client's rooms
        char *client_id = strtok(NULL, " ");
    } else if (strcmp(token, "create") == 0) {
        // create a room
    } else if (strcmp(token, "update") == 0) {
        // update a room
    } else if (strcmp(token, "delete") == 0) {
        // delete a room
    } else if (strcmp(token, "rent") == 0) {
        // rent a room
    } else {
        printf("Request command was not recognized: %s!\n", token);
        failed = 0;
    }

    printf("Json array:\n%s\n", json_object_to_json_string_ext(json_array, JSON_C_TO_STRING_PRETTY)); // DEBUG

    // sending reply
    if (!failed) {
        send_message(client_id, json_object_to_json_string(json_array));
    }

    // cleanup
    json_object_put(json_array);
    free(cache);
}
