#ifndef H_Z_SERVER
#define H_Z_SERVER

#include "z_net_common.h"

/**
 * @brief sends a reply message to the client
 * @param client_id identification number of client
 * @param msg reply message to send
 * @return exit code (0 on success)
 *
 * send_message() sends a reply message provided by msg to the client provided by client_id. client_id can be received in the callback
 * function of start_server(). On return, the function either returns 0 on succes or -1 on failure, and reason for failure is printed
 * in perror
 */
int send_message(int client_id, const char *msg);

/**
 * @brief starts listening to clients
 * @param port server port on which to start
 * @param on_receive callback function which is called when a request is received from the client
 * @param param additional paramater that can be passed to the callback function
 * @return exit code (0 on success)
 * @note this is a blocking function
 *
 * start_server() starts an infinite loop, accepting incoming client connections. When a client is connected, separate thread is created
 * for the client to handle its request/repies. When a request is received, the callback function is called provided by in arguments.
 * Note that this is a blocking function and should only be used at the end of a main execution flow (check example in server.c) or
 * in a separate thread. port is the server port to start listening to. on_receive() is called as soon as a request is received from
 * the client. It is passed the client_id which is used to send repliest, msg which is a request that was received from
 * the client and an additional param. param parameter is passed on to the callback function so custom structures can be used. On return,
 * the function either returns 0 on succes or -1 on failure, and reason for failure is printed in perror
 */
int start_server(int port, void (*on_receive)(int client_id, const char *msg, void *param), void *param);

#endif
