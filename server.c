#include <stdio.h>
#include <stdlib.h>
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
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
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
	
    send_message(client_id, msg);
}
