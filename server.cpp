#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <sstream>
#include <winsock2.h>
#include <map>
#pragma comment(lib, "ws2_32.lib")

std::mutex consoleMutex;
//std::vector<SOCKET> clients;
std::vector<SOCKET> room1;
std::vector<SOCKET> room2;
std::vector<SOCKET> room3;
std::vector<SOCKET> room4;
std::vector<SOCKET> room5;
std::map<std::string, std::vector<SOCKET>> roomset;

void sendText(const std::string& text, const int clientSocket)
{
    int textLength = text.size();
    send(clientSocket, (char*)&textLength, sizeof(int), 0);
    send(clientSocket, text.c_str(), textLength, 0);
}

std::string receiveText(const int clientSocket)
{
    int textLenght;
    int bytesReceived = recv(clientSocket, (char*)&textLenght, sizeof(int), 0);
    if (bytesReceived > 0) {
        std::vector<char> textBuffer(textLenght);
        recv(clientSocket, textBuffer.data(), textLenght, 0);
        return std::string(textBuffer.begin(), textBuffer.end());
    }
    else if (bytesReceived <= 0) {
        closesocket(clientSocket);
        return "error";
    }
}

void broadcastMessage(const std::string& message, SOCKET senderSocket, std::string roomNum) {
    std::lock_guard<std::mutex> lock(consoleMutex);
    std::cout << "Client " << senderSocket << ": " << message << std::endl;

    for (SOCKET client : roomset[roomNum]) {
        if (client != senderSocket) {
            sendText(message, client);
        }
    }
}

void handleClient(SOCKET clientSocket) {
    std::string roomNum = receiveText(clientSocket);
    roomset[roomNum].push_back(clientSocket);
    while (true) {
        std::string message = receiveText(clientSocket);
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(message);

        while (std::getline(tokenStream, token, ' ')) {
            tokens.push_back(token);
        }
        if (tokens[0] == "REJOIN") {
            for (int i = 0; i < roomset[roomNum].size(); i++)
            {
                if (roomset[roomNum][i] == clientSocket) {
                    roomset[roomNum].erase(roomset[roomNum].begin()+i);
                    break;
                }
            }
            roomNum = tokens[1];
            roomset[roomNum].push_back(clientSocket);
        }
        if (message == "error") {
            std::lock_guard<std::mutex> lock(consoleMutex);
            std::cout << "Client " << clientSocket << " disconnected.\n";
            break;
        }
        broadcastMessage(message, clientSocket,roomNum);
    }

    closesocket(clientSocket);
}

int main() {
    roomset["1"] = room1;
    roomset["2"] = room2;
    roomset["3"] = room3;
    roomset["4"] = room4;
    roomset["5"] = room5;


    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is listening on port 8080...\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed.\n";
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        std::lock_guard<std::mutex> lock(consoleMutex);
        std::cout << "Client " << clientSocket << " connected.\n";

        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach(); // Detach the thread to allow handling multiple clients concurrently
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}