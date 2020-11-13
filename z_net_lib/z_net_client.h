#ifndef H_Z_CLIENT
#define H_Z_CLIENT

#include "z_net_common.h"
#include <pthread.h>

struct server_address
{
    char ip[HOST_SIZE];
    int port;
};

struct search_serv_info
{
    int socket;
    pthread_t thread_id;
    void (*on_server_found)(struct server_address *serv_addr, void *param);
    void *param;
};

struct network_info
{
    int socket;
};

/**
 * @brief starts a thread that searches for servers on the broadcast channel
 * @param search_info structure provided by the user which is filled and used to stop server search
 * @param on_server_found callback function called when a valid server was found
 * @param param additional paramater that can be passed to the callback function
 * @return exit code (0 on success)
 *
 * search_servers() simply starts a thread which is used to receive broadcast messages on the broadcast port. Everytime a server
 * is found, the callback function on_server_found() is called for further processing. It is passed the server address containing
 * ip and port, and additional param. param parameter is passed on to the callback function so custom structures can be used.
 * search_info should be provided by the user, which is then filled out with broadcast connection information and should be used
 * to stop the server search thread. On return, the function either returns 0 on succes or -1 on failure, and reason for failure
 * is printed in perror
 */
int search_servers(struct search_serv_info *search_info, void (*on_server_found)(struct server_address *serv_addr, void *param), void *param);

/**
 * @brief stops the server search thread
 * @param search_info information of the broadcast connection used to terminate thread
 *
 * stop_server_search() stops the thread that was used to search for servers. search_info was filled by search_servers() and is
 * required by this function to close the broadcast connection
 */
void stop_server_search(struct search_serv_info *search_info);

/**
 * @brief connects to a server
 * @param ip server ip in string format ("127.0.0.1")
 * @param port server port
 * @param net_info structure provided by the user to be filled and is used to disconnect from server
 * @return exit code (0 on success)
 *
 * connect_to_server() starts a TCP connection to the server provided by ip and port. net_info should be provided
 * by the user, which is filled by this function and should be then used to for requests and closing connection to server. On return,
 * the function either returns 0 on succes or -1 on failure, and reason for failure is printed in perror
 */
int connect_to_server(const char *ip, int port, struct network_info *net_info);

/**
 * @brief disconnects from an existing server
 * @param net_info information of the server connection used to disconnect from server
 *
 * disconnect_from_server() stops the connection to the server. net_info was filled by connect_to_server() and is
 * required by this function to disconnect from the server
 */
void disconnect_from_server(struct network_info *net_info);

/**
 * @brief starts a thread which sends request to server and waits for reply
 * @param net_info connection information to server
 * @param msg request message to be sent to the server
 * @param on_receive callback function which is called when reply is received from the server
 * @param param additional paramater that can be passed to the callback function
 * @return exit code (0 on success)
 *
 * request_async() starts a separate thread that sends a request message provided by msg to the server and waits for a reply, unless
 * connection was forcibly closed. net_info was filled by connect_to_server() and is required by this function to send requests and
 * wait for replies from the server. on_receive() is called as soon as a reply is received from the server, and after finishing
 * the callback, thread is closed. It is passed the message which was received and additional param. param parameter is passed on
 * to the callback function so custom structures can be used. On return, the function either returns 0 on succes or -1 on failure,
 * and reason for failure is printed in perror
 */
int request_async(struct network_info *net_info, const char *msg, void (*on_receive)(const char *msg, void *param), void *param);

#endif
