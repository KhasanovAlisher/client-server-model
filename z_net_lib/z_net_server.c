#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <asm/types.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include "z_net_server.h"
#include "z_net_common_pvt.h"

struct address_info
{
	char server_ip[HOST_SIZE];
	int server_port;
	char broadcast_ip[HOST_SIZE];
};

int send_message(int client_id, const char *msg)
{
    return send(client_id, msg, strlen(msg), 0);
}

void *connection_thread(void *rec_info_ptr)
{
	struct receive_info *rec_info = (struct receive_info *)rec_info_ptr;
	
	int read_size;
	char message[2000];
    int client_id = -1;
	
	// Server start
	while (1) {
		memset(message, 0, sizeof(message));
		read_size = recv(rec_info->socket, message, sizeof(message), 0);
		if (!read_size)
			break;
		
		// Calling on_receive callback function
        ((void (*)(int, int *, const char *, void *param))rec_info->on_receive_ptr)(rec_info->socket, &client_id, message, rec_info->param);
	}
	
	printf("Client [%d] disconnected\n", rec_info->socket);
	fflush(stdout);
	close(rec_info->socket);
	free(rec_info);
	
	return 0;
}

// Code from https://stackoverflow.com/questions/15668653/how-to-find-the-default-networking-interface-in-linux
#define BUFSIZE 8192

struct route_info{
	u_int dstAddr;
	u_int srcAddr;
	u_int gateWay;
	char ifName[IF_NAMESIZE];
};

int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId) {
	struct nlmsghdr *nlHdr;
	int readLen = 0, msgLen = 0;
	do {
		/* Receive response from the kernel */
		if((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0) {
			perror("SOCK READ: ");
			return -1;
		}
		nlHdr = (struct nlmsghdr *)bufPtr;
		
		/* Check if the header is valid */
		if((0 == NLMSG_OK(nlHdr, readLen)) || (NLMSG_ERROR == nlHdr->nlmsg_type)) {
			perror("Error in received packet");
			return -1;
		}
		
		/* Check if it is the last message */
		if (NLMSG_DONE == nlHdr->nlmsg_type)
			break;

		/* Else move the pointer to buffer appropriately */
		bufPtr += readLen;
		msgLen += readLen;

		/* Check if its a multi part message; return if it is not. */
		if (0 == (nlHdr->nlmsg_flags & NLM_F_MULTI))
			break;
			
	} while ((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));
	
	return msgLen;
}

int get_default_interface(char *if_name)
{
	int sock, len, msgSeq = 0;

	/* Create Socket */
	if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
		perror("Socket Creation: ");

	/* Initialize the buffer */
	char msgBuf[BUFSIZE];
	memset(msgBuf, 0, BUFSIZE);

	/* point the header and the msg structure pointers into the buffer */
	struct nlmsghdr *nlMsg = (struct nlmsghdr *)msgBuf;
	nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
	nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .

	nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
	nlMsg->nlmsg_seq = msgSeq++; // Sequence of the message packet.
	nlMsg->nlmsg_pid = getpid(); // PID of process sending the request.

	/* Send the request */
	if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0) {
		printf("Write To Socket Failed...\n");
		return -1;
	}

	/* Read the response */
	if((len = readNlSock(sock, msgBuf, msgSeq, getpid())) < 0) {
		printf("Read From Socket Failed...\n");
		return -1;
	}
	
	/* Parse and print the response */
	struct route_info *rtInfo = (struct route_info *)malloc(sizeof(struct route_info));
	for(; NLMSG_OK(nlMsg, len); nlMsg = NLMSG_NEXT(nlMsg, len)) {
		memset(rtInfo, 0, sizeof(struct route_info));
		struct rtmsg *rtMsg;
		struct rtattr *rtAttr;
		int rtLen;
		rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);

		/* If the route is not for AF_INET or does not belong to main routing table
		then return. */
		if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
			return -1;
	
		/* get the rtattr field */
		rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
		rtLen = RTM_PAYLOAD(nlMsg);
		for (; RTA_OK(rtAttr,rtLen); rtAttr = RTA_NEXT(rtAttr,rtLen)) {
			switch(rtAttr->rta_type) {
				case RTA_OIF:
					if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
					break;
				case RTA_GATEWAY:
					rtInfo->gateWay = *(u_int *)RTA_DATA(rtAttr);
					break;
				case RTA_PREFSRC:
					rtInfo->srcAddr = *(u_int *)RTA_DATA(rtAttr);
					break;
				case RTA_DST:
					rtInfo->dstAddr = *(u_int *)RTA_DATA(rtAttr);
					break;
			}
		}
		
		if (rtInfo->dstAddr == 0) {
			strncpy(if_name, rtInfo->ifName, strlen(rtInfo->ifName));
			return 0;
        }
	}
	
	return -1;
}
// End of code

