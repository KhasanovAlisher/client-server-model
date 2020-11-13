#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "z_net_lib/z_net_client.h"

// structure used to send parameters to on_server_found() callbacks
struct serv_found_info
{
    struct network_info *net_info;    // used to send asynchronous request
    struct search_serv_info *sserv_info;    // used to stop server search
};

// callback functions
void on_server_found(struct server_address *serv_addr, void *param);
void on_receive(const char *msg, void *param);

// optionally first command-line argument can be provided to be ip otherwise search servers in broadcast channel
// optionally second command-line argument can be provided to be port otherwise use default port
int main(int argc, char *argv[])
{
    struct network_info net_info;    // required for sending requests and terminating connection

    if (argc >= 2) {    // if second argument is present use it as server ip
        char *ip = argv[1];
		
        int port = DEFAULT_PORT;
        if (argc >= 3)    // if third argument is present use it as port
            port = atoi(argv[2]);

        struct network_info net_info;
        if (connect_to_server(ip, port, &net_info) < 0) // connecting to server
            return -1;

        request_async(&net_info, "Random message that I wanted to tell", on_receive, NULL);    // sending request
    }
    else {
        struct search_serv_info sserv_info;    // given by user to search_servers, used to stop server search
        struct serv_found_info serv_found_info;    // parameter for callback function on_server_found()
        serv_found_info.net_info = &net_info;
        serv_found_info.sserv_info = &sserv_info;
        if (search_servers(&sserv_info, on_server_found, &serv_found_info) < 0)
            perror("Failed to find a sever");
    }

    sleep(300);    // not letting client exit right away
    disconnect_from_server(&net_info);    // terminating connection
	
	return 0;
}

// broadcast server found callback. In applications with UI connect_to_server can be in the main thread, since it
// can be displayed as an option from a list of servers. Same principle goes for stop_server_search, which can be
// called when server search menu is quit, or a server is connected to
void on_server_found(struct server_address *serv_addr, void *param)
{
    printf("Server found: %s:%d\n", serv_addr->ip, serv_addr->port);

    struct serv_found_info *serv_found_info = (struct serv_found_info *)param;
    if (connect_to_server(serv_addr->ip, serv_addr->port, serv_found_info->net_info) < 0)    // connecting to server
        perror("Couldn't connect to server");

    stop_server_search(serv_found_info->sserv_info);    // stop searching for servers after first is found
    request_async(serv_found_info->net_info, "Random message that I wanted to tell", on_receive, NULL);    // sending request
}

// callback prints received reply from server
void on_receive(const char *msg, void *param)
{
    printf("[Server]: %s\n", msg);
}
