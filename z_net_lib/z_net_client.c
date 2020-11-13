#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "z_net_client.h"
#include "z_net_common_pvt.h"

// Code from http://cs.baylor.edu/~donahoo/practical/CSockets/code/BroadcastReceiver.c
void *bcast_recv_thread(void *search_info_ptr)
{
    struct search_serv_info *search_info = (struct search_serv_info *)search_info_ptr;

    /* Create a best-effort datagram socket using UDP */
    int sock;
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Could not create socket\n");
        return (void *)1;
    }
    search_info->socket = sock;

    /* Construct bind structure */
    struct sockaddr_in broadcastAddr;
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    broadcastAddr.sin_port = htons(BROADCAST_PORT);

    /* Bind to the broadcast port */
    if (bind(sock, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr)) < 0) {
        perror("Could not bind broadcast address to socket");
        return (void *)1;
    }

    /* Start receiving datagrams from the server */
    char recvString[BCAST_MSG_SIZE];
    int recvStringLen;
    while (1) {
        memset(recvString, 0, sizeof(recvString));
        if ((recvStringLen = recvfrom(sock, recvString, BCAST_MSG_SIZE, 0, NULL, 0)) < 0)
            break;

        recvString[recvStringLen] = '\0';

        struct server_address serv_addr;
        char *bcast_check_str = strtok(recvString, ":");
        if (strcmp(bcast_check_str, "zdonik") != 0) {
            perror("The zDonik Broadcast Protocol not followed. Discarding message");
            continue;
        }
        strcpy(serv_addr.ip, strtok(NULL, ":"));
        serv_addr.port = atoi(strtok(NULL, ":"));

        ((void (*)(struct server_address *serv_addr, void *param))search_info->on_server_found)(&serv_addr, search_info->param);
    }

    return 0;
}
// End of code

int search_servers(struct search_serv_info *search_info, void (*on_server_found)(struct server_address *, void *), void *param)
{
    // Creating local thread info structure
    search_info->on_server_found = on_server_found;
    search_info->param = param;

    // Create search thread
    pthread_t thread;
    if (pthread_create(&thread, NULL, bcast_recv_thread, search_info) < 0) {
        perror("Could not create thread");
        return -1;
    }

    search_info->thread_id = thread;
    puts("Server search thread started");
    return 0;
}

void stop_server_search(struct search_serv_info *search_info)
{
    close(search_info->socket);
    puts("Stopped server search");
}

int connect_to_server(const char *ip, int port, struct network_info *net_info)
{
    // Create socket
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        perror("Could not create socket\n");
        return -1;
    }

    // Prepare the sockaddr_in structure
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_aton(ip, &(server_addr.sin_addr));

    // Connect to server
    if (connect(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connecting to server failed\n");
        return -1;
    }
    puts("Connected to server");

    net_info->socket = socket_desc;

    return 0;
}

void disconnect_from_server(struct network_info *net_info)
{
    close(net_info->socket);
}

void *receive_thread(void *rec_info_ptr)
{	
    struct receive_info *rec_info = (struct receive_info *)rec_info_ptr;

    char message[2000];
    int read_size = recv(rec_info->socket, message, sizeof(message), 0);
    if (!read_size) {
        perror("Lost connection to server\n");
        fflush(stdout);
        return 0;
    }
    puts("Received reply from server");

    // Calling on_receive callback function
    ((void (*)(const char *, void *param))rec_info->on_receive_ptr)(message, rec_info->param);

    free(rec_info_ptr);

    return 0;
}

int request_async(struct network_info *net_info, const char *msg, void (*on_receive)(const char *msg, void *param), void *param)
{
    send(net_info->socket, msg, strlen(msg), 0);

    // Setup receive_info struct
    struct receive_info *rec_info = (struct receive_info *)malloc(sizeof(struct receive_info));
    rec_info->socket = net_info->socket;
    rec_info->on_receive_ptr = on_receive;
    rec_info->param = param;

    // Create receive thread
    pthread_t thread;
    if (pthread_create(&thread, NULL, receive_thread, rec_info) < 0) {
        perror("Could not create thread");
        return -1;
    }

    puts("Receive thread started");
    return 0;
}
