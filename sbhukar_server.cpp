#include "server.h"

int CreateSocket(int port);

/*
 * Method to start the program.
 * @Input: int argc - number of arguments
 * 		   char *argv[] - Pointer to argument array
 * @Return: int - Success
 * 				  Failure 
 */
int main(int argc,char *argv[])
{
	int ch;
	char *topology_file = NULL;
	FILE *pFile = NULL;
	char *line = NULL, *pch = NULL;
	size_t len = 0;
	int params[2];
	char *serverip_port[6];
	int i = 0;
	int serverid= 0 ;
	int temp = 0;
	serverinfo serverinfo_;
	routingtable *current = NULL;
	timespec time1;
	for(i;i<6;i++)
	{
		serverip_port[i] = NULL;
	}
	i = 0;
	if(argc != 5)
	{
		printf("wrong number of parameter \n");
		exit(0);
	}
	while ((ch = getopt(argc, argv, "t:i:")) != -1)
	{
		switch(ch) {
			case 't':
				topology_file = optarg;
				break;
			case 'i':
				serverinfo_.update_interval = atoi(optarg);
				break;
			default:
				printf("Wrong input parameters \n");
				exit(0);
		}
	}
	argc -= optind;
	if(argc != 0  || topology_file == NULL)
	{
		printf("Wrong input parameters \n");
		exit(1);
	}
	if(topology_file != NULL)
	{
		pFile = fopen(topology_file,"rb");
	}
	if(pFile == NULL)
	{
		printf("unable to open topology file, error %d \n",errno);
		exit(0);
	}
	clock_gettime(CLOCK_REALTIME,&time1);
	while (getline(&line, &len, pFile) != -1) {
	  // printf("line = %s and len = %d \n", line, len);
	   if(i<2)
	   {
		   params[i++] = atoi(line);
		   free(line);
		   line = NULL;
	   }
	   else if(i>=2 && (i<params[0]+2))
	   {
		   len = strlen(line);
		   serverip_port[i-2] = (char *)calloc(len+1,sizeof(char));
		   strncpy(serverip_port[i-2],line,len);
		   free(line);
		   line = NULL;
		   i++;
	   }else if( (i>=2+params[0]) && (i<2+params[0]+params[1]))
	   {
		   #ifdef _DEBUG_
		   printf ("%s %d %d %d \n",line, i, params[0], params[1]);
		   #endif //#ifdef _DEBUG_
		   pch = strtok (line," ");
		   serverinfo_.serverid = atoi(pch);
		   pch = strtok (NULL, " ");
		   temp = atoi(pch);
		   pch = strtok (NULL, " ");
		   serverinfo_.dv[temp] = atoi(pch);
		   serverinfo_.neighbordis[temp] = atoi(pch);
		   serverinfo_.nhop[temp] = temp;
		   if(current != NULL)
		   {
			   routingtable *routingtable_ = new routingtable;
			   routingtable_->id = temp;
			   routingtable_->isneighbor = true;
			   routingtable_->nexthop = temp;
			   routingtable_->dvofid[temp] = atoi(pch);
			   routingtable_->lastupdate = time1;
			   //printf("%ld lastupdate &id %d \n",routingtable_->lastupdate.tv_sec,routingtable_->id);
			   current->next = routingtable_;
			   current = current->next;
		   }
		   else
		   {
			   serverinfo_.routingtable_ = new routingtable;
			   serverinfo_.routingtable_->id = temp;
			   serverinfo_.routingtable_->isneighbor = true;
			   serverinfo_.routingtable_->nexthop = temp;
			   serverinfo_.routingtable_->dvofid[temp] = atoi(pch);
			   serverinfo_.routingtable_->lastupdate = time1;
			   //printf("%ld lastupdate &id %d \n",serverinfo_.routingtable_->lastupdate.tv_sec,serverinfo_.routingtable_->id);
			   current = serverinfo_.routingtable_;
		   }
		   i++;
	   }
   }
   serverinfo_.dv[serverinfo_.serverid] = 0;
   serverinfo_.nhop[serverinfo_.serverid] = serverinfo_.serverid;
   i = 0;
   //Search for serverid
   pch = strtok (serverip_port[i]," ");
   while (pch != NULL)
   {
	   if(serverinfo_.serverid == atoi(pch))
	   {
		   pch = strtok(NULL," ");
		   serverinfo_.serverip = pch;
		   pch = strtok(NULL," ");
		   serverinfo_.serverport = atoi(pch);
		   //pch = strtok(NULL," ");
	   }
	   else
	   {
		   int id = atoi(pch);
		   pch = strtok(NULL," ");
		   routingtable *prev;
		   routingtable *routingtable_ = serverinfo_.routingtable_;
		   while(routingtable_)
		   {
			   prev = routingtable_;
			   if(routingtable_->id == id)
			   {
				   routingtable_->ip = pch;
				   pch = strtok(NULL," ");
				   routingtable_->port = atoi(pch);
				   routingtable_->isdisabled = false;
				   break;
			   }
			   routingtable_ = routingtable_->next;
		   }
		   if(routingtable_ == NULL)
		   {
			   routingtable *routingtable_ = new routingtable;
			   routingtable_->id = id;
			   serverinfo_.dv[id] = INT_MAX;
			   serverinfo_.neighbordis[id] = INT_MAX;
			   serverinfo_.nhop[id] = id;
			   routingtable_->ip = pch;
			   pch = strtok(NULL," ");
			   routingtable_->port = atoi(pch);
			   routingtable_->isneighbor = false;
//			   routingtable_->cost = INT_MAX;
			   routingtable_->isdisabled = false;
			   prev->next = routingtable_;
		   }
	   }
	   i++;
	   if(serverip_port[i] != NULL)
			pch = strtok (serverip_port[i], " ");
	   else
			pch = NULL;
   }
   
   serverinfo_.listen_sock = CreateSocket(serverinfo_.serverport);
   if(serverinfo_.listen_sock == 0)
   {
		printf("main: CreateSocket error %d \n",errno);
		return 0;
   }
   
   start_server(&serverinfo_);
   
   return 1;
}

/*
 * Method to create a listening socket.
 * @Input: int port - listening port 
 * @Return: int - socket fd
 */
int CreateSocket(int port)
{
	struct sockaddr_in serv;
	socklen_t  s;

	memset((void *)&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;

	serv.sin_port = htons(port);

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("socket creation failed with error %d",errno);
		return 0;
	}
	if (bind(s, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
		printf("socket bind failed with error %d",errno);
		return 0;
	}
	return s;
}

