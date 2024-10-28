#include <ncurses.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <algorithm>
#include <fstream>

std::atomic<int> chatAmnt;
std::atomic<bool> running = true;

struct userData {
	std::string name;
	std::string ip;
	int port;
};

class Chat {
	std::vector<std::string> chats;
	int chatMax, r, c;
	WINDOW *window;

public:
	Chat(WINDOW *chatWindow) {
		window = chatWindow;
		getmaxyx(window, r, c);
		chatMax = r - 4;
	}

	void moveChats(int direction) {
		wclear(window);
		for (int i = 0; i < chatMax; i++) {
			mvwprintw(window, i, 0, chats[chats.size() - chatMax + i].c_str());
		}
		wrefresh(window);
	}

	void chatAdd(std::string message) {
		chats.push_back(message);
		if (chatAmnt > chatMax) {
			moveChats(1);
			wrefresh(window);
			chatAmnt++;
		} else {
			mvwprintw(window, chatAmnt, 0, message.c_str());
			wrefresh(window);
			chatAmnt++;
		}
		std::string amnts = std::to_string(chatAmnt); 
		mvprintw(1, 1, amnts.c_str());
	}
};

std::string wgetinput(WINDOW *window) {
	std::string input;

	curs_set(1);
	
	int ch = wgetch(window);

	while (ch != '\n') {
		if (ch == KEY_BACKSPACE || ch == KEY_DC || ch == 127) {
			input.pop_back();
			mvwprintw(window, 0, input.length(), " ");
		} else {
			input.push_back(ch);
		}

		mvwprintw(window, 0, 0, input.c_str());
		wrefresh(window);

		ch = wgetch(window);
	}

	curs_set(0);
	wclear(window);
	return input;
}

userData getUserData() {
	std::string line;
	std::ifstream config("config.txt");

	userData userdata;

	while (getline(config, line)) {
		if(line.find("Name = ") != std::string::npos) {
			userdata.name = line.substr(line.find("Name = ") + 7);
		} else if(line.find("ip = ") != std::string::npos) {
			userdata.ip = line.substr(line.find("ip = ") + 5);
		} else if(line.find("port = ") != std::string::npos) {
			userdata.port = std::stoi(line.substr(line.find("port = ") + 7));
		}
	}
	return userdata;
}


void handleInput(WINDOW *window, Chat chat, int cs) {
	std::string input;
	input = wgetinput(window);

	while (running) {
		if (input == ":q") {
			running = false;
		} else {
			chat.chatAdd(input);
			send(cs, input.c_str(), input.length(), 0);
			input = wgetinput(window);
		}
	}
}

void handleRecvMessages(Chat chat, int cs) {
	char receiveBuffer[1024] = { 0 };
	int n = sizeof(receiveBuffer) / sizeof(receiveBuffer[0]);

	while (running) {
		int rByteCount = recv(cs, receiveBuffer, 1024, 0);
		if (rByteCount > 0) {
			chat.chatAdd(std::string(receiveBuffer));
		}
		std::fill(receiveBuffer, receiveBuffer + n, 0);
	}
}

void writeDataWindow(WINDOW *window, userData userdata) {
	int w, h;
	getmaxyx(window, h, w);

	wattron(window, COLOR_PAIR(1));

	for (int i = 0; i < w; i++) {
		mvwprintw(window, 0, i, " ");
	}

	mvwprintw(window, 0, 0, "AfdChat v.0.0.0 ### Enter ':' followed by a command to use commands, enter ':q' to exit the app");
	
	for (int i = 0; i < w; i++) {
		mvwprintw(window, 6, i, "^");
	}
	wattroff(window, COLOR_PAIR(1));

	std::string ipdisplay = "Connected IP: " + userdata.ip;
	std::string portdisplay = "Connected port: " + std::to_string(userdata.port);
	std::string namedisplay = "Current username: " + userdata.name;

	mvwprintw(window, 1, 0, ipdisplay.c_str());
	mvwprintw(window, 2, 0, portdisplay.c_str());
	mvwprintw(window, 3, 0, namedisplay.c_str());

	wrefresh(window);
}

int main() {
	initscr();
	cbreak();
	noecho();
	curs_set(0);

	clear();

	refresh();

	start_color();
	init_pair(1, COLOR_YELLOW, COLOR_BLUE);

	int maxX, maxY;
	getmaxyx(stdscr, maxY, maxX);

	WINDOW *inputWindow = newwin(1, maxX, maxY - 1, 0);
	WINDOW *chatWindow = newwin(maxY - 8, maxX, 8, 0);
	WINDOW *dataWindow = newwin(7, maxX, 0, 0);
	refresh();
	wrefresh(inputWindow);
	wrefresh(chatWindow);
	wrefresh(dataWindow);
	keypad(inputWindow, true);

	int clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	refresh();
	wrefresh(inputWindow);
	wrefresh(chatWindow);
	wrefresh(dataWindow);

	userData userdata = getUserData();
	writeDataWindow(dataWindow, userdata);

	std::string ip;

	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(userdata.ip.c_str());
	serverAddress.sin_port = htons(userdata.port);

	if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		mvprintw(1, 1, "Connection failed");
	}

	Chat chat(chatWindow);
	

	send(clientSocket, userdata.name.c_str(), userdata.name.length(), 0);

	std::thread t1(handleInput, inputWindow, chat, clientSocket);
	t1.detach();

	std::thread t2(handleRecvMessages, chat, clientSocket);
	t2.detach();


	while (running) {
		//just to keep the app open	
	}


	endwin();

	exit(0);
}
