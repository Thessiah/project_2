#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
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
const std::map<char,int> RouterMap::port = RouterMap::create_map();
const std::map<int,char> NameMap::name = NameMap::create_map();

/*******************************************************************************
 *  FUNCTION PROTO-TYPES
 ******************************************************************************/
bool NameIsValid(int);
bool PortIsValid(int);
bool isMessage(std::string);
std::string constructDV(Router*);
std::string constructHeader(std::string, char, char, int, int);
std::string constructHeader(std::string, Router*, char, int);
void initRouter(Router*);
void initNeighbors(Router*);
void RequestDVFromNeighbors(Router*);
void send_cntl(Router*, char, int);
void send_data(Router*, char, int, std::string);
void send_rqst(Router*, char, int);
void SendDVToNeighbors(Router *);
void sendMsg(Router*, int, std::string);
void runDVAlgorithm(Router*, char, std::string);
void runDVAlgorithm(Router*, char, std::string, bool);

/*******************************************************************************
 *  MAIN
 *  - Check for correct instantiation (i.e. if port number is not provided,
 *      alert the user of proper usage)
 *  - Socket binding mumbo-jumbo
 *  - Read 'init.txt' and assign distances to neighbors
 *  - ??
 *  - Message format:
 *    | TYPE | SRC | DST | IN_PORT | OUT_PORT | PAYLOAD |
 *	  TYPE: 4 chars; either "DATA" or "CNTL" (control)
 *    SRC : 1 char ; link that the message originally came from
 *    DST : 1 char ; link that the message has to get to
 *    IN_PORT: 5 chars; (from the receiver's perspective) the port that the
 *        message was sent from
 *    OUT_PORT: 5 chars; (from the receiver's perspective) the port that the
 *        message was sent to (i.e. the receiver's port)
 *    PAYLOAD: rest of buffer; contains the good stuff
 ******************************************************************************/
