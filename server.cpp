#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <winsock2.h>
#include <map>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <queue>
#pragma comment(lib, "ws2_32.lib")
std::mutex consoleMutex;
std::mutex m;
const int BUFSIZE = 2500;
//std::vector<SOCKET> clients;
std::vector<SOCKET> room1;
std::vector<SOCKET> room2;
std::vector<SOCKET> room3;
std::vector<SOCKET> room4;
std::vector<SOCKET> room5;
std::map<std::string, std::vector<SOCKET>> roomset;
struct Message {
	std::string message;
	SOCKET messageSocket;
	std::string rumNum;
	std::string flag;
};

std::mutex messageQueueMutex;
std::condition_variable messageAvailableCondition;
std::queue<Message> messageQueue;
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
void addMessageToQueue(const Message& message) {
	{
		std::lock_guard<std::mutex> lock(messageQueueMutex);
		messageQueue.push(message);
	}
	messageAvailableCondition.notify_one();
}
void PutToClient(const int clientSocket, std::string name)
{
	std::string filepath = "C:\\Users\\Давід\\source\\repos\\Te\\ClientServer2\\ClientServer2\\" + name;
	std::ifstream file(filepath, std::ios::binary | std::ios::ate);
	std::streamsize fileSize = file.tellg();
	file.seekg(0, std::ios::beg);
	send(clientSocket, (char*)&fileSize, sizeof(std::streamsize), 0);
	std::streamsize totalSent = 0;
	char buffer[BUFSIZE];
	while (totalSent < fileSize)
	{
		std::streamsize remaining = fileSize - totalSent;
		std::streamsize currentChunkSize = (remaining < BUFSIZE) ? remaining : BUFSIZE;
		file.read(buffer, currentChunkSize);
		send(clientSocket, buffer, currentChunkSize, 0);
		std::cout << "Chunk size is:" << currentChunkSize << std::endl;
		totalSent += currentChunkSize;
	}
	file.close();
}
void broadcastMessage(const std::string& message, SOCKET senderSocket, std::string roomNum, std::string flag) {
	std::lock_guard<std::mutex> lock(consoleMutex);
	std::cout << "Client " << senderSocket << ": " << message << std::endl;
	for (SOCKET client : roomset[roomNum]) {
		if (client != senderSocket) {
			if (flag == "text")
			{
				sendText(std::to_string(senderSocket), client);
				sendText(message, client);
			}
			else {
				sendText(std::to_string(senderSocket), client);
				sendText("FILEINCOMING", client);
				sendText(message, client);
				PutToClient(client, message);
			}
		}
	}
}
void broadcastMessages() {
	while (true) {
		std::unique_lock<std::mutex> lock(messageQueueMutex);
		messageAvailableCondition.wait(lock, [] { return
			!messageQueue.empty(); });
		while (!messageQueue.empty()) {
			Message message = messageQueue.front();
			messageQueue.pop();
			broadcastMessage(message.message, message.messageSocket, message.rumNum, message.flag);
		}
	}
}



void GetFromClient(const int clientSocket)
{
	std::string name = receiveText(clientSocket);
	std::streamsize fileSize;
	if (recv(clientSocket, (char*)&fileSize, sizeof(std::streamsize), 0) == SOCKET_ERROR)
	{
		std::lock_guard<std::mutex> lock(consoleMutex);
		std::cout << WSAGetLastError() << std::endl;

	}
	std::ofstream outFile("C:\\Users\\Давід\\source\\repos\\Te\\ClientServer2\\ClientServer2\\" + name, std::ios::binary);
	std::streamsize totalReceived = 0;
	while (totalReceived < fileSize)
	{
		char buffer[BUFSIZE];
		std::streamsize bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		outFile.write(buffer, bytesReceived);
		totalReceived += bytesReceived;
		std::lock_guard<std::mutex> lock(consoleMutex);
		std::cout << "current file size:" << totalReceived << std::endl;
	}
	outFile.close();
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
		if (message == "error") {
			std::lock_guard<std::mutex> lock(consoleMutex);
			std::cout << "Client " << clientSocket << " disconnected.\n";
			break;
		}
		if (tokens[0] == "REJOIN") {
			for (int i = 0; i < roomset[roomNum].size(); i++)
			{
				if (roomset[roomNum][i] == clientSocket) {
					roomset[roomNum].erase(roomset[roomNum].begin() + i);
					break;
				}
			}
			roomNum = tokens[1];
			roomset[roomNum].push_back(clientSocket);
		}
		if (tokens[0] == "FILE")
		{

			GetFromClient(clientSocket);
			std::string flag = "file";
			Message mes{ tokens[1], clientSocket, roomNum, flag };
			addMessageToQueue(mes);
		}
		else {
			Message mes{ message, clientSocket, roomNum, "text" };
			addMessageToQueue(mes);
		}

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
	if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) ==
		SOCKET_ERROR) {
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
	std::thread messageBroadcastThread(broadcastMessages);
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
		clientThread.detach();
	}
	messageBroadcastThread.join();
	closesocket(serverSocket);
	WSACleanup();
	return 0;
}