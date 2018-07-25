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
std::vector<Client> outsocks;
std::vector<Client> onlinesocks;
int connectedSocks;
int maxSocks;
int sockfd;

void exit_handler(int s) {
	for(int i = 0; i < onlinesocks.size(); i++) {
		std::cout << "Did quit " << onlinesocks[i].name << "\n";
		close(onlinesocks[i].sockfd);
		//TODO: clear onlinesocks
	}
	close(sockfd);
	exit(1);
}

void error(std::string msg) {
	perror(msg.c_str());
}



bool commandListener(Client client, char * buffer) {

	std::string command(buffer);
	if (command.compare(0, 1, "/") == 0) {
		if(command.compare(1, 7, "friends") == 0) {
			std::string returnString;
			returnString = "Friends:\n";
			for(int i; i < client.friends.size(); i++) {
				returnString += client.friends[i].name;
			}
			write(client.sockfd, returnString.c_str(), 255);
		} else if (command.compare(1, 6, "online") == 0) {
			std::string returnString;
			returnString = "Online:\n";
			for(int i = 0; i < onlinesocks.size(); i++) {
				std::cout << onlinesocks[i].name;
				returnString += onlinesocks[i].name;
			}
			write(client.sockfd, returnString.c_str(), 255);
		} else if (command.compare(1, 9, "all_users") == 0) {
			std::string returnString;
			returnString = "Users:\n";
			for(int i = 0; i < outsocks.size(); i++) {
				std::cout << outsocks[i].name;
				returnString += outsocks[i].name;
			}
			write(client.sockfd, returnString.c_str(), 255);
		} else if (command.compare(1, 11, "add_friend ") == 0) {
			std::string name;
			for(int i = 0; i < (strlen(buffer) - 12); i++) {
				name.push_back(buffer[12 + i]);
			}

			int i = 0;

			for(int j = 0; j < client.friends.size(); j++) {
				if (name.compare(client.friends[j].name) == 0) {
					write(client.sockfd,("Already friends.\n"), 16);
					break;
				}
			}

			for(i; i < outsocks.size(); i++) {
				std::cout << outsocks[i].name;
				std::cout << name;
				if (name.compare(outsocks[i].name) == 0) {
					//TODO impliment friend requests
					client.friends.push_back(outsocks[i]);
					outsocks[i].friends.push_back(client);
					write(client.sockfd,("Friend added.\n"), 20);	
					goto EXIST;
				}
			}
			
			write(client.sockfd,("No such user exist.\n"), 20);

			EXIST:;
		} else if (command.compare(1, 4, "chat") == 0) {
			
			std::string name;
			for(int i = 0; i < (strlen(buffer) - 4); i++) {
				name.push_back(buffer[4 + i]);
			}

			bool exist = false;
			int i = 0;

			for(i; i < client.friends.size(); i++) {
				if (name.compare(client.friends[i].name) == 0) {
					exist = true;
					break;
				}
			}

			if (exist) {			
				client.currChat = client.friends[i].sockfd;
			} else {
				write(client.sockfd,("No such user exist.\n"), 20);
			}
		} else if (command.compare(1, 4, "exit") == 0) {
			for(int i = 0; i < onlinesocks.size(); i++) {
				return false;
			}
		} else {
			write(client.sockfd, ("Command does not exists\n"), 24);
		}
	} else {
		std::cout << "Here is the message: " << buffer;
		for (int i = 0; i < outsocks.size(); i++) {
			if (onlinesocks[i].sockfd != client.sockfd) {
				fprintf(stderr, "write\n");
				write(onlinesocks[i].sockfd, buffer, strlen(buffer));
			}
		}
	}

	return true;
}



void readAndWrite(Client client) {

	std::cout << "New client joined\n";
	bool connected = true;

	while (connected) {

		char buffer[256];

		connected = (read(client.sockfd, buffer, 255) > 0);
		
		outsocks_mtx.lock();
		connected = connected && commandListener(client, buffer);
		outsocks_mtx.unlock();
		
		buffer[0] = 0;
	}

	return;
}

void createUser(int newsockfd, std::string name) {
	try {
		write(newsockfd,("Unable to find user.\n"), 21);
		write(newsockfd,("Creating new user.\n"), 19);

		Client temp = Client();

		temp.name = name;
		temp.sockfd = newsockfd;
		char buffer[256];
		std::fill_n(buffer, 256, '\0');


		LOOP:
		write(newsockfd,("Please enter new password:\n"), 27);
		read(newsockfd, buffer, 255);
		std::string pass(buffer);

		std::fill_n(buffer, 256, '\0');

		write(newsockfd,("Please confirm your password:\n"), 30);
		read(newsockfd, buffer, 255);
		std::string pass_confirm(buffer);
		
		std::fill_n(buffer, 256, '\0');

		if (pass.compare(pass_confirm) == 0) {
			temp.pass = pass;
			write(newsockfd,("Password confirmed!\n"), 20);
		} else {
			write(newsockfd,("Passwords do not match, please try again.\n"), 42);
			goto LOOP;
		}
		outsocks_mtx.lock();
		outsocks.push_back(temp);
		outsocks_mtx.unlock();

	} catch (int e){
		error("ERROR creating user");
		return;
	}

	return;
}