int main(int argc, char** argv)
{
	struct sockaddr_in addr;
	int PORT_NUMBER;
	
	// Assign port number
	if (argc < 2)
	{
		std::cerr << "USAGE: ./udprouter <port number>; Please try again"
			<< std::endl;
		exit(0);
	}
	else
		PORT_NUMBER = atoi(argv[1]);

	// Router declaration
	Router router(PORT_NUMBER, NameMap::name.at(PORT_NUMBER));

	// Get socket file-descriptor
	if ((router.socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		std::cerr << "ERROR: Cannot create socket" << std::endl;
		exit(0);
	}

	// Assign port values
	memset((char *) &addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PORT_NUMBER);

	// Bind socket to port
	if (bind(router.socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		std::cerr << "ERROR: Bind failed for socket" << std::endl;
		exit(0);
	}

	// Initialize link-costs to neighbors
	initRouter(&router);
	initNeighbors(&router);

	// Wait for instructions
	char buf[BUFSIZE];
	long recvlen;
	struct sockaddr_in remaddr;
	socklen_t addrlen = sizeof(remaddr);

	for (;;)
	{
		printf("Router Port %d listening: \n", PORT_NUMBER);
		recvlen = recvfrom(router.socket, buf, BUFSIZE, 0, (struct sockaddr*)&remaddr, &addrlen);
		
		// SHUTDOWN message: if 'SHUTDOWN' is received, break out of loop and
		// terminate router
		if (strcmp(buf, "SHUTDOWN\n") == 0)
		{
			printf("Shutting down router on port %d...\n", PORT_NUMBER);
			break;
		}

		// UPDATE: change distance between node1 and node2
		else if (std::string(buf).find("UPDATE") == 0)
		{
			int distance;
			char node1, node2;
			
			sscanf(buf,"UPDATE: %c %c %d\n", &node1, &node2, &distance);
			if (node1==router.name && router.getPredecessorFor(node2)==router.name)
			{
				router.updateDistanceTo(node2, distance);
				SendDVToNeighbors(&router);
				RequestDVFromNeighbors(&router);
			}
		}

		// PRINTTABLE message: requests DVs from other nodes in network
		else if (strcmp(buf, "PRINTTABLE\n") == 0)
		{
			router.printTable();
		}
		
		// DV message: link cost in network has changed; update via DV-algorithm
		else if (isMessage(buf))
		{
			//printf("Received Message: \n\t%s\n", buf);
			
			std::string msg = buf;
			std::string dv = msg.substr(16,std::string::npos);
			bool cntl = (msg.substr(0,4)=="CNTL");
			bool rqst = (msg.substr(0,4)=="RQST");
			char src = msg[4];
			char dst = msg[5];
			int inport = atoi(msg.substr(6,5).c_str());
			int dstport = atoi(msg.substr(11,5).c_str());
			
			if (dst == router.name)
			{
				// if this is destination, run DV if "CNTL", print message if "DATA"
				if (cntl)
				{
					runDVAlgorithm(&router, src, msg.substr(16,std::string::npos));
				}
				else if (rqst)
				{
					send_cntl(&router, i2c(inport-10000), inport);
					printf("\tRespond to request from %d\n", inport);
				}
				else
				{
					std::cout << getTime() << std::endl;
					printf("\tSource: %c\n\tDestination: %c\n\tArrival Port: %d\n\t"
						   "Destination Port: %d\n\tMessage:\n\t  %s", src, dst,
						   inport, dstport, msg.substr(16,std::string::npos).c_str());
				}
			}
			// if destination is not this, forward msg
			else
			{
				std::cout << getTime() << std::endl;
				printf("\tSource: %c\n\tDestination: %c\n\tArrival Port: %d\n\t"
					   "Destination Port: %d\n", src, dst, inport, dstport);
				std::string header = constructHeader(msg.substr(0,4), src, dst, dstport,
													 router.getNextHopFrom(dst));
				sendMsg(&router, router.getNextHopFrom(dst),
						header+msg.substr(16,std::string::npos));
			}
		}
		else
		{
			printf("Message format unrecognized\n");
			continue;
		}
		
		std::fill(buf, buf+strlen(buf), 0);
	}

	// Deallocate socket resources
	close(router.socket);


	return 0;
}

/*******************************************************************************
 *  FUNCTION INITIALIZATIONS
 *  - NameIsValid()
 *  - PortIsValid()
 *  - isMessage()
 *  - constructDV()
 *  - constructHeader()
 *  - initRouter()
 *  - initNeighbors()
 *  - send_cntl()
 *  - sendMsg()
 *  - runDVAlgorithm()
 ******************************************************************************/

bool NameIsValid(char key)
{
	return (key==NameMap::name.at(RouterMap::port.at(key)));
}

bool PortIsValid(char key, int port)
{
	return (RouterMap::port.at(key)==port);
}

bool isMessage(std::string buf)
{
	return (buf.substr(0,4)=="CNTL" || buf.substr(0,4)=="DATA" ||
			buf.substr(0,4)=="RQST");
}

std::string constructHeader(std::string type, Router *src, char dst, int dst_port)
{
	char fromport[6], dstport[6], header[17];
	sprintf(fromport, "%d", src->port);
	sprintf(dstport, "%d", dst_port);
	sprintf(header, "%s%c%c%s%s", type.c_str(), src->name, dst, fromport, dstport);
	
	return header;
}

std::string constructHeader(std::string type, char src, char dst, int inport, int outport)
{
	char fromport[6], dstport[6], header[17];
	sprintf(fromport, "%d", inport);
	sprintf(dstport, "%d", outport);
	sprintf(header, "%s%c%c%s%s", type.c_str(), src, dst, fromport, dstport);
	
	return header;
}

std::string constructDV(Router *r)
{
	char buf[16];
	std::string payload = "";
	for (std::map<char, link_info_t>::iterator i = r->getTableBegin();
		 i != r->getTableEnd(); i++)
	{
		sprintf(buf, "%c %d\n", i->first, i->second.cost);
		payload += buf;
	}
	
	return payload;
}

void initRouter(Router* router)
{
	char src, dst;
	int  dstPort, distance;
	std::string buf;
	
	std::ifstream init("init.txt");
	if (init.is_open())
	{
		while (getline(init, buf))
		{
			sscanf(buf.c_str(), "%c,%c,%d,%d\n", &src, &dst, &dstPort,
				   &distance);
			
			if (router->name==src)
			{
				if (NameIsValid(src) && NameIsValid(dst) &&
					PortIsValid(dst, dstPort))
				{
					// TODO: save data into node's distanceTo array
					router->setDistanceTo(dst, src, distance, dstPort);
				}
				else
				{
					std::cerr << "ERROR: Invalid initialize statement\n\t" << buf
					<< std::endl;
					exit(0);
				}
			}
		}
	}
	else
		std::cerr << "Unable to read file" << std::endl;
}

void initNeighbors(Router* router)
{
	// Send out distance vectors to all neighbors
	for (std::map<char, link_info_t>::iterator i = router->getTableBegin();
		 i != router->getTableEnd(); i++)
	{
		send_cntl(router, i->first, i->second.next_hop);
	}
}

void RequestDVFromNeighbors(Router *router)
{
	for (std::map<char, link_info_t>::iterator i = router->getTableBegin();
		 i != router->getTableEnd(); i++)
	{
		if (i->second.predecessor == router->name)
			send_rqst(router, i->first, i->second.next_hop);
	}
}

void send_cntl(Router *src, char dst, int dst_port)
{
	// Construct Header
	std::string header = constructHeader("CNTL", src, dst, dst_port);
	
	// Construct Payload
	std::string payload = constructDV(src);
	
	// Send message
	sendMsg(src, dst_port, header+payload);
}

void send_rqst(Router *src, char dst, int dst_port)
{
	// Construct Header
	std::string header = constructHeader("RQST", src, dst, dst_port);
	
	// Send message
	sendMsg(src, dst_port, header);
}

void SendDVToNeighbors(Router *router)
{
	for (std::map<char, link_info_t>::iterator i = router->getTableBegin();
		 i != router->getTableEnd(); i++)
	{
		if (i->second.predecessor == router->name)
			send_cntl(router, i->first, i->second.next_hop);
	}
}

void sendMsg(Router* router, int dst_port, std::string msg)
{
	int clientSock = router->socket;
	char address[BUFSIZE] = "localhost";
	struct hostent *dst_router;
	struct sockaddr_in dst_addr;
	
	// Find destination info
	dst_router = gethostbyname(address);
	if (!dst_router)
	{
		fprintf(stderr,"ERROR: Could not obtain address of %d\n",dst_port);
		return;
	}
	
	dst_addr.sin_family = AF_INET;
	memcpy((void *)&dst_addr.sin_addr, dst_router->h_addr_list[0], dst_router->h_length);
	dst_addr.sin_port = htons(dst_port);
	
	// Send message to client
	if (sendto(clientSock,msg.c_str(),msg.length(),0,
			   (struct sockaddr *)&dst_addr,sizeof(dst_addr)) < 0)
	{
		fprintf(stderr,"ERROR: 'sendto' failed\n");
		return;
	}
}

void runDVAlgorithm(Router *router, char src, std::string dv)
{
	runDVAlgorithm(router, src, dv, false);
}

void runDVAlgorithm(Router *router, char src, std::string dv, bool dvChanged)
{
	// Extract values from DV (taken from isDV())
	char id;
	int distance, old_dist, new_dist, idx=0;
	std::map<char,int> distanceFromSrcTo;
	std::string buf, name="";
	std::stringstream dv_stream(dv);
	
	// Extract DV information
	while (getline(dv_stream, buf))
	{
		sscanf(buf.c_str(), "%c %d", &id, &distance);
		distanceFromSrcTo[id] = distance;
		name += id;
		idx++;
	}
	
	// for each node (A-F), compute new link cost using new distance table
	for (idx = 0; idx < name.size(); idx++)
	{
		old_dist = router->getDistanceTo(name[idx]);
		new_dist = router->getDistanceTo(src)+distanceFromSrcTo[name[idx]];
		
		if (new_dist > 0 && old_dist > new_dist)
		{
			router->setDistanceTo(name[idx], src, new_dist, router->getNextHopFrom(src));
			dvChanged = true;
		}
	}
	
	// if received dv has fewer entries than local dv, bcast local dv to neighbors
	if (name.size() < router->getTableSize())
	{
		dvChanged = true;
	}
	
	// if dv changed at all
	if (dvChanged)
	{
		SendDVToNeighbors(router);
		router->printTable();
	}
}

