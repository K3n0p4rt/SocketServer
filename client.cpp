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
#include <signal.h>

void error(std::string msg) {
	perror(msg.c_str());
	exit(0);
}


void error(std::string msg, int sockfd) {
	perror(msg.c_str());
	exit(0);
}

void writeLoop(int sockfd) {
	
	int n;

	while (1) {
		char buffer[256];
		std::fill_n(buffer, 256, '\0');
		fgets(buffer, 255, stdin);
		n = write(sockfd, buffer, strlen(buffer));
		if (n < 0) {
			close(sockfd);
			return;
		}
	}
}

void readLoop(int sockfd) {

	int n;

	while (1) {
		char buffer[256];
		std::fill_n(buffer, 256, '\0');
		n = read(sockfd, buffer, 255);
		if (n < 0) {
			close(sockfd);
			return;
		}
		if (strlen(buffer) > 0) {
			std::cout << buffer;
		}
	}

	
}


int main(int argc, char *argv[]) {
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	//TODO Deals with interupt signal


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
		error("ERROR: no such host");
		exit(0);
	}

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;

	memcpy((char *) &serv_addr.sin_addr.s_addr, (char *) server -> h_addr, 
		server -> h_length);

	serv_addr.sin_port = htons(portno);

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		error("ERROR connecting");
	}

	std::thread readThread(readLoop,sockfd);
	std::thread writeTread(writeLoop,sockfd);

	readThread.join();
	writeTread.join();
	close(sockfd);
	std::cout << "Exit gracfully";

	return 0;
}