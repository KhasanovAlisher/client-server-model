#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <sqlite3.h>
#include "z_net_lib/z_net_server.h"

#define DB_NAME "mydb.db"

// callback functions
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
        fprintf(stderr, "Couldn't open database: %s", sqlite3_errmsg(db));
        return -1;
    }
	
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

    // parsing request
    char *token = strtok(cache, " ");
    if (strcmp(token, "list") == 0) {
        // build up a list of rooms
    } else if (strcmp(token, "my_list") == 0) {
        // build up a list of client's rooms
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
        return;
    }

    // freeing cached message
    free(cache);

    // test constructing json object
    json_object *obj = json_object_new_object();
    json_object_object_add(obj, "mykey", json_object_new_string("myvalue"));

    // sending reply
    send_message(client_id, json_object_to_json_string(obj));

    // deleting object
    json_object_put(obj);
}
