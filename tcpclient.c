#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define MAX_LINE 20

int main (int argc, char *argv[]) {
  char* host_addr = argv[1];
  int port = atoi(argv[2]);

  /* Open a socket */
  int s;
  if((s = socket(PF_INET, SOCK_STREAM, 0)) <0){
    perror("simplex-talk: socket");
    exit(1);
  }

  /* Config the server address */
  struct sockaddr_in sin;
  sin.sin_family = AF_INET; 
  sin.sin_addr.s_addr = inet_addr(host_addr);
  sin.sin_port = htons(port);
  // Set all bits of the padding field to 0
  memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));

  /* Connect to the server */
  if(connect(s, (struct sockaddr *)&sin, sizeof(sin))<0){
    perror("simplex-talk: connect");
    close(s);
    exit(1);
  }
  
    //we create first message "HELLO X"
  	char d[] = "HELLO ";
    strcat(d,argv[3]);
    int len = strlen(d)+1;
	//SENDING "HELLO X" to server
    send(s, d, len, 0);
  
	//receive response from server
    char buf[MAX_LINE];
    len = recv(s, buf, sizeof(buf), 0);
	//print "HELLO Y"
	printf("%s\n", buf);
	fflush(stdout);
	
	int myY = atoi(argv[3])+1;
	//extract Y from received message
	char number[10];
	memcpy(number, &buf[6], strlen(buf)-6);
	number[strlen(buf)-6] = '\0';
	//convert char* to int
	int recY = atoi(number);
	
	if(myY == recY){
		//create "HELLO Z" message
		char message[] = "HELLO ";
		char z[5];
        sprintf(z, "%d", myY+1);
		strcat(message,z);
		len = strlen(message)+1;
		//send message to server
		send(s, message, len, 0);
		//close connection
		close(s);
	}
	else
		{
		//send MESSAGE and close connection
		char *message = "ERROR";
		len = strlen(message)+1;
		send(s, message, len, 0);
		close(s);
	}
  return 0;
}