// Code from https://gist.github.com/loderunner/ec7d4725daca39283606
// and https://stackoverflow.com/questions/2283494/get-ip-address-of-an-interface-on-linux
int get_addresses(char *cur_addr, char *broad_addr)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	
	struct ifreq ifr;
	ifr.ifr_addr.sa_family = AF_INET;

	// Getting default interface name
	if (get_default_interface(ifr.ifr_name) < 0)
		return -1;
	
	printf("Operating on network interface '%s'\n", ifr.ifr_name);

	// Getting broadcast ip address
	ioctl(fd, SIOCGIFBRDADDR, &ifr);
	getnameinfo(&ifr.ifr_broadaddr, sizeof(ifr.ifr_broadaddr), broad_addr, HOST_SIZE, 0, 0, NI_NUMERICHOST);
	printf("Broadcast address: %s\n", broad_addr);

	// Getting current ip address
	ioctl(fd, SIOCGIFADDR, &ifr);
	strcpy(cur_addr, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	printf("Server address: %s\n", cur_addr);
	
	close(fd);

	return 0;
}
// End of code

// Code from http://cs.baylor.edu/~donahoo/practical/CSockets/code/BroadcastSender.c
void *broadcast_thread(void *addr_info_ptr)
{
	struct address_info *addr_info = (struct address_info *)addr_info_ptr;

	/* Create socket for sending/receiving datagrams */
	int socket_desc;
	if ((socket_desc = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		perror("Could not create socket");

	/* Set socket to allow broadcast */
	int broadcast_perm = 1;
	if (setsockopt(socket_desc, SOL_SOCKET, SO_BROADCAST, (void *)&broadcast_perm, sizeof(broadcast_perm)) < 0) {
		perror("Could not set socket option to broadcast");
		return 0;
	}

	/* Construct local address structure */
	struct sockaddr_in broadcastAddr;
	broadcastAddr.sin_family = AF_INET;
	broadcastAddr.sin_addr.s_addr = inet_addr(addr_info->broadcast_ip);
	broadcastAddr.sin_port = htons(BROADCAST_PORT);

	char sendString[BCAST_MSG_SIZE];
	sprintf(sendString, "zdonik:%s:%d", addr_info->server_ip, addr_info->server_port);
	int sendStringLen = strlen(sendString);
	while (1) {
		if (sendto(socket_desc, sendString, sendStringLen, 0, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr)) != sendStringLen) {
			perror("Broadcast different number of bytes than expected");
			close(socket_desc);
			return 0;
		}

		sleep(BCAST_PERIOD);
	}

	return 0;
}
// End code

int start_broadcast_local_ip(int port)
{
	struct address_info *addr_info = (struct address_info *)malloc(sizeof(struct address_info));
	addr_info->server_port = port;
	if (get_addresses(addr_info->server_ip, addr_info->broadcast_ip) < 0)
		return -1;

	pthread_t thread_id;
	if (pthread_create(&thread_id, NULL, broadcast_thread, addr_info) < 0) {
		perror("Could not create thread");
		return -1;
	}
	puts("Broadcast started");
	
    return 0;
}

int start_server(int port, void (*on_receive)(int, int *, const char *, void *), void *param)
{
	// Create socket
	int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1) {
		perror("Could not create socket");
		return -1;
	}
	
	// Prepare the sockaddr_in structure
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);
	
	// Bind
	if(bind(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Socket bind failed");
		return -1;
	}
	puts("Socket bind complete");
	
	// Listen
	listen(socket_desc, 5);
	puts("Waiting for incoming connections...");
	
	// Start broadcasting ip
	start_broadcast_local_ip(port);
	
	// Accept an incoming connection
	int new_socket;
	int sockaddr_size = sizeof(struct sockaddr_in);
	struct sockaddr_in client;
	while((new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&sockaddr_size))) {
		puts("Connection accepted");
		
		// Creating thread for client handler
		pthread_t thread_id;
		struct receive_info *rec_info = (struct receive_info *)malloc(sizeof(struct receive_info));
		rec_info->socket = new_socket;
		rec_info->on_receive_ptr = on_receive;
		rec_info->param = param;
		if(pthread_create(&thread_id, NULL, connection_thread, rec_info) < 0) {
			perror("Could not create thread");
			return -1;
		}
		printf("Handler assigned to Client [%d]\n", new_socket);
	}
	
	if (new_socket < 0) {
		perror("Socket accept failed\n");
		close(new_socket);
		return -1;
	}
	
	return 0;
}
