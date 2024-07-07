/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 
#define MAXDATASIZE 1024 // max number of bytes we can get at once 
#define SERVERADDR "172.16.13.134"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void parse_url(char* url, char **host, char **port, char **path) {
    char *token;
    
    // Allocate memory for host and path
    *host = malloc(100);
    *port = malloc(100);
    *path = malloc(100);
    
    int tokenCount = 0; // To keep track of the number of tokens
    
    // Use strtok to split the string by "/"
    token = strtok(url, "/");
    
    // Keep looping to extract all tokens and store them in the array
    while (token != NULL && tokenCount < 100) {
        if (tokenCount < 2) {
            strcpy(*host, token);
        } else {
            strcat(*path, "/");
            strcat(*path, token);
        }
        
        tokenCount++;
        token = strtok(NULL, "/");
    }

    if (strstr(*host, ":") == NULL) {
        strcpy(*port, "80");    
    } else {
        char *temp_host = strtok(*host, ":");
        strcpy(*port, strtok(NULL, ":"));
        strcpy(*host, temp_host);
    }
}
void rcv_and_write_to_output(int client_socket){
	char buffer[1024];
	int bytes_rcv;
	FILE* output_file = fopen("output", "wb");
	
	if(output_file == NULL){
		perror("fopen");
		exit(1);
	}
	
	while((bytes_rcv = recv(client_socket, buffer, sizeof(buffer), 0)) > 0){
		fwrite(buffer, 1, bytes_rcv, output_file);
	}
	
	if(bytes_rcv < 0){
		printf("400 Bad Request \n");
		exit(1);
	}
	fclose(output_file);
}

int main(int argc, char *argv[])
{
	int client_socket;  
	struct addrinfo hints, *servinfo, *p;
	char s[INET6_ADDRSTRLEN];
	char* host = NULL;
	char* port = NULL;
	char* path = NULL;
	char req[100] = "GET ";

	if (argc != 2) {
	    fprintf(stderr,"usage: ./client <http>\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	// parse input http address and save in request string
	parse_url(argv[1], &host, &port, &path);
	strcat(req, path);
	strcat(req, " HTTP/1.1\r\n\r\n");
	
	if ((client_socket = getaddrinfo(host ,port, &hints, &servinfo)) != 0) {
		printf("400 Bad Request");
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(client_socket));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((client_socket = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(client_socket, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(client_socket);
			continue;
		}

		break;
	}
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);
	freeaddrinfo(servinfo); // all done with this structure
	
	// send request to the server
	if(send(client_socket, req, sizeof(req), 0) == -1){
		perror("send");
		}
	
	printf("%s\n", req);
	
	// receive server response
	while(1){
		char server_res[MAXDATASIZE];
		int bytesRead;
		if((bytesRead = recv(client_socket, server_res, MAXDATASIZE-1, 0) > 0)){
			printf("%s\n", server_res);
			rcv_and_write_to_output(client_socket);
			//fprintf(output, "%s", server_res);
			//server_res[MAXDATASIZE] = '\0';
			//fwrite(server_res, sizeof(server_res)-1, bytesRead, output);
			//fclose(output);
		}
		else{
			break;
		}
		
	}
	// fclose(output);
	close(client_socket);
	exit(0);
	

	return 0;
}