//Checks weather current client exist, 
//	then if not calls createUser, 
//	else, adds client to onlinesocket vector.
//	then calls readAndWrite
//	"logoff" on return
void logIn(int newsockfd) {

	//Depending on weather client exist:
	//		Welcome client back
	//		Create new client and enter it to outsocks
	std::cout << "Prompt for name.\n";
	Client thisClient;

	try {
		write(newsockfd,("Please enter username:\n"), 23);
		char buffer[256];
		std::fill_n(buffer, 256, '\0');

		read(newsockfd, buffer, 255);
		std::cout << "Name read.\n";

		std::string name(buffer);
		std::fill_n(buffer, 256, '\0');
		bool exist = false;

		for(int i = 0; i < outsocks.size(); i++) {
			outsocks_mtx.lock();
			if (outsocks[i].name.compare(name) == 0) {
				write(newsockfd,("Welcome back!\n"), 14);
				exist = true;
				outsocks[i].sockfd = newsockfd;
				thisClient = outsocks[i];
			}
			outsocks_mtx.unlock();
		}

		if (!exist) {
			createUser(newsockfd, name);
			thisClient = outsocks.back();
		}

		write(newsockfd, ("Please enter password:\n"), 23);
		PASS_LOOP:
		read(newsockfd, buffer, 255);
		std::string pass(buffer);
		std::fill_n(buffer, 256, '\0');
		if(pass.compare(thisClient.pass) != 0) {
			write(newsockfd, ("Password incorrect. Please retry.\n"), 34);
			goto PASS_LOOP;
		}

		write(newsockfd,("Connected to chatroom!\n"), 23);
		onlinesocks.push_back(thisClient);
		readAndWrite(onlinesocks.back());
		for(int i = 0; i < onlinesocks.size(); i++) {
			if(onlinesocks[i].name.compare(name) == 0) {
				std::cout << name << " going offline\n";
				onlinesocks.erase(onlinesocks.begin()+i);
			}
		}
	} catch (std::exception) {
		error("ERROR writing to socket");
		return;
	}

	return;
}

//Manages server capacity. Counts connected clients, as well as manages weather a client can join or not
void manageClient(int newsockfd) {
	//TODO impliment Client Management where: 
	//		if connectedSocks > maxSocks close connection
	//		if connectedSocks <= maxSocks readAndWrite
	outsocks_mtx.lock();
	connectedSocks ++;
	bool canConnect = connectedSocks <= maxSocks;
	outsocks_mtx.unlock();

	std::cout << "Checking server capacity.\n";

	if (canConnect) {
		std::cout << "Client loging in.\n";
		logIn(newsockfd);
	} else {
		write(newsockfd,("Server full. Please try agian later\n"), 37);
	}

	std::cout << "Client leaving.\n";

	outsocks_mtx.lock();
	std::cout << "Did close\n";
	write(newsockfd,("Server Disconnected\n"), 20);
	close(newsockfd);
	connectedSocks --;
	outsocks_mtx.unlock();

	return;
}

//TODO move server creation to helper fuction
// int initServer() {

// 	sockfd = socket(AF_INET, SOCK_STREAM, 0);	//create local socket
// 	if (sockfd < 0) {
// 		error("ERROR opening socket");
// 		return -1;
// 	}

// 	if (bind(sockfd, (struct sockaddr *)))
// }

int main(int argc, char *argv[]) {
	int portno;
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
		error("ERROR: no port provided");
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);	//create local socket
	if (sockfd < 0) {
		error("ERROR opening socket");
		return -1;
	}

	//Bind socket to server address
	std::fill_n((char *) &serv_addr, sizeof(serv_addr), '\0');
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		error("ERROR on accept");
		close(sockfd);
		return -1;
	}

	if (listen(sockfd,maxSocks + 1) < 0) {
		error("ERROR on listen");
		close(sockfd);
		return -1;
	}


	//Loop for accepting clients
	while(1) {

		int newsockfd;
		socklen_t clilen;
		struct sockaddr_in cli_addr;

		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

		threads.push_back(std::thread(manageClient, newsockfd));
		std::cout << threads.back().get_id() << " thread started\n";
	}

	//Join treads to main thread
	for(auto &t : threads) {
		std::cout << t.get_id() << " thread joined\n";
		t.join();
	}

	return 0;
}