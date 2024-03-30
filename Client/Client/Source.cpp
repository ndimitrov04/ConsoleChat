#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

//Function for recieving messages to client from sever, sock is the listening socket for the current client
void recieveData(SOCKET sock) {
	char buf[4096];

	while (true) {
		// Wait for name
		ZeroMemory(buf, 4096);
		int nameRecieved = recv(sock, buf, 4096, 0);

		if (nameRecieved > 0) {
			//If everything goes well, echo the name to console
			std::cout << std::string(buf, 0, nameRecieved) << ": ";
		}
		// Wait for message
		ZeroMemory(buf, 4096);
		int bytesRecieved = recv(sock, buf, 4096, 0);

		if (bytesRecieved > 0) {
			//If everything goes well, echo the response to console
			std::cout << std::string(buf, 0, bytesRecieved) << "\n";
		}
	}
}


//For some of the things here that aren't commented, refer to the server code comments
void main() {
	std::string ipAddress = "127.0.0.1";		//ip of the server
	int port = 54000;							//listening port on the server

	// Initialize winSock
	WSAData wsaData;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &wsaData);

	if (wsResult != 0) {
		std::cerr << "Can't start winsock, Err#" << wsResult << "\n";
		return;
	}


	// Create socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cerr << "Can't create socket, Err#" << WSAGetLastError() << "\n";
		WSACleanup();
		return;
	}


	// Fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	//https://docs.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-inet_pton
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);


	// Connect to server
	int connectResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if (connectResult == SOCKET_ERROR) {
		std::cerr << "Can't connect to server, Err#" << WSAGetLastError() << "\n";
		closesocket(sock);
		WSACleanup();
		return;
	}


	// Do-While loop to send data
	std::string userInput;
	char pass[4096];

	//Enter name
	std::cout << "Enter name: ";
	getline(std::cin, userInput); //Use this instead of std::cin, as cin will split the input up on transmit
	int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);

	//Enter password
	std::cout << "Enter password: ";
	getline(std::cin, userInput); //Use this instead of std::cin, as cin will split the input up on transmit
	sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);
	
	ZeroMemory(pass, 4096);
	int answerRecieved = recv(sock, pass, 4096, 0);

	if (answerRecieved > 0) {
		//Take answer wheter password is correct or not
		std::cout << std::string(pass, 0, 4096) << "\n";
	}

	//Start message recieve thread
	std::thread* threadPointer = new std::thread(recieveData, sock);

	//Send messages
	do {
		// Wait for the user to enter a message 
		// (the user will see their message twice in this program, once when they type it and once when 
		// they recieve it back from the server, this will not be a problem in the gui version as 
		// I will be able to automatically clear the input textbox once the user has sent a message
		// think discord

		getline(std::cin, userInput); //Use this instead of std::cin, as cin will split the input up on transmit

		// Send the message if the user's message isn't blank
		if (userInput.size() > 0) {
			int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);
		}
	} while (userInput.size() > 0);


	// Close everything
	closesocket(sock);
	WSACleanup();

}