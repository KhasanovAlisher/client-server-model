CC = gcc
CFLAGS  = -pthread


default: room_rent

room_rent:  z_net_server.o z_net_client.o server.o client.o 
	$(CC) $(CFLAGS) -o server z_net_server.o server.o
	$(CC) $(CFLAGS) -o client z_net_client.o client.o


z_net_server.o:  z_net_lib/z_net_server.c z_net_lib/z_net_server.h 
	$(CC) $(CFLAGS) -c z_net_lib/z_net_server.c
	
z_net_client.o:  z_net_lib/z_net_client.c z_net_lib/z_net_client.h 
	$(CC) $(CFLAGS) -c z_net_lib/z_net_client.c

server.o:  server.c z_net_lib/z_net_server.h 
	$(CC) $(CFLAGS) -c server.c

client.o:  client.c z_net_lib/z_net_client.h
	$(CC) $(CFLAGS) -c client.c


clean: 
	$(RM) server client *.o *~
