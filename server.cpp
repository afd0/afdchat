#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <cstring>
#include <algorithm>

struct clientName {
	char name[128];
};

struct clientDetails {
	int32_t clientfd;
	int32_t serverfd;
	std::vector<int> clientList;
	std::vector<clientName> clientNames;
	clientDetails(void) {
		this -> clientfd = -1;
		this -> serverfd = -1;
	}
};

const int port = 4277;
const char ip[] = "127.0.0.1";
const int backlog = 5;

int handleMessage(int client, std::string message) {
	std::cout << "Message from client: " << message << "\n";
}

int main() {
	auto client = new clientDetails();

	client -> serverfd = socket(AF_INET, SOCK_STREAM, 0);

	int opt = 1;
	setsockopt(client -> serverfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof opt);

	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	bind(client -> serverfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	std::cout << "Bind done.\n";
	listen(client -> serverfd, backlog);
	std::cout << "Listening...\n";
	fd_set readfds;
	size_t valread;
	int maxfd;
	int sd = 0;
	int activity;

	while (true) {
		std::cout << "Waiting for activity\n";
		FD_ZERO(&readfds);
		FD_SET(client -> serverfd, &readfds);
		maxfd = client -> serverfd;

		for(auto sd : client -> clientList) {
			FD_SET(sd, &readfds);
			if (sd>maxfd) {
				maxfd = sd;
			}
		}
		
		if (sd > maxfd) {
			maxfd = sd;
		}

		activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
		if (activity < 0) {
			std::cerr << "Select error\n";
			continue;
		}

		if (FD_ISSET(client -> serverfd, &readfds)) {
			std::cout << "Receiving new connection\n";
			client -> clientfd = accept(client -> serverfd, (struct sockaddr *) NULL, NULL);
			if (client -> clientfd < 0) {
				std::cerr << "Accept error\n";
				continue;
			}

			client -> clientList.push_back(client -> clientfd);

			// Get client name
			
			char welcome[128] = "Welcome to the chatroom\n\n Please enter your name: \n";
			send(client -> clientfd, welcome, strlen(welcome), 0);

			char clientname[128];
			clientName cn;
			read(client -> clientfd, clientname, 128);
			std::string cns = clientname;
			strcpy(cn.name, cns.c_str());
			client -> clientNames.push_back(cn);

			// Connection done
			std::cout << "new client connected\n" << "socket fd: " << client -> clientfd << "\n";
			std::fill(clientname, clientname + (sizeof(clientname) / sizeof(clientname[0])), 0);
		}

		char message[1024] = { 0 };
		int n = sizeof(message) / sizeof(message[0]);
		for (int i = 0; i < client -> clientList.size(); i++) {
			std::cout << "Reading message " << i << "\n";
			sd = client -> clientList[i];
			if (FD_ISSET(sd, &readfds)) {
				valread = read(sd, message, 1024);
				if (valread == 0) {
					std::cout << "Client disconnected\n";
					getpeername(sd, (struct sockaddr*)&serverAddr, (socklen_t*)&serverAddr);
					close(sd);
					client -> clientList.erase(client -> clientList.begin()+i);
				} else {
					//std::string msgStr = message;
					//std::thread t1(handleMessage, sd, message);
					//t1.detach();
					std::cout << "Message: " << message << "\n";
					
					// Modify message to be sent out
					std::string msgOut = std::string("Message from client ") + (client -> clientNames[i].name) + std::string(": ") + message;
					strncpy(message, msgOut.c_str(), sizeof(message));
					message[sizeof(message)-1] = 0;
					
					for (int j = 0; j < client -> clientList.size(); j++) {
						if (client -> clientList[j] != sd) {
							send(client -> clientList[j], message, strlen(message), 0);
						}
					}
				}
				std::fill(message, message + n, 0);
			}
		}


	}
}
