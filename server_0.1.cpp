#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sstream>


void error(char *msg) {
	perror(msg);
	exit(1);
}

void manageClient(int sockfd, std::vector<int> outsocks) {

	int newsockfd;
	socklen_t clilen;
	struct sockaddr_in cli_addr;
	char buffer[255];

	clilen = sizeof(cli_addr);

	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	outsocks.push_back(newsockfd);
	int id = outsocks.size();

	printf("New client joined");

	while (read(newsockfd, buffer, 255) > 0) {
		printf("Here is the message: %s\n", buffer);
		for (int i = 0; i < outsocks.size(); i++) {
			write(outsocks[i],("%s says: %s\n", id, buffer), 18);
		}
	}

	return;
}

int main(int argc, char *argv[]) {
	int sockfd, portno;
	struct sockaddr_in serv_addr;

	int maxThreads = 2;
	int connectedThreads = 0;
	std::vector<std::thread> threads;
	std::vector<int> outsocks;

	if (argc < 2) {
		fprintf(stderr, "ERROR: no port provided\n");
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		error("ERROR opening socket");
	}

	memset((char *) &serv_addr, 0, sizeof(serv_addr));

	portno = atoi(argv[1]);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		error("ERROR on accept");
	}

	listen(sockfd,maxThreads);

	for(int i = 0; i < maxThreads; i++) {
		threads.push_back(std::thread(manageClient,sockfd, outsocks));
	}

	for(auto &t : threads) {
		t.join();
	}
}