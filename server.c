#include <stdio.h>
#include <stdlib.h>
#include "z_net_lib/z_net_server.h"

// callback functions
void on_receive(int client_id, const char *msg, void *param);

// optionally first command-line argument can be provided to be port otherwise use default port
int main(int argc, char *argv[])
{
	int port = DEFAULT_PORT;
	if (argc >= 2)
		port = atoi(argv[1]);
	
    return start_server(port, on_receive, NULL);    // starting to listen for possible client connections
}

// callback function called when request is received from client, simply resends message back to client
void on_receive(int client_id, const char *msg, void *param)
{
	printf("[Client %d]: %s\n", client_id, msg);
	
    send_message(client_id, msg);
}
