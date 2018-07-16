#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <mutex>

void error(char *msg) {
	perror(msg);
	exit(0);
}


void error(char *msg, int sockfd) {
	perror(msg);
	exit(0);
}

void writeLoop(int sockfd) {
	
	char buffer[256];

	while (1) {
		fgets(buffer, 255, stdin);
		n = write(sockfd, buffer, strlen(buffer));
		std::fill_n(buffer, 256, '\0');
		if (n < 0) {
			error("ERROR writing to socket",sockfd);
		}
	}
}

void readLoop(int sockfd) {

	char buffer[256];

	while (1) {
		n = read(sockfd, buffer, 255);
		if (strlen(buffer) > 0) {
			printf("%s\n",buffer);
			std::fill_n(buffer, 256, '\0');
		}
		if (n < 0) {
			error("ERROR reading from socket",sockfd);
		}
	}

	
}


int main(int argc, char *argv[]) {
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	if (argc < 3) {
		//printf(stderr, "usage %s hostname port\n", argv[0]);
		exit(0);
	}

	portno = atoi(argv[2]);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error("ERROR opening socket");
	}

	server = gethostbyname(argv[1]);
	if (server == NULL) {
		fprintf(stderr, "ERROR: no such host\n");
		exit(0);
	}

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;

	memcpy((char *) &serv_addr.sin_addr.s_addr, (char *) server -> h_addr, 
		server -> h_length);

	serv_addr.sin_port = htons(portno);

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		error("ERROR connecting");
	}

	std::thread readThread(readLoop,sockfd);
	std::thread writeTread(writeLoop,sockfd);

	readThread.join();
	writeTread.join();
	return 0;
}