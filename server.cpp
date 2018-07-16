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

std::mutex outsocks_mtx;
int connectedSocks;
int maxSocks;

struct client {
	int sockfd;
	std::string name;
	std::vector<client> friends;
	int refs;

	client(int sockfd, std::string name) : refs(1)
	{
		this->sockfd = sockfd;
		this->name = name;
	}
	
	//Deconstructor. 
	//!! MAKE SURE TO ONLY CALL EXPLICITLY WHEN OUTSOCKS_MTX IS LOCKED !!
	// ~client() {
	// 	if (this.ref == 0) {
	// 		for(int i = 0; i < sizeof(this->friends); i++) {
				
	// 			delete this->friends[i];

	// 		}

	// 	} else {
	// 		this.refs--;
	// 	}
	// }

	// inc_ref() {
	// 	this->refs++;
	// }
};

std::vector<client*> outsocks;
std::vector<client*> onlinesocks;

void error(char *msg) {
	perror(msg);
	exit(1);
}

void readAndWrite(int sockfd, client* &thisClient) {

	char buffer[256];

	outsocks_mtx.lock();
	int newsockfd = thisClient->sockfd;
	outsocks_mtx.unlock();

	cout << "New client joined\n";

	// while (read(newsockfd, buffer, 18) > 0) {

	// 	printf("%s %s\n", thisClient->name.c_str(), buffer);
	// 	bool didWrite = false;

	// 	outsocks_mtx.lock();
	// 	for (int i = 0; i < outsocks.size(); i++) {
	// 		if (onlinesocks[i]->sockfd != newsockfd) {
	// 			fprintf(stderr, "write\n");
	// 			if (write(onlinesocks[i]->sockfd, 
	// 				("%s says: %s\n" ,thisClient->name.c_str(),buffer), 
	// 				strlen(buffer)) > 0) {
	// 				didWrite = true;
	// 			}
	// 		}
	// 	}
	// 	outsocks_mtx.unlock();

	// 	if (didWrite) {
	// 		std::fill_n(buffer, 256, '\0');
	// 	}
	// }

	int n;

	while (1) {

		std::fill_n(buffer, 256, '\0');

		n = read(newsockfd,buffer,255);

		if (n < 0) {
			error("ERROR reading from socket");
		}
		
		cout << "Here is the message: " + buffer;

		outsocks_mtx.lock();
		for (int i = 0; i < outsocks.size(); i++) {
			if (onlinesocks[i]->sockfd != newsockfd) {
				fprintf(stderr, "write\n");
				n = write(onlinesocks[i]->sockfd, 
					("%s says: %s\n" ,thisClient->name.c_str(),buffer), 
					strlen(buffer) > 0);
			}
		}
		outsocks_mtx.unlock();

		if (n < 0) {
			error("ERROR writing to socket");
		}
	}

	return;
}



void manageClient(int sockfd, int newsockfd) {

	cout << "Started Managing\n";

	bool clientExist = false;
	client * thisClient;

	cout << "Instatiated Vars\n";


	//Check weather client has logged in b4
	outsocks_mtx.lock();
	for(int i = 0; i < outsocks.size(); i++) {
		if(outsocks[i]->sockfd == newsockfd) {
			cout << "Client already exist";
			clientExist = true;
			thisClient = outsocks[i];
		}
	}
	outsocks_mtx.unlock();

	cout << "Checked for existence\n";

	//Depending on weather client exist:
	//		Welcome client back
	//		Create new client and enter it to outsocks
	if(clientExist) {
		write(newsockfd,
			("Welcome back %s\n", thisClient->name.c_str()), 
			13 + thisClient->name.size());
	} else {
		if (write(newsockfd,("Please enter your name:"),23) > 0) {

			char buffer[256];

			if (read(newsockfd, buffer, 18) < 0) {
				error("ERROR reading from socket");
			}
			
			std::string name(buffer);

			outsocks_mtx.lock();
				thisClient = new client(newsockfd,name);
				outsocks.push_back(thisClient);
			outsocks_mtx.unlock();
		}
	}

	cout << "Dealt with existence\n";

	//Try to manage clients where: 
	//		if connectedSocks > maxSocks close connection to client
	//		if connectedSocks <= maxSocks readAndWrite client
	outsocks_mtx.lock();
		connectedSocks ++;
		bool canConnect = connectedSocks <= maxSocks;
	
		onlinesocks.push_back(thisClient);
		// thisClient->inc_ref();	
	outsocks_mtx.unlock();

	cout << "Assgined client\n";

	if (canConnect) {
		readAndWrite(sockfd,thisClient);
	} else {
		write(newsockfd,("Server full. Please wait..."), 27);
		//TODO handle que of clients waiting to get in;
	}

	cout << "Handled client\n";

	outsocks_mtx.lock();
		connectedSocks --;
		for(int i = 0; i < onlinesocks.size(); i++) {
			if (onlinesocks[i]->sockfd == thisClient->sockfd) {
				onlinesocks.erase(outsocks.begin()+i);

				close(thisClient->sockfd);
			}
		}
	outsocks_mtx.unlock();

	cout << "Client left\n";	

	return;
}

int main(int argc, char *argv[]) {
	int sockfd, portno;
	struct sockaddr_in serv_addr;

	maxSocks = 3;
	connectedSocks = 0;
	std::vector<std::thread> threads;

	cout << "Starting server...\n";

	if (argc < 2) {
		fprintf(stderr, "ERROR: no port provided\n");
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
	}

	for(auto &t : threads) {
		t.join();
	}

	return 0;
}