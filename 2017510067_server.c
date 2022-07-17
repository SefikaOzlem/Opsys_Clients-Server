#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 100
#define MAX_GROUPS 100
#define BUFFER_SZ 2048
#define LENGTH 2048

static _Atomic unsigned int counter_cli = 0;
static int uid = 10; // Client starting user_id

typedef struct  // CLIENT STRUCTURE
{
	struct sockaddr_in address;
	int uid;
	int sockfd;
	int group_id;
	char groupname[32];
	char phone_number[32];
} 
type_clients;
type_clients *temp_clients[MAX_CLIENTS]; //array with client properties
pthread_mutex_t temp_clients_mutex = PTHREAD_MUTEX_INITIALIZER; //client runtime control mutex


typedef struct  //GROUP STRUCTURE
{
	int gr_id;
	char group_name[32];
	char password[32];
} 
group_t;
group_t *groups[MAX_GROUPS]; //array with group properties
pthread_mutex_t groups_mutex = PTHREAD_MUTEX_INITIALIZER;


void overwrite() {
    printf("\r%s", "=");
    fflush(stdout);
}
void replaceWordInText(const char *text, const char *oldWord, const char *newWord) // replace function for communication clients message text string
{
	int i = 0, cnt = 0;
	int len1 = strlen(newWord);
	int len2 = strlen(oldWord);
	for (i = 0; text[i] != '\0'; i++) 
	{
		if (strstr(&text[i], oldWord) == &text[i]) 
		{
			cnt++;
			i += len2 - 1;
		}
	}
	char *newString = (char *)malloc(i + cnt * (len1 - len2) + 1);
	i = 0;
	while (*text) 
	{
		if (strstr(text, oldWord) == text) 
		{
			strcpy(&newString[i], newWord);
			i += len1;
			text += len2;
		}
		else
		newString[i++] = *text++;
	}
		
}
void trim_str(char* array, int length) 
{
  int j;
  for (j = 0; j < length; j++) 
  { 
    if (array[j] == '\n') 
    {
      array[j] = '\0';
      break;
    }
  }
}
void client_address(struct sockaddr_in addr) //print client address
{
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

void add_cl(type_clients *cl) // Creating the client joining the system and adding to the queue
{
	pthread_mutex_lock(&temp_clients_mutex);
	for(int i=0; i < MAX_CLIENTS; ++i)
	{
		if(!temp_clients[i])
		{
			temp_clients[i] = cl;
			break;
		}
	}
	pthread_mutex_unlock(&temp_clients_mutex);
}

void pop_cl(int cl_uid) // emptying the location of the client who wants to exit the system
{
	pthread_mutex_lock(&temp_clients_mutex);
	for(int i=0; i < MAX_CLIENTS; ++i)
	{
		if(temp_clients[i])
		{
			if(temp_clients[i]->uid == cl_uid)
			{
				temp_clients[i] = NULL;
				break;
			}
		}
	}
	pthread_mutex_unlock(&temp_clients_mutex);
}

void messg_sender(char *str, int cl_uid) //The message written by the sender(client) is sent to other clients.
{
	pthread_mutex_lock(&temp_clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i)
	{
		if(temp_clients[i])
		{
			if(temp_clients[i]->uid != cl_uid)
			{
				if(write(temp_clients[i]->sockfd, str, strlen(str)) < 0)
				{
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&temp_clients_mutex);
}


void *client_temple(void *arg) //Providing communication between clients and forwarding to the server (handle all communication)
{
	char from_client_buff[BUFFER_SZ]; // array that holds incoming clients
	char jsonformat[LENGTH + 32] = {}; //temp to print json format 
	char phone_number[32]; //client phone_number
	char group_name[32]; // It keeps the names given to the groups created from client
	int tmp_flag = 0; //controller system array overwrite condition (break or not break)
    int info_temp=0; // controller whoami
	int greate_temp=0; // controller gcreate(group create)
	int joiny_temp=0; // controller join a group
	int ext_grp=0; // controller exit group 
	counter_cli++; // To count the number of clients
	type_clients *client = (type_clients *)arg; //Assign the argument to the client type variable
	group_t *group = (group_t *)arg; //Assign the argument to the group type variable
	if(recv(client->sockfd, phone_number, 32, 0) <= 0 || strlen(phone_number) < 2 || strlen(phone_number) >= 32-1) 
	{
		tmp_flag = 1;
	} 
	else
	{
		strcpy(client->phone_number, phone_number);
		sprintf(from_client_buff, "%s has joined common chat room \n", client->phone_number);
		printf("%s", from_client_buff);
		messg_sender(from_client_buff, client->uid); // other clients have been notified that a new client has entered the system
      
	}
	bzero(from_client_buff, BUFFER_SZ);
	while(1)
	{
		if (tmp_flag) {
			break;
		}
        if(info_temp)
		{
			continue;
		}
		if(greate_temp)
		{
			continue;
		}
		if(joiny_temp)
		{
			continue;
		}
		if(ext_grp)
		{
			continue;
		}
		int receive = recv(client->sockfd, from_client_buff, BUFFER_SZ, 0);
		if (receive > 0)
		{
			if(strlen(from_client_buff) > 0 && strstr(from_client_buff, "send") == 0) //send command and reporting message to server
			{
				messg_sender(from_client_buff, client->uid);
				trim_str(from_client_buff,strlen(from_client_buff));
				printf("\"send_message\":[{\"to\" : \"DEUCENG\",\"from\":\"%s\"}]\n", from_client_buff); // jsonformat for server system
			}
		} 
		else if (receive == 0 || strcmp(from_client_buff, "exit") == 0) // exit commond for client left the system
		{
			sprintf(from_client_buff, "%s has left common chat room\n", client->phone_number);
			printf("%s", from_client_buff);
			messg_sender(from_client_buff, client->uid);
			tmp_flag = 1;
		}
		else if (receive == 0 || strcmp(from_client_buff, "whoami") == 0) // client view own phone number
		{
			sprintf(from_client_buff, "%s Your phone number\n", client->phone_number);
			printf("%s", from_client_buff);
			messg_sender(from_client_buff, client->uid);
			greate_temp=1;
			
		}
		else if (strcmp(from_client_buff, "join") == 0) // client  join a group using group name and  password ( I could not not give group password)
		{
			sprintf(from_client_buff, "%s joined the group %s\n",client->phone_number, group->group_name);
			printf("%s", from_client_buff);
			messg_sender(from_client_buff, client->uid);
			joiny_temp=1;
			
		}
		else if (strcmp(from_client_buff, "exit_group") == 0) // client left the group that registered
		{
			sprintf(from_client_buff, "%s has left the group %s\n",client->phone_number, group->group_name);
			printf("%s", from_client_buff);
			messg_sender(from_client_buff, client->uid);
			ext_grp=1;
			
		}
		else if ( strstr(from_client_buff, "gcreate") == 0)// client can create group with group name
		{
			/* The variable of type group joins the group array.*/
			group_t *gr;
			pthread_mutex_lock(&groups_mutex);
			for(int i=0; i < MAX_GROUPS; ++i)
			{
				if(!groups[i])
				{
					groups[i] = gr;
						break;
				}
			}
			pthread_mutex_unlock(&groups_mutex);

			sprintf(from_client_buff, "The group named %s was created from %s\n", group->group_name);
			printf("%s", from_client_buff);
			messg_sender(from_client_buff, client->uid);
			info_temp=1;	
		}
		
		 else 
		 {
			printf("ERROR: -1\n");
			tmp_flag = 1;
		}

		bzero(from_client_buff, BUFFER_SZ);
	}
  
  close(client->sockfd); //client socket identifier is closed
  pop_cl(client->uid); // When the client leaves the system, its id is defined and removed.
  free(client); // Memory created for the client is freed if the client leaves the system
  counter_cli--;
  pthread_detach(pthread_self());
	return NULL;
}

int main(int argc)
{
	char *ip = "127.0.0.1"; //server will be hosted on localhost
	int port = 3205;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	pthread_t tid;

	//Data is set for the socket//
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);	
	
	signal(SIGPIPE, SIG_IGN); // ignore signal 
    if(bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) //control the addresses that are available on our host. 
	{
		perror("Binding failed");
		exit(1);
	}
    if (listen(listenfd, 10) < 0) 
	{
		perror("Socket listening failed");
		exit(1);
	}

	printf("*****LOGIN to DEU Signal*****\n");
	while(1)
	{
		socklen_t clilength = sizeof(client_addr);
		connfd = accept(listenfd, (struct sockaddr*)&client_addr, &clilength);
		
		if((counter_cli + 1) == MAX_CLIENTS) ///Control max clients is reached 
		{
			printf("Max clients reached. Rejected: ");
			client_address(client_addr);
			printf(":%d\n", client_addr.sin_port);
			close(connfd);
			continue;
		}
		
		type_clients *clients = (type_clients *)malloc(sizeof(type_clients)); //client size setting
		clients->address = client_addr; //client address setting
		clients->sockfd = connfd; // client socket descriptor setting
		clients->uid = uid++; // client user_id(user_id starting 10)
			
		add_cl(clients); // add client system 
		pthread_create(&tid, NULL, &client_temple, (void*)clients);// fork thread for client adding to system
			
		sleep(1);
	}
	exit(0);
}
