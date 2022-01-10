#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define MAX_CONN 100    // Maximum number of clients
#define MAX_LINE 32     // Maximum buffer length


/* Struct to store the client connections */
typedef struct client_t
{
	int sock;                 // file descriptor of socket 
	struct sockaddr_in addr;  // address of the server (remote) host 
	int X;                    // X val
	int Y;                    // Y val
	int Z;                    // Z val
} client_t;

/* Struct to store the server details*/
typedef struct server_t
{
	int sock;                   // Socket file descriptor
	struct sockaddr_in addr;    // address of local server
} server_t;

static server_t srv;                // Global instance of server
static client_t *clts[MAX_CONN];    // Array containing all client connections
static int n_clts = 0;              // Count of active client connections
static int max_fd;                  // To store the highest fd value 
char buf[MAX_LINE];                 // Buffer to store messages

/** 
 * Function that initializes a the server struct
 * @param port takes an integer port number to bind the socket to
 * @return returns 0 on success, -1 otherwise
 */
int init_server(int port) 
{
    memset(&srv, 0, sizeof(server_t));
	srv.sock = -1;
    
	int t = 1;
	int sock = -1;

    // Creating a socket file desciptor
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("simplex-talk: socket");
		goto error;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)) < 0) {
		perror("simplex-talk: setsockopt");
		goto error;
	}

    // Binding the server socket with a port
	memset(&srv.addr, 0, sizeof(srv.addr));
	srv.addr.sin_family = AF_INET;
	srv.addr.sin_port = htons(port);
	
	if (bind(sock, (const struct sockaddr *)&srv.addr, sizeof(srv.addr)) < 0) {
		perror("simplex-talk: bind");
		goto error;
	}

    // Begin listening in the socket
	if (listen(sock, 5) < 0){
		perror("simplex-talk: listen");
		goto error;
	}

	srv.sock = sock;
	return 0;
error:
	if (sock >= 0) close(sock);
	return -1;
}

/**
 * Function that creates a client struct 
 * @return returns pointer to the struct on success , NULL otherwise 
*/
client_t * init_client()
{
    client_t * new_c = (client_t *) malloc(sizeof(client_t));
    memset(new_c,0,sizeof(client_t));
    // Setting the default values 
    new_c->sock = -1;   
    new_c->X=INT_MIN;
    new_c->Y=INT_MIN;
    new_c->Z=INT_MIN;
    return new_c;
}

/**
 * Function that adds a new client connection to the array
 * @param s pointer to the server struct
 * @return return 0 on success, -1 otherwise
*/
int new_connection(server_t *s) 
{
    client_t * c = init_client();

	socklen_t len;
	int sock = -1;
    
    // Accept the connection and initialze the address of remote server
	len = sizeof(c->addr);
	sock = accept(s->sock, (struct sockaddr *)&c->addr, &len);
	if (sock < 0)
    {
		perror("simplex-talk: accept");
		goto error;
	}

	c->sock = sock;

    // make the file descriptor non blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | 04000);

    // Add connection to the first non empty array position
    for(int i=0;i<MAX_CONN;i++)
    {
        if(clts[i]==NULL)
        {
            clts[i] = c;
            break;
        }
    }
    
    // Increment the count of active client connections
    n_clts++;

	return 0;
error:
	if (sock >= 0) close(sock);
    free(c);
	return -1;
}

/**
 * Function that closes a client connection
 * @param i array index of where the client struct is stored
*/
void close_connection(int i)
{
    close(clts[i]->sock);
    free(clts[i]);    
    clts[i] = NULL;
    n_clts--;
}

/**
 * Function that handles the first shake with client 
 * @param client struct containing client details
*/
void handle_first_shake(client_t * client)
{
    // Recieve the first message and print it to stdout
    memset(buf,'\0',MAX_LINE);
    recv(client->sock , buf, sizeof(buf),0);
    printf("%s\n",buf);
    fflush(stdout);
    
    // Parse the message and extract X value
    char number[10];
    memcpy(number, &buf[6], strlen(buf)-6);
    number[strlen(buf)-6] = '\0';
    client->X = atoi(number);
    client->Y = client->X +1;

    // Write and send the message back to client
    char message[MAX_LINE] = "HELLO ";
    char num[10] = "";
    sprintf(num, "%d", client->Y); 
    strcat(message, num);

    send(client->sock, message, strlen(message)+1, 0);
}

/**
 * Function that handles second shake with client
 * @param client struct containing client details
*/
void handle_second_shake(client_t * client)
{
    // Recieve a message and print it to stdout
    memset(buf,'\0',MAX_LINE);
    recv(client->sock,buf, MAX_LINE ,0);
    printf("%s\n",buf);
    fflush(stdout);
}


int main(int argc, char *argv[])
{
    if(argc!=2)
    {
        printf("Usage: %s <port no>\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);

    // Initialize server
    init_server(port);
    // Initialize client array to NULL
    for(int i=0 ;i<MAX_CONN;i++) clts[i] = NULL;

    // Initialize fd_set
    fd_set fds;
    
    while(1)
    {
        max_fd = srv.sock;
        FD_ZERO(&fds); 
        
        // Add server file descriptor to fd_set
        FD_SET(srv.sock, &fds);

        // Add client file descriptor to fd_set
        for(int i=0;i<MAX_CONN;i++)
        {
            
            if(clts[i]!=NULL)FD_SET(clts[i]->sock, &fds);
            if(clts[i]!=NULL && clts[i]->sock > max_fd) max_fd = clts[i]->sock;
        }
        
        if(select( max_fd + 1, &fds, NULL, NULL, NULL)<0)
        {
            perror("select error");
            exit(1);
        }

        // Accept an incoming connection if the server socket is set
        if(FD_ISSET(srv.sock,&fds))
        {
            // If number of connections exceed the maximum allowed then decline
            if(n_clts > MAX_CONN)
            {
                perror("simplex-talk: accept limit of 100 exceeded");
                // exit(1);
            }
            // Add the new client connection to array
            if(new_connection(&srv)==-1)
            {
                perror("simplex-talk: new connection");
                exit(1);
            }
        }

        // for each of the active clients in array check whether they are set
        for(int i=0;i<MAX_CONN;i++)
        {
            if(clts[i]==NULL) continue;

            else if(FD_ISSET(clts[i]->sock,&fds))
            {
                // If X is INT_MIN then handle the first shake
                if(clts[i]->X==INT_MIN) handle_first_shake(clts[i]);
                // Else handle second connection and close it.
                else
                {
                    handle_second_shake(clts[i]);
                    close_connection(i);
                }
                continue;
            }
        }
    }

  return 0;
}


