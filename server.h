#include <stdio.h>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>
#include <map>
//#include <sys/timerfd.h>

//#define _DEBUG_
typedef struct RoutingTable
{
	int id; //Server Id 
	std::map<int,int> dvofid;//via,cost
	std::map<int,int> rcvdcost;//to,cost
	std::string ip; //IP of the srever
	int port; //Port number of the server
	int nexthop;// nexthop  to reach this server
	timespec lastupdate; //Time when last update was received
	bool isneighbor;// Is this a neighbor of server maintaining this table
	bool isdisabled;// Is this link disabled
	struct RoutingTable *next;
}routingtable;

typedef struct ServerInfo{
	int serverport;
	int listen_sock;
	int serverid;
	long update_interval;
	std::string serverip;
	std::map<int,int> neighbordis;//id, cost
	std::map<int,int> dv;//node, cost
	std::map<int,int> nhop;//node, nexthop
	routingtable *routingtable_;
}serverinfo;

typedef struct PktDetail{
	int id;
	unsigned char ip[sizeof(struct in_addr)];
	int port;
	int cost;
}pktdetails;

typedef struct SockStruct{
	short fields;
	short serverport;
	unsigned char serverip[sizeof(struct in_addr)];
	int selfid;
	int selfcost;
	pktdetails pktdetails_[5];
}socketstruct;

void start_server(serverinfo *serverinfo_);
void Display(serverinfo *serverinfo_);
void SendUpdate(serverinfo *serverinfo_);
void UpdateDV(serverinfo *serverinfo_, int node, int cost);
void UpdateMin(serverinfo *serverinfo_,std::map<int,int> dvofid, int node);
