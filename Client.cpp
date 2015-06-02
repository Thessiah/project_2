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


/*******************************************************************************
 *  CONSTANT DEFINITIONS
 ******************************************************************************/
#define BUFSIZE		2048
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
	
	servaddr.sin_family = AF_INET;
	memcpy((void *)&servaddr.sin_addr, server->h_addr_list[0], server->h_length);
	servaddr.sin_port = htons(atoi(argv[2]));
	
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
	
	if (sendto(clientSock,message,strlen(message),0,
			   (struct sockaddr *)&servaddr,sizeof(servaddr)) < 0)
	{
		fprintf(stderr,"ERROR: 'sendto' failed\n");
		return 0;
	}
	

	return 0;
}
