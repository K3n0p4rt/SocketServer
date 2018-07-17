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
#include <mutex>
#include <signal.h>

std::mutex outsocks_mtx;
std::mutex connectedSocks_mtx;
std::vector<int> outsocks;
int connectedSocks;
int maxSocks;

void exit_handler(int s) {
	for(int i = 0; i < outsocks.size(); i++) {
		std::cout << "did quit" << outsocks[i] << "\n";
		close(outsocks[i]);
	}
	exit(1);
}

void error(char *msg) {
	perror(msg);
}

void readAndWrite(int sockfd, int newsockfd) {

	char buffer[256];

	outsocks_mtx.lock();
	outsocks.push_back(newsockfd);
	outsocks_mtx.unlock();

	std::cout << "New client joined\n";

	while (read(newsockfd, buffer, 255) > 0) {

		std::cout << "Here is the message: " << buffer;
		bool didWrite = false;

		outsocks_mtx.lock();
		for (int i = 0; i < outsocks.size(); i++) {
			if (outsocks[i] != newsockfd) {
				fprintf(stderr, "write");
				if (write(outsocks[i],("%s\n", buffer), 18) > 0) {
					didWrite = true;
				}
			}
		}
		outsocks_mtx.unlock();

		if (didWrite) {
			std::fill_n(buffer, 256, '\0');
		}
	}


	//NEED TESTING clear sock from outsock when done
	outsocks_mtx.lock(); 
	for (int i = 0; i < outsocks.size(); i++) {
		if (outsocks[i] == newsockfd) {
			outsocks.erase(outsocks.begin()+i);
		}
	}
	outsocks_mtx.unlock();

	return;
}

void manageClient(int sockfd, int newsockfd) {
	//TODO impliment Client Management where: 
	//		if connectedSocks > maxSocks close connection
	//		if connectedSocks <= maxSocks readAndWrite

	connectedSocks_mtx.lock();
	connectedSocks ++;
	bool canConnect = connectedSocks <= maxSocks;
	connectedSocks_mtx.unlock();

	if (canConnect) {
		readAndWrite(sockfd,newsockfd);
	} else {
		write(newsockfd,("Server full. Please try agian later\n"), 35);
	}

	connectedSocks_mtx.lock();
	std::cout << "Did close\n";
	write(newsockfd,("Server Disconnected\n"), 19);
	close(newsockfd);
	connectedSocks --;
	connectedSocks_mtx.unlock();

	return;
}

int main(int argc, char *argv[]) {
	int sockfd, portno;
	struct sockaddr_in serv_addr;

	//Deals with interupt signal
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = exit_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);

	signal(SIGINT,exit_handler);	

	maxSocks = 3;
	connectedSocks = 0;
	std::vector<std::thread> threads;

	std::cout << "Starting server...\n";

	if (argc < 2) {
		error("ERROR: no port provided\n");
		exit(1); 
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		error("ERROR opening socket");
	}

	std::fill_n((char *) &serv_addr, sizeof(serv_addr), '\0');

	portno = atoi(argv[1]);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		error("ERROR on accept");
	}

	listen(sockfd,maxSocks + 1);

	while(1) {

		int newsockfd;
		socklen_t clilen;
		struct sockaddr_in cli_addr;

		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

		threads.push_back(std::thread(manageClient, sockfd, newsockfd));
		std::cout << threads.back().get_id() << " thread started\n";
	}

	for(auto &t : threads) {
		std::cout << t.get_id() << " thread joined\n";
		t.join();
	}

	return 0;
}