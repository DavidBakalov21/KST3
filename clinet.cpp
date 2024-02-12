#include <iostream>
#include <thread>
#include <string>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

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
        closesocket(clientSocket); //maybe will remove
        return "error";
    }
}

void receiveMessages(SOCKET clientSocket) {
    //char buffer[4096];
    while (true) {
        std::string message=receiveText(clientSocket);
      //  int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (message =="error") {
            std::cerr << "Server disconnected.\n";
            break;
        }

        //buffer[bytesReceived] = '\0';
        std::cout << "Server: " << message << std::endl;
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    InetPton(AF_INET, L"127.0.0.1", &serverAddr.sin_addr);
    serverAddr.sin_port = htons(8080);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed.\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server.\n";

    // Start a thread to receive messages from the server
    std::thread receiveThread(receiveMessages, clientSocket);

    // Main thread to send messages to the server
    std::string message;
    std::string roomChoice;
    std::cout << "Select chat room(1,2,3,4,5).\n";
    std::getline(std::cin, roomChoice);
    sendText(roomChoice, clientSocket);
    while (true) {
        std::getline(std::cin, message);
        sendText(message, clientSocket);
        //send(clientSocket, message.c_str(), message.size() + 1, 0);
    }

    // Close the socket and clean up
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}