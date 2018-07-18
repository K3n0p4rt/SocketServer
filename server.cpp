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

struct Client {
	std::string name;
	std::string pass;
	int sockfd;
	std::vector<Client> friends;
	int currChat;
};

std::mutex outsocks_mtx;
std::mutex connectedSocks_mtx;
std::vector<Client> outsocks;
std::vector<Client> onlinesocks;
int connectedSocks;
int maxSocks;

void exit_handler(int s) {
	for(int i = 0; i < outsocks.size(); i++) {
		std::cout << "did quit" << outsocks[i].name << "\n";
		close(outsocks[i].sockfd);
	}
	exit(1);
}

void error(char *msg) {
	perror(msg);
}

void readAndWrite(int newsockfd, std::string name) {

	char buffer[256];
	int n;

	std::cout << "New client joined\n";

	while (1) {
		
		std::fill_n(buffer, 256, '\0');

		n = read(newsockfd,buffer, 255);
		if(n <= 0)
			break;
		if(n > 0) {
			std::cout << "Here is the message: " << buffer;
			outsocks_mtx.lock();
			for (int i = 0; i < outsocks.size(); i++) {		
				if (outsocks[i].sockfd != newsockfd) {
					fprintf(stderr, "write");
					n = write(outsocks[i].sockfd,("%s\n", buffer), 18);
					if(n < 0) 
						break;
				}
			}
		}
	}

	return;
}

void createUser(int newsockfd, std::string name) {
	
	write(newsockfd,("Unable to find user.\n"), 23);
	write(newsockfd,("Creating new user.\n"), 23);

	Client temp = Client();
	temp.name = name;
	temp.sockfd = newsockfd;
	char buffer[256];

	LOOP:
	write(newsockfd,("Please enter password:\n"), 30);
	read(newsockfd, buffer, 255);

	std::string pass(buffer);
	std::fill_n(buffer, 256, '\0');

	write(newsockfd,("Please confirm your password:\n"), 30);

	read(newsockfd, buffer, 255);

	std::string pass_confirm(buffer);
	std::fill_n(buffer, 256, '\0');

	if (pass.compare(pass_confirm) == 0) {
		temp.pass = pass;
		write(newsockfd,("Password confirmed!"), 30);
	} else {
		write(newsockfd,("Passwords do not match, please try again."), 40);
		goto LOOP;
	}

	outsocks.push_back(temp);

	return;
}


//Checks weather current client exist, 
//	then if not calls createUser, 
//	else, adds client to onlinesocket vector.
//	then calls readAndWrite
//	"logoff" on return
void logIn(int sockfd, int newsockfd) {

	//Depending on weather client exist:
	//		Welcome client back
	//		Create new client and enter it to outsocks
	write(newsockfd,("Please enter username:\n"), 23);

	char buffer[256];

	if (read(newsockfd, buffer, 255) < 0) {
		error("ERROR reading from socket");
	}

	std::string name(buffer);
	bool exist = false;
	Client thisClient;

	for(int i = 0; i < outsocks.size(); i++) {
		if (outsocks[i].name.compare(name) == 0) {
			write(newsockfd,("Welcome back %s\n", name.c_str()), 13 + name.length());
			exist = true;
			outsocks[i].sockfd = newsockfd;
			onlinesocks.push_back(outsocks[i]);
			thisClient = outsocks[i];
		}
	}

	if (!exist) {
		outsocks_mtx.lock();
		createUser(newsockfd, name);
		thisClient = outsocks.back();
		outsocks_mtx.unlock();
	}

	//TODO impliment password check

	onlinesocks.push_back(thisClient);
	readAndWrite(newsockfd, name);

	for(int i = 0; i < onlinesocks.size(); i++) {
		if(onlinesocks[i].name.compare(name) == 0) {
			outsocks.erase(outsocks.begin()+i);
		}
	}

	return;
}

//Manages server capacity. Counts connected clients, as well as manages weather a client can join or not
void manageClient(int sockfd, int newsockfd) {
	//TODO impliment Client Management where: 
	//		if connectedSocks > maxSocks close connection
	//		if connectedSocks <= maxSocks readAndWrite



	connectedSocks_mtx.lock();
	connectedSocks ++;
	bool canConnect = connectedSocks <= maxSocks;
	connectedSocks_mtx.unlock();

	if (canConnect) {
		logIn(sockfd,newsockfd);
	} else {
		write(newsockfd,("Server full. Please try agian later\n"), 37);
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

	maxSocks = 3;
	connectedSocks = 0;
	std::vector<std::thread> threads;

	//Deals with interupt signal
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = exit_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);

	signal(SIGINT,exit_handler);	

	std::cout << "Starting server...\n";

	//Throws error if user input is insuficient
	if (argc < 2) {
		//error("ERROR: no port provided\n");
		exit(1); 
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);	//create local socket

	if (sockfd < 0) {
		//error("ERROR opening socket");
	}

	//Bind socket to server address
	std::fill_n((char *) &serv_addr, sizeof(serv_addr), '\0');
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		//error("ERROR on accept");
	}

	listen(sockfd,maxSocks + 1);


	//Loop for accepting clients
	while(1) {

		int newsockfd;
		socklen_t clilen;
		struct sockaddr_in cli_addr;

		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

		threads.push_back(std::thread(manageClient, sockfd, newsockfd));
		std::cout << threads.back().get_id() << " thread started\n";
	}

	//Join treads to main thread
	for(auto &t : threads) {
		std::cout << t.get_id() << " thread joined\n";
		t.join();
	}

	return 0;
}