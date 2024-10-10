#include <iostream>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <string>
#include <algorithm>

#pragma comment(lib, "WS2_32.lib")

void getInput(SOCKET cs)
{
    while (true) {
        //char message[1024] = { 0 };
        //std::cin >> message;

        std::string message;
        std::getline(std::cin, message);
        send(cs, message.c_str(), message.length(), 0);
    }
}

int main()
{
    WSADATA wsaData;
    int wsaerr;
    WORD wVersionRequested = MAKEWORD(2, 2);
    wsaerr = WSAStartup(wVersionRequested, &wsaData);

    if (wsaerr != 0) {
        std::cout << "The winsock dll not found" << std::endl;
        return 0;
    }

    SOCKET clientSocket;
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    std::string ip;
    std::cout << "Please input server ip. Input d for default: ";
    std::cin >> ip;
    if (ip == "d") { ip = "192.168.100.23"; }

    sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr(ip.c_str());
    clientService.sin_port = htons(4277);

    if (connect(clientSocket, reinterpret_cast<SOCKADDR*>(&clientService), sizeof(clientService)) == SOCKET_ERROR) {
        std::cout << "Client: connect() - Failed to connect: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 0;
    }
    else {
        std::cout << "Waiting in line...\n";
        //std::cout << "Connection OK!\n";
    }

    std::thread t1(getInput, clientSocket);
    t1.detach();

    char receiveBuffer[1024] = { 0 };
    int n = sizeof(receiveBuffer) / sizeof(receiveBuffer[0]);

    while (true) {
        int rbyteCount = recv(clientSocket, receiveBuffer, 1024, 0);
        if (rbyteCount > 0) {
            std::cout << receiveBuffer;
        }
        std::fill(receiveBuffer, receiveBuffer + n, 0);
    }

    return 0;
}

