#include <stdio.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>


#define LENGTH 2048
#define PH_LENGTH 32

char phone_number[32]; // client phone number
int temp=0; // control print whoami command
int temp_gr=0; // control print gcreate (group create) command
int temp_join=0; // control print join command
int sockfd = 0; // listening socket descriptor
char password[32]; // Password assigned to join the group and when group create
volatile sig_atomic_t flag = 0; // significantly weaker guarantee flag 

void trim_str(char* array, int len) // trim function
{
  int j;
  for (j = 0; j < len; j++) 
  { 
    if (array[j] == '\n') 
    {
      array[j] = '\0';
      break;
    }
  }
}
int search(char str[], char word[]) //To find the index value of the desired word in string
{
    int l, i, j;
    /* finding length of word */
    for (l = 0; word[l] != '\0'; l++);
    for (i = 0, j = 0; str[i] != '\0' && word[j] != '\0'; i++)
    {
        if (str[i] == word[j])
        {
            j++;
        }
        else
        {
            j = 0;
        }
    }
    if (j == l)
    {
        /* substring found */
        return (i - j);
    }
    else
    {
        return  - 1;
    }
}
int delete_word(char str[], char word[], int index) //Deleting the word from the string according to the index value
{
    int i, l; 
    /* finding length of word */
    for (l = 0; word[l] != '\0'; l++);
    for (i = index; str[i] != '\0'; i++)
    {
        str[i] = str[i + l + 1];
    }
}
void overwrite() 
{
  printf("%s", "=> ");
  fflush(stdout);
}
void exit_cont(int sig) // Checking whether output is with signal value
{
    flag = 1;
}
void send_message()
{
  
  char buffer[LENGTH + PH_LENGTH] = {}; //storage message and clients of informations 
  char mssg_text[LENGTH] = {}; // sending message text 
  while(1) 
  {
  	overwrite();
    fgets(mssg_text, LENGTH, stdin); // input message 
    trim_str(mssg_text,LENGTH);
    if (strcmp(mssg_text, "exit") == 0) // client exit system
    {
			break;
    } 
    else if (strcmp(mssg_text, "whoami") == 0)  // client view phone number
    {
      sprintf(buffer, "%s: %s\n", phone_number, mssg_text);
      temp=1;
    } 
    else if (strstr(mssg_text,"gcreate")!=NULL) // group create command 
    {
      //fgets(password, LENGTH, stdin);
      int indexx;
      indexx=search(mssg_text,"gcreate");
      if (indexx !=  - 1)
      {
        delete_word(mssg_text, "gcreate", indexx);
      }
      
      sprintf(buffer, "The group named %s was created from %s\n", mssg_text, phone_number);
      send(sockfd, buffer, strlen(buffer), 0);
      
    }
    else if (strstr(mssg_text,"send")!=NULL) // send message command 
    {
      int index;
      index=search(mssg_text,"send");
      if (index !=  - 1)
      {
        delete_word(mssg_text, "send", index);
      }
      sprintf(buffer, "%s: %s\n", phone_number, mssg_text);
      send(sockfd, buffer, strlen(buffer), 0);
    }
    else if(strstr(mssg_text,"join")!=NULL) // client join defined a group with password
    {
      int indexim;
      indexim=search(mssg_text,"join");
      if(indexim!=-1)
      {
        delete_word(mssg_text, "join", indexim);
      }
      sprintf(buffer, "%s joined the group  %s\n", phone_number, mssg_text);
      send(sockfd, buffer, strlen(buffer), 0);
      temp_join=1;
    }
    else if(strstr(mssg_text,"exit_group")!=NULL) // client exit recorded group
    {
      int indeximm;
      indeximm=search(mssg_text,"exit_group");
      if(indeximm!=-1)
      {
        delete_word(mssg_text, "exit_group", indeximm);
      }
      sprintf(buffer, "%s has left the group  %s\n", phone_number, mssg_text);
      send(sockfd, buffer, strlen(buffer), 0);
      temp_gr=1;
    }
    else // not send,not gcreate,not join,not exit_group,not exit,not whoami
    {
      sprintf(buffer, "%s\n"," Do not use invalid expressions");
      send(sockfd, buffer, strlen(buffer), 0);
    }
    if(temp_join==1)
    {
      printf("You joined the group %s \n",mssg_text);
      temp_join=0;
    }
    if(temp_gr==1)
    {
      printf("You has left the group %s \n",mssg_text);
      temp_gr=0;
    }
    if(temp==1)
    {
      printf("Your phone number %s \n",phone_number);
      temp=0;
    }
		bzero(mssg_text, LENGTH); // erases the data in the n bytes of the memory starting at the location pointed to by s
    bzero(buffer, LENGTH + PH_LENGTH);
  }
  exit_cont(2); 
}
void receive_message() //receiving messages from other clients
{
	char mssg_text[LENGTH] = {};
  while (1) 
  {
		int receive = recv(sockfd, mssg_text, LENGTH, 0);//temp message empty or full
    if (receive > 0)
    {
      printf("%s", mssg_text);
      overwrite();
    } 
    else if (receive == 0) 
    {
			break;
    } 
		memset(mssg_text, 0, sizeof(mssg_text)); //set the sizeof(mssg_text) bytes starting at address mssg_text to the value 0
  }
}

int main(int argc)
{	
	char *ip = "127.0.0.1"; //server will be hosted on localhost
	int port = 3205;
  signal(SIGINT, exit_cont);
  printf("Please enter 11 characters your phone number : ");
  fgets(phone_number, PH_LENGTH, stdin);
  trim_str(phone_number,strlen(phone_number));
	if (strlen(phone_number) > 11|| strlen(phone_number) < 11)
  {
		printf("Your Phone number must be 11 characters.\n");
		exit(1);
	}
	struct sockaddr_in server_addr; // create server address using type sock address 
	/* Socket settings */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip);
  server_addr.sin_port = htons(port);
 
  int connect_error = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)); //control connect server
  if (connect_error == -1) 
  {
		printf("Not connected server-client \n");
		exit(1); // FAILURE
	}
	send(sockfd, phone_number, PH_LENGTH, 0); // send to server phone number using socket descriptor
	printf("*****LOGIN to DEU Signal*****\n");
	pthread_t message_thread;
  if(pthread_create(&message_thread, NULL, (void *) send_message, NULL) != 0)
  {
		printf("Message could not be sent\n");
    exit(1);
	}
	pthread_t receive_message_thread;
  if(pthread_create(&receive_message_thread, NULL, (void *) receive_message, NULL) != 0)
  {
		printf("Message could not be receive\n");
		exit(1);
	}
	while (1)
  {
		if(flag) // if entry exit command
    {
			break;
    }
    
	}
	close(sockfd);
	exit(0); //SUCCESS
}
