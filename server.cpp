#include "server.h"
#include <sys/time.h>
#include <sys/types.h>

/*
 * Method to listen to incoming data.
 * @Input: serverinfo *serverinfo_: Pointer to struct serverinfo.
 * @Return: void 
 */
void start_server(serverinfo *serverinfo_)
{
	#ifdef _DEBUG_
	printf("start_server success %ld \n",serverinfo_->update_interval);
	#endif //#ifdef _DEBUG_
	fd_set master;    // master file descriptor list
	fd_set read_fds;  // temp file descriptor list for select()
	int fdmax;        // maximum file descriptor number
	struct timeval timeout;
	char buf[512];// buffer for recv
	int nbytes, noOfPackets = 0;
	int updatercvd = 0;
	timespec time1;
	
	FD_ZERO(&master);
    FD_ZERO(&read_fds);
	// add the listener to the master set
    FD_SET(serverinfo_->listen_sock, &master);
	FD_SET(0, &master);
	// keep track of the biggest file descriptor
    fdmax = serverinfo_->listen_sock;
	timeout.tv_sec = serverinfo_->update_interval;
	timeout.tv_usec = 0;

	while(1)
	{
		#ifdef _DEBUG_
		printf("timeout left = %ld \n",timeout.tv_sec);
		#endif //#ifdef _DEBUG_
		memset(buf,0,sizeof(buf)/sizeof(buf[0]));
		read_fds = master; // copy it
		if (select(fdmax+1, &read_fds, NULL, NULL, &timeout) == -1) {
			printf("select error %d \n",errno);
			return;
		}
		
		if(FD_ISSET(0, &read_fds))
		{
			// Read from stdin
			nbytes = read(0,buf,sizeof buf);
			buf[nbytes-1] = '\0';
			strtok (buf," ");
			if((0 == strcmp(buf,"UPDATE"))||
					(0 == strcmp(buf,"update")))
			{
				#ifdef _DEBUG_
				printf("update command received %d \n",serverinfo_->serverid);
				#endif //#ifdef _DEBUG_
				char *ptr = strtok (NULL, " ");
				if(ptr == NULL || (atoi(ptr) != serverinfo_->serverid))
				{
					printf("update invalid command \n");
					continue;
				}
				ptr = strtok (NULL, " ");
				if(ptr == NULL)
				{
					printf("update invalid command \n");
					continue;
				}
				int n = atoi(ptr);
				ptr = strtok (NULL, " ");
				if(ptr == NULL)
				{
					printf("update invalid command \n");
					continue;
				}
				if(updatercvd)
				{
					if(0 == strcmp(ptr,"inf"))
						UpdateDV(serverinfo_,n,INT_MAX);
					else
						UpdateDV(serverinfo_,n,atoi(ptr));
				}
				else
				{
					serverinfo_->nhop[n] = n;
					if(0 == strcmp(buf,"inf"))
						serverinfo_->dv[n] = INT_MAX;
					else
						serverinfo_->dv[n] = atoi(ptr);
				}
				printf("update SUCCESS \n");
			}
			else if((0 == strcmp(buf,"STEP"))||
					(0 == strcmp(buf,"step")))
			{
				#ifdef _DEBUG_
				printf("step command received \n");
				#endif //#ifdef _DEBUG_
				SendUpdate(serverinfo_);
				memset(&timeout,0,sizeof(struct timeval));
				timeout.tv_sec = serverinfo_->update_interval;
				timeout.tv_usec = 0;
				printf("step SUCCESS \n");
			}
			else if((0 == strcmp(buf,"PACKETS"))||
					(0 == strcmp(buf,"packets")))//TODO increment
			{
				printf("No. of distance vector packets received %d \n",noOfPackets);
				noOfPackets = 0;
				printf("packets SUCCESS \n");
			}
			else if((0 == strcmp(buf,"DISPLAY"))||
					(0 == strcmp(buf,"display")))
			{
				#ifdef _DEBUG_
				printf("display command received \n");
				#endif //#ifdef _DEBUG_
				Display(serverinfo_);
				printf("display SUCCESS \n");
			}
			else if((0 == strcmp(buf,"DISABLE"))||
					(0 == strcmp(buf,"disable")))
			{
				#ifdef _DEBUG_
				printf("disable command received \n");
				#endif //#ifdef _DEBUG_
				char *ptr = strtok (NULL, " ");
				if(ptr == NULL)
				{
					printf("disable invalid command \n");
					continue;
				}
				int n = atoi(ptr);
				routingtable *routingtable_ = serverinfo_->routingtable_;
				while(routingtable_)
				{
					if( (routingtable_->id == n) && (routingtable_->isneighbor))
					{
						routingtable_->isdisabled = true;
						routingtable *iterate = serverinfo_->routingtable_;
						while(iterate)
						{
							iterate->dvofid[n] = INT_MAX;
							UpdateMin(serverinfo_,iterate->dvofid,iterate->id);
							iterate = iterate->next;
						}
						//UpdateMin(serverinfo_,routingtable_->dvofid,n);
						break;
					}
					routingtable_ = routingtable_->next;
				}
				if(routingtable_ == NULL)
					printf("%d is not neighbour of %d \n",n,serverinfo_->serverid);
				else
					printf("disable SUCCESS \n");
			}
			else if((0 == strcmp(buf,"CRASH"))||
					(0 == strcmp(buf,"crash")))
			{
				printf("crash SUCCESS\n");
				while(1);
			}
			else
			{
				printf("This input is not supported \n");
			}
		}
		else if (FD_ISSET(serverinfo_->listen_sock,&read_fds))
		{
			#ifdef _DEBUG_
			printf("Received some data on socket \n");
			#endif //#ifdef _DEBUG_
			struct sockaddr_storage addr;
			socklen_t remotelen = sizeof(addr);
			char server_buffer[1024];
			nbytes = recvfrom(serverinfo_->listen_sock,server_buffer,1024,0,
						(struct sockaddr *)&addr,&remotelen);
			if(nbytes > 0)
			{
				noOfPackets++;
				updatercvd = 1;
				clock_gettime(CLOCK_REALTIME,&time1);
				int i = 0;
				int fixedcost = INT_MAX;
				socketstruct *socketstruct_ = (socketstruct *)server_buffer;
				routingtable *routingtable_ = serverinfo_->routingtable_;
				routingtable *idptr = NULL;
				std::map<int,int>::iterator itr;
				fixedcost = serverinfo_->neighbordis[socketstruct_->selfid];
				while(routingtable_)
				{
					if(socketstruct_->selfid == routingtable_->id)
					{
						routingtable_->lastupdate = time1;
						idptr = routingtable_;
						idptr->rcvdcost[routingtable_->id] = 0;
						break;
					}
					routingtable_ = routingtable_->next;
				}
				if(!routingtable_->isdisabled)
				{
					printf("RECEIVED A MESSAGE FROM SERVER %d\n",socketstruct_->selfid);
					while(i<socketstruct_->fields)
					{
						#ifdef _DEBUG_
						printf("ip %s id %d cost %d\n",socketstruct_->pktdetails_[i].ip,
							socketstruct_->pktdetails_[i].id,socketstruct_->pktdetails_[i].cost);
						#endif //#ifdef _DEBUG_
						routingtable_ = serverinfo_->routingtable_;
						if( socketstruct_->pktdetails_[i].id != serverinfo_->serverid)
						{
							int recvdcost = socketstruct_->pktdetails_[i].cost;
							while(routingtable_)
							{
								if(socketstruct_->pktdetails_[i].id == routingtable_->id)
								{
									#ifdef _DEBUG_
									printf("fixedcost %d, rcvdcost %d\n",fixedcost,recvdcost);
									#endif //#ifdef _DEBUG_
									if(recvdcost != INT_MAX && ((recvdcost+fixedcost) >= 0))
									{
										idptr->rcvdcost[routingtable_->id] = recvdcost;
										routingtable_->dvofid[socketstruct_->selfid] = recvdcost+fixedcost;
									}
									else
									{
										idptr->rcvdcost[routingtable_->id] = INT_MAX;
										routingtable_->dvofid[socketstruct_->selfid] = INT_MAX;
									}
									UpdateMin(serverinfo_,routingtable_->dvofid,socketstruct_->pktdetails_[i].id);
									break;
								}
								routingtable_ = routingtable_->next;
							}
						}
						i++;
					}
				}
				#ifdef _DEBUG_
				printf("\n*****\n");
				#endif //#ifdef _DEBUG_
				//Display(serverinfo_);
			}
			else
			{
				printf("bytes received < 0 with error %d \n",errno);
			}
		}
		else
		{
			#ifdef _DEBUG_
			printf("Send and update now \n");
			#endif //#ifdef _DEBUG_
			SendUpdate(serverinfo_);
			memset(&timeout,0,sizeof(struct timeval));
			timeout.tv_sec = serverinfo_->update_interval;
			timeout.tv_usec = 0;
		}
		
	};
}
