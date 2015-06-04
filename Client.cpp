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
	int clientSock;
	struct hostent *server;
	struct sockaddr_in servaddr;
	char message[BUFSIZE];
	
	// Ensure correct usage
	if (argc < 3)
	{
		fprintf(stderr,"USAGE: %s <hostname> <port number>\n",argv[0]);
		exit(0);
	}
	
	// Find server info
	server = gethostbyname(argv[1]);
	if (!server)
	{
		fprintf(stderr,"ERROR: Could not obtain address of %s\n",argv[1]);
		return 0;
	}
	
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
	
	// Send message to client
	printf("Please enter the message: ");
	bzero(message,BUFSIZE);
	fgets(message,BUFSIZE,stdin);

	// Band-aid fix for updating routers
	if (std::string(message).find("UPDATE") == 0)
	{
		int distance, port1, port2;
		char node1, node2, buf1[BUFSIZE], buf2[BUFSIZE];
		
		sscanf(message,"UPDATE: %c %c %d\n", &node1, &node2, &distance);
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
		
		sendto(clientSock, buf1, strlen(buf1), 0,
			   (struct sockaddr *)&servaddr1, sizeof(servaddr1));
		printf("First msg sent; ");
		sendto(clientSock, buf2, strlen(buf2), 0,
			   (struct sockaddr *)&servaddr2, sizeof(servaddr2));
		printf("Second msg send\n");


		return 0;
	}
	
	// Generate destination address information
	servaddr.sin_family = AF_INET;
	memcpy((void *)&servaddr.sin_addr, server->h_addr_list[0], server->h_length);
	servaddr.sin_port = htons(atoi(argv[2]));
	
	// Send message to destination router
	if (sendto(clientSock,message,strlen(message),0,
			   (struct sockaddr *)&servaddr,sizeof(servaddr)) < 0)
	{
		fprintf(stderr,"ERROR: 'sendto' failed\n");
		return 0;
	}
	

	return 0;
}
