/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

// #define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold
#define MAXDATASIZE 512

void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


//respond function
void msg_err(int client_socket){
	char response[50] = "400 Bad Request\r\n\r\n";
	if(send(client_socket, response, sizeof(response), 0) == -1)
		perror("send");
}

void msg_notfound(int client_socket){
	char response[50] = "HTTP/1.1 404 Not Found\r\n\r\n";
	if(send(client_socket, response, sizeof(response), 0) == -1)
		perror("send");
}

void msg_OK(int client_socket){
	char response[20] = "HTTP/1.1 200 OK\r\n\r\n";
	if(send(client_socket, response, sizeof(response)-1, 0) == -1)
		perror("send");
}

int main(int argc, char* argv[])
{
	int server_socket;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
        if(argc != 2){
		fprintf(stderr, "usage: server port\n");
		exit(1);
	}

	char PORT[10];
	strcpy(PORT, argv[1]);
	
	if ((server_socket = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(server_socket));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((server_socket = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(server_socket, p->ai_addr, p->ai_addrlen) == -1) {
			close(server_socket);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(server_socket, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof(their_addr);
		int new_socket = accept(server_socket, (struct sockaddr *)&their_addr, &sin_size);
		if (new_socket == -1) {
			msg_err(new_socket);
			continue;
		}
		
		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		// GET HTTP request
		if (!fork()) {
			close(server_socket);
			char buf[MAXDATASIZE];
			size_t bytes_read;
			
			bytes_read = recv(new_socket, buf, sizeof(buf)-1, 0);
			if(bytes_read == -1){
				msg_err(new_socket);
				close(new_socket);
				exit(1);
			}
			
			buf[bytes_read] = '\0';

			if (strstr(buf, "GET ") == NULL || strstr(buf, "HTTP/1.1") == NULL) {
				msg_err(new_socket);
				close(new_socket);
				exit(1);
		    }
		    
		       	
		    char* temp;
		    char fname[100] = ".";
			char* token = strtok(buf, " ");
			if(token!=NULL){
				token = strtok(NULL, " ");
				if(token != NULL){
					temp = strdup(token);
				}
			}
			strcat(fname, temp);
			
			FILE* file;
			file = fopen(fname, "rb");
			//printf("%s\n", fname);
						
			if (file != NULL) {
				msg_OK(new_socket);
				
				char fileContent[MAXDATASIZE];
				size_t bytesRead;
				
				while((bytesRead = fread(fileContent, 1, sizeof(fileContent), file)) > 0) {
				    if(send(new_socket, fileContent, bytesRead, 0) == -1){
				    	perror("send");
				    	exit(1);
				    } 
				}
				fclose(file);
				exit(0);
				//printf("HTTP response:\n%s\n",msg_ok);
			}
			else{
				msg_notfound(new_socket);
				//printf("HTTP response:\n%s\n",msg_notfound);
			}
			printf("server: end of connection from %s\n\n", s);
			close(new_socket);
			printf("server: waiting for connections...\n");
			exit(0);
		}
		close(new_socket);
	}

	return 0;
}

