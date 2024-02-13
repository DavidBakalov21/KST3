#include <iostream>
#include <thread>
#include <string>
#include <winsock2.h>
#include <fstream>
#include <Ws2tcpip.h>
#include <sstream>
#include <filesystem>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
const int BUFSIZE = 2500;
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
void GetF(const std::string& name, const int clientSocket)
{
	//sendText(choice);
	//sendText(name, clientSocket);
	std::streamsize fileSize;
	if (recv(clientSocket, (char*)&fileSize, sizeof(std::streamsize), 0) == SOCKET_ERROR)
	{
		std::cout << "something went wrong when receiving file size" << std::endl;
		std::cout << WSAGetLastError() << std::endl;
	}
	std::cout << "File size is:" << fileSize << std::endl;
	if (!std::filesystem::create_directory("C:\\Users\\Давід\\source\\repos\\Te\\ClientChat2\\ClientChat2\\" + std::to_string(clientSocket)))
	{
		//m.lock();
		//std::cout << "Directory already exists" << std::endl;
		
	}
	std::ofstream outFile("C:\\Users\\Давід\\source\\repos\\Te\\ClientChat2\\ClientChat2\\" + std::to_string(clientSocket) + "\\" + name, std::ios::binary);
	std::streamsize totalReceived = 0;
	while (totalReceived < fileSize)
	{
		char buffer[BUFSIZE];
		std::streamsize bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		outFile.write(buffer, bytesReceived);
		totalReceived += bytesReceived;
		std::cout << "current file size:" << totalReceived << std::endl;
	}
	outFile.close();
}
void receiveMessages(SOCKET clientSocket) {
	while (true) {
		std::string user = receiveText(clientSocket);
		std::string message = receiveText(clientSocket);
		if (message == "error") {
			std::cerr << "Server disconnected.\n";
			break;
		}
		if (message == "FILEINCOMING") {
			std::cout << "Incommming file decline/accept" << std::endl;
			std::string name = receiveText(clientSocket);
			GetF(name, clientSocket);
		}
		else {
			std::cout << "User " << user << ":" << message << std::endl;
		}
	}
}
void SendFile(const std::string& name, const int clientSocket)
{
	sendText("FILE "+name, clientSocket);
	sendText(name, clientSocket);
	std::string filepath =  name;
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
	//std::string confirmation = receiveText();
	//std::cout << confirmation << std::endl;
}


//FILE name
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
	if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) ==
		SOCKET_ERROR) {
		std::cerr << "Connection failed.\n";
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}
	std::cout << "Connected to server.\n";
	std::thread receiveThread(receiveMessages, clientSocket);
	std::string message;
	std::string roomChoice;
	std::cout << "Select chat room(1,2,3,4,5).\n";
	std::getline(std::cin, roomChoice);
	sendText(roomChoice, clientSocket);
	while (true) {

		std::getline(std::cin, message);
		std::vector<std::string> tokens;
		std::string token;
		std::istringstream tokenStream(message);

		while (std::getline(tokenStream, token, ' ')) {
			tokens.push_back(token);
		}
		if (tokens[0] == "FILE") {
			SendFile(tokens[1], clientSocket);
		}
		else {
			sendText(message, clientSocket);
		}
		
		
	}
	closesocket(clientSocket);
	WSACleanup();
	return 0;
}