#include "server.h"

/*
 * Method to display routing table.
 * @Input: serverinfo *serverinfo_: Pointer to struct serverinfo.
 * @Return: void
 */
void Display(serverinfo *serverinfo_)
{
	std::map<int,int> dv = serverinfo_->dv;
	std::map<int,int>::iterator it;
	std::map<int,int>::iterator itr = serverinfo_->nhop.begin();
	for (it=serverinfo_->dv.begin(); it!=serverinfo_->dv.end(); ++it)
	{
		if(it->second != INT_MAX)
			printf("<%d> <%d> <%d> \n",it->first,itr->second,it->second);
		else
			printf("<%d> <N.A> <inf> \n",it->first);
		itr++;
	}
}

/*
 * Method to calculate minimum distance and update routing table.
 * @Input: serverinfo *serverinfo_ - Pointer to struct serverinfo.
 * 		   std::map<int,int> dvofid - Map of cost and id.
 * 		   int node - node to be updated
 * @Return: void
 */
void UpdateMin(serverinfo *serverinfo_,std::map<int,int> dvofid, int node)
{
	#ifdef _DEBUG_
	printf("node to be updated %d \n",node);
	#endif //#ifdef _DEBUG_
	int min = INT_MAX;
	int nexthop;
	std::map<int,int>::iterator it;
	std::map<int,int>::iterator itr = serverinfo_->nhop.begin();
	//printf("\n");
	for (it=dvofid.begin(); it!=dvofid.end(); ++it)
	{
		//printf("via %d, cost %d, min %d \n",it->first,it->second,min);
		if(min > it->second)
		{
			min = it->second;
			nexthop = it->first;
		}
	}
	serverinfo_->dv[node] = min;
	serverinfo_->nhop[node] = node;
	if(min != INT_MAX)
	{
		//printf("min != intmax nexthop %d \n",nexthop);
		serverinfo_->nhop[node] = nexthop;
	}
}

/*
 * Method to update distance vector on update command.
 * @Input: serverinfo *serverinfo_ - Pointer to struct serverinfo.
 * 		   int node - node whose cost needs to be updated
 * 		   int cost - cost to be updated
 * @Return: void 
 */
void UpdateDV(serverinfo *serverinfo_, int node, int cost)
{
	std::map<int,int>::iterator it;
	routingtable *routingtable_ = serverinfo_->routingtable_;
	while(routingtable_)
	{
		if(routingtable_->id == node)
		{
			#ifdef _DEBUG_
			printf("updateDV routingtable_->id = %d \n",routingtable_->id);
			#endif //#ifdef _DEBUG_
			serverinfo_->neighbordis[node] = cost;
			//UpdateMin(serverinfo_,routingtable_->dvofid,node);
			routingtable *iterate = serverinfo_->routingtable_;
			while(iterate)
			{
				if( (routingtable_->rcvdcost[iterate->id] != INT_MAX) && (cost != INT_MAX) )
				{
					iterate->dvofid[routingtable_->id] = routingtable_->rcvdcost[iterate->id] + cost;
				}
				else
					iterate->dvofid[routingtable_->id] = INT_MAX;
				UpdateMin(serverinfo_,iterate->dvofid,iterate->id);
				iterate = iterate->next;
			}
			break;
		}
		routingtable_ = routingtable_->next;
	}
}

/*
 * Method to send data on a socket to neighbors.
 * @Input: serverinfo *serverinfo_: Pointer to struct serverinfo.
 * @Return: void 
 */
void SendUpdate(serverinfo *serverinfo_)
{
	int index = 0;
	timespec time1;
	clock_gettime(CLOCK_REALTIME,&time1);
	routingtable *routingtable_ = serverinfo_->routingtable_;
	routingtable *iterate = NULL;
	
	//If 3 updates were missed than set the cost to inf
	while(routingtable_)
	{
		if( (routingtable_->lastupdate.tv_sec + 3*(serverinfo_->update_interval) ) < time1.tv_sec
			&& (routingtable_->isneighbor) )
		{
			#ifdef _DEBUG_
			printf("cost set to inf %ld lastupdate\n",routingtable_->lastupdate.tv_sec);
			printf("cost set to inf %ld serverinfo_->update_interval \n",serverinfo_->update_interval);
			printf("cost set to inf %ld time1.tv_sec\n",time1.tv_sec);
			#endif //#ifdef _DEBUG_
			iterate = serverinfo_->routingtable_;
			while(iterate)
			{
				iterate->dvofid[routingtable_->id] = INT_MAX;
				UpdateMin(serverinfo_,iterate->dvofid,iterate->id);
				iterate = iterate->next;
			}
		}
		routingtable_ = routingtable_->next;
	}
		
	std::map<int,int>::iterator it = serverinfo_->dv.begin();
	routingtable_ = serverinfo_->routingtable_;
	socketstruct *senddata = new socketstruct();
	senddata->serverport = serverinfo_->serverport;
	senddata->selfid = serverinfo_->serverid;
	senddata->selfcost = 0;
	inet_pton(AF_INET,(char *)serverinfo_->serverip.c_str() , &senddata->serverip);
	while(routingtable_)
	{
		inet_pton(AF_INET,(char *)routingtable_->ip.c_str() , &senddata->pktdetails_[index].ip);
		senddata->pktdetails_[index].port = routingtable_->port;
		senddata->pktdetails_[index].id = routingtable_->id;
		#ifdef _DEBUG_
		printf("for id %d cost is %d \n",routingtable_->id,serverinfo_->dv[routingtable_->id]);
		#endif //#ifdef _DEBUG_
		senddata->pktdetails_[index].cost = serverinfo_->dv[routingtable_->id];
		index++;
		routingtable_ = routingtable_->next;
	}
	senddata->fields = index;
	
	//std::string buffer = CreateMsg(ndv,serverinfo_->serverid);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	routingtable_ = serverinfo_->routingtable_;
	while(routingtable_)
	{
		#ifdef _DEBUG_
		printf("neighbor id %d and isneighbor %d \n",routingtable_->id, routingtable_->isneighbor);
		#endif //#ifdef _DEBUG_
		//inf cost or disabled link eventually results in same behavior
		if(routingtable_->isneighbor && (!routingtable_->isdisabled))
		{
			addr.sin_port = htons(routingtable_->port);
			inet_pton(AF_INET,(char *)routingtable_->ip.c_str() , &addr.sin_addr);
			//send update to this neighbor
			int bytes = sendto(serverinfo_->listen_sock,senddata,
					sizeof(struct SockStruct),0,(struct sockaddr *)&addr,sizeof(addr));
			#ifdef _DEBUG_
			printf("bytes sent %d \n",bytes);
			#endif //#ifdef _DEBUG_
			if(bytes < 0)
				printf("sendto function returned error %d \n",errno);
		}
		routingtable_ = routingtable_->next;
	}
}
