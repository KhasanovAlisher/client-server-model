#ifndef Z_NET_COMMON_PVT
#define Z_NET_COMMON_PVT

/**
 * @def BROADCAST_PORT
 * @brief default port to send broadcast messages to and receive from
 */
#define BROADCAST_PORT 25123

/**
 * @def BCAST_MSG_SIZE
 * @brief broadcast message size including check string, 2 ":" delimiters, ip and port
 */
#define BCAST_MSG_SIZE 30

/**
 * @def BCAST_PERIOD
 * @brief the frequency (technically period) in which broadcast messages should be sent (in seconds)
 */
#define BCAST_PERIOD 3

struct receive_info
{
	int socket;
	void *on_receive_ptr;
	void *param;
};

#endif
