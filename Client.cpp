#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "Router.h"


/*******************************************************************************
 *  CONSTANT DEFINITIONS
 ******************************************************************************/
#define CLIENTPORT	10020

/*******************************************************************************
 *  MAIN
 *  - Creates routers A-F
 *  - ??
 ******************************************************************************/
int main(int argc, const char * argv[])
{
	// Ensure correct usage
	if (argc != 4 && argc != 5)
	{
		printf("USAGE:\n"
			   "\t./client UPDATE <node1> <node2> <distance>  OR\n"
			   "\t./client MESSAGE <from_port> <destination_router_name>\n");
		return 0;
	}
	
	int clientSock;
	struct hostent *server;
	struct sockaddr_in servaddr;
	std::string testmsg = "This is a test\n";
	
	// Find server info
	server = gethostbyname("localhost");
	
	// Create client socket
	if ((clientSock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{
		fprintf(stderr,"ERROR: Cannot create client socket\n");
		return 0;
	}
	
	struct sockaddr_in clientaddr;
	memset((char *)&clientaddr,0,sizeof(clientaddr));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	clientaddr.sin_port = htons(CLIENTPORT);
	
	if (bind(clientSock, (struct sockaddr*)&clientaddr,
			 sizeof(clientaddr)) < 0)
	{
		fprintf(stderr,"ERROR: client bind failed\n");
		return 0;
	}
	
	// Band-aid fix for updating routers
	if (strcmp(argv[1],"UPDATE") == 0)
	{
		int distance, port1, port2;
		char node1, node2, buf1[BUFSIZE], buf2[BUFSIZE];
		
		node1 = *(argv[2]);
		node2 = *(argv[3]);
		distance = atoi(argv[4]);
		port1 = 10000+c2i(node1);
		port2 = 10000+c2i(node2);
		
		struct sockaddr_in servaddr1, servaddr2;
		servaddr1.sin_family = AF_INET;
		servaddr2.sin_family = AF_INET;
		memcpy((void *)&servaddr1.sin_addr, server->h_addr_list[0], server->h_length);
		memcpy((void *)&servaddr2.sin_addr, server->h_addr_list[0], server->h_length);
		servaddr1.sin_port = htons(port1);
		servaddr2.sin_port = htons(port2);

		sprintf(buf1, "UPDATE: %c %c %d\n", node1, node2, distance);
		sprintf(buf2, "UPDATE: %c %c %d\n", node2, node1, distance);
		
		if (sendto(clientSock, buf1, strlen(buf1), 0,
			   (struct sockaddr *)&servaddr1, sizeof(servaddr1)) < 0)
		{
			fprintf(stderr,"ERROR: 'sendto' (1) failed\n");
			return 0;
		}
		if (sendto(clientSock, buf2, strlen(buf2), 0,
			   (struct sockaddr *)&servaddr2, sizeof(servaddr2)) < 0)
		{
			fprintf(stderr,"ERROR: 'sendto' (2) failed\n");
			return 0;
		}

		return 0;
	}
	else if (strcmp(argv[1], "MESSAGE") == 0)
	{
		int from_port = 10000+*(argv[2])-'A';
		char destination = *(argv[3]);
		
		// Generate destination address information
		servaddr.sin_family = AF_INET;
		memcpy((void *)&servaddr.sin_addr, server->h_addr_list[0], server->h_length);
		servaddr.sin_port = htons(from_port);
		
		char header[BUFSIZE];
		sprintf(header, "DATAH%c%d%d", destination, CLIENTPORT, from_port);
		std::string msg = std::string(header)+testmsg;
		
		// Send message to destination router
		if (sendto(clientSock,msg.c_str(),msg.size(),0,
				   (struct sockaddr *)&servaddr,sizeof(servaddr)) < 0)
		{
			fprintf(stderr,"ERROR: 'sendto' failed\n");
			return 0;
		}
	}
	else
	{
		printf("USAGE:\n"
			   "\t./client UPDATE <node1> <node2> <distance>  OR\n"
			   "\t./client MESSAGE <from_this_router> <destination_router_name>\n");
		return 0;
	}
	

	return 0;
}
