#ifndef Router_h
#define Router_h

#include <climits>
#include <ctime>
#include <map>
#include <string>
#include <vector>

#define NUM_ROUTERS		6
#define BUFSIZE			256


const int DEFAULT_PORT[NUM_ROUTERS] = { 10000, 10001, 10002, 10003, 10004, 10005 };
const char DEFAULT_NAME[NUM_ROUTERS+1] = "ABCDEF";

std::string getTime()
{
	char buffer[BUFSIZE];
	time_t t = time(0);
	struct tm *nw = localtime(&t);
	
	strftime(buffer, BUFSIZE, "[%H:%M:%S %Z] ", nw);
	return buffer;
}

char i2c(int i)
{
	return i+'A';
}

int  c2i(char c)
{
	return c-'A';
}


struct RouterMap
{
	static std::map<char,int> create_map()
	{
		std::map<char,int> m;
		for (int i=0; i<NUM_ROUTERS; i++)
			m[DEFAULT_NAME[i]] = DEFAULT_PORT[i];
		return m;
	}

	static const std::map<char,int> port;
};


struct NameMap
{
	static std::map<int,char> create_map()
	{
		std::map<int,char> m;
		for (int i=0; i<NUM_ROUTERS; i++)
			m[DEFAULT_PORT[i]] = DEFAULT_NAME[i];
		return m;
	}
	
	static const std::map<int,char> name;
};


struct link_info
{
	int cost;
	int next_hop;
	char predecessor;
	
	link_info()
	{
		cost = 0;
		next_hop = 0;
		predecessor = '_';
	}
	
	link_info(int c, int nh, char p)
	{
		cost = c;
		next_hop = nh;
		predecessor = p;
	}
};

typedef struct link_info link_info_t;

struct thread_data
{
	int thread_id;
	int origin;
	std::clock_t time;
	bool ack_recieved;
	struct Router* router;
};

/*******************************************************************************
 *  ROUTER CLASS DEFINITION
 *  - getDistanceTo()
 *  - printTable()
 ******************************************************************************/
class Router
{
  public:
	int socket;
	int port;
	char name;
	
	Router(int p, char c)
	{
		port = p;
		name = c;
	}
	
	char getPredecessorFor(char dst)
	{
		return fwd_table[dst].predecessor;
	}
	
	int getDistanceTo(char dst)
	{
		if (dst == this->name)
			return 0;
		else if (fwd_table.find(dst) == fwd_table.end())
			return INT_MAX;
		else
			return fwd_table[dst].cost;
	}
	
	int getNextHopFrom(char dst)
	{
		return fwd_table[dst].next_hop;
	}
	
	size_t getTableSize()
	{
		return fwd_table.size();
	}
	
	std::map<char, link_info_t>::iterator getTableBegin()
	{
		return this->fwd_table.begin();
	}
	
	std::map<char, link_info_t>::iterator getTableEnd()
	{
		return this->fwd_table.end();
	}
	
	void printTable()
	{
		// Table entries will be in the following format:
		//   <NAME> <LINK_COST> <PORT_NUMBER>
		//   - NAME: the name of the destination router
		//   - LINK_COST: the cost of sending a message to the destination router
		//   - PORT_NUMBER: the port number to the next link on the shortest path
		char n;
		printf("%s\n\tRouter %c Forwarding Table\n", getTime().c_str(),
			   this->name);
		for (int i=0; i<NUM_ROUTERS; i++)
		{
			n = i2c(i);
			if (fwd_table.find(n) != fwd_table.end())
				printf("\t  %c  %d     %d\n", n, fwd_table.at(n).next_hop,
					   fwd_table.at(n).cost);
		}
		printf("\n");
	}
	
	void setDistanceTo(char dst, char pre, int distance, int nh)
	{
		if (dst == this->name)
			return;
		
		fwd_table[dst] = link_info_t(distance, nh, pre);
	}
	
	void updateDistanceTo(char dst, int distance)
	{
		if (dst == this->name)
			return;
		
		/*
		int nh;
		while (pre != this->name && pre != '_')
			pre = fwd_table[pre].predecessor;
		nh = fwd_table[pre].next_hop;
		*/
		
		if (fwd_table.find(dst) == fwd_table.end())
		{
			std::cerr << "Huge ass error; not using this in the right spot" << std::endl;
			exit(0);
		}
		else
		{
			int nh = 10000+c2i(dst);
			for (std::map<char, link_info_t>::iterator i = this->getTableBegin();
				 i != this->getTableEnd(); i++)
			{
				if (i->second.next_hop == nh)
					i->second.cost = INT_MAX;
			}

			
			fwd_table[dst].cost = distance;
		}
	}


  private:
	std::map<char, link_info_t> fwd_table;
	
};

#endif
