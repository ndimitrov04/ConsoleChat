#include <iostream>
#include <WS2tcpip.h>
#include <future>
#include <thread>
#include <chrono>
#include <vector>
#include <sstream>
#include <fstream>
#include <direct.h>

#pragma comment (lib, "ws2_32.lib")

//htons https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-htons
//and ntohs https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-ntohs

class Account {
private:
	std::string m_name;
	std::string m_password;
public:
	Account(std::string name, std::string password) :
		m_name{ name }, m_password{ password }
	{
	}
	std::string Name() {
		return m_name;
	}
	std::string Password() {
		return m_password;
	}
};


// Vector to store sockets
std::vector<SOCKET> sockets;

// Vector to store accounts (this vector will be loaded into from a savefile on program startup
std::vector<Account> accounts;


void saveVectorToFile(std::vector<Account>& accounts);

// Checks if account exists in accounts vector
bool accountExists(std::string nameOfAccount, std::vector<Account> accounts) {
	for (Account account : accounts) {
		if (nameOfAccount == account.Name()) {
			return true;
		}
	}
	return false;
}

bool passwordCorrect(std::string password, std::vector<Account> accounts) {
	for (Account account : accounts) {
		if (password == account.Password()) {
			return true;
		}
	}
	return false;
}

// Create account and add it to accounts vector, important here to pass accs vector by reference
void createAccount(std::vector<Account>& accounts, std::string name, std::string password) {
	Account newAcc(name, password);
	accounts.push_back(newAcc);
	saveVectorToFile(accounts);
}

// Load an account from a vector, look for it by name
Account loadAccount(std::vector<Account>& accounts, std::string nameOfAccount) {
	for (Account account : accounts) {
		if (nameOfAccount == account.Name()) {
			return account;
		}
	}
}

// Saves accounts vector to file
void saveVectorToFile(std::vector<Account>& accounts) {
	std::ofstream file{ "accounts.ngc" };
	for (Account account : accounts) {
		file << account.Name() << "\n";
		file << account.Password() << "\n";
	}
	file << "end";
}

// Loads accounts to accounts vector from a file
void loadVectorFromFile(std::vector<Account>& accounts) {
	std::ifstream file{ "accounts.ngc" };
	int iterator{ 0 };
	int iteration{ 0 };

	std::string name;
	std::string password;

	for (std::string line; getline(file, line);) {
		switch (iteration) {
		case 0:
			name = static_cast<std::string>(line);
			iterator++;
			break;
		case 1:
			password = static_cast<std::string>(line);
			accounts.push_back(Account(name, password));
			iterator = 0;
			break;
		}
		iteration = iterator;
	}
}

// Recieves data from a certain socket and sends it to all other sockets
void recieveData(SOCKET clientSocket, std::vector<SOCKET>& sockets) {
	char buf[4096]; //buffer should be larger in most cases, this is in bytes, so its 4kb
	char name[4096]; //name of client
	char pass[4096]; //password of client
	int type{ 1 }; //datatype, 0 = message, 1 = name, 2 = password
	int bytesRecieved{0};

	// now keep waiting to recieve data
	while (bytesRecieved != SOCKET_ERROR) {
		switch (type) {
		case 1:
			//zero the name buf and get it ready to recieve
			ZeroMemory(name, 4096);
			bytesRecieved = recv(clientSocket, name, 4096, 0);
			if (bytesRecieved == SOCKET_ERROR) {
				std::cerr << "Error in recv()\n";
				closesocket(clientSocket);
				break;
			}
			if (bytesRecieved == 0) {
				std::cout << "Client disconnected\n";
				closesocket(clientSocket);
				break;
			}
			std::cout << "Name recieved\n";
			type = 2;
		case 2:
			//zero the password buf and get it ready to recieve
			ZeroMemory(pass, 4096);
			bytesRecieved = recv(clientSocket, pass, 4096, 0);
			if (bytesRecieved == SOCKET_ERROR) {
				std::cerr << "Error in recv()\n";
				closesocket(clientSocket);
				break;
			}
			else if (bytesRecieved == 0) {
				std::cout << "Client disconnected\n";
				closesocket(clientSocket);
				break;
			}
			else if (accountExists(name, accounts) && passwordCorrect(pass, accounts)) {
				std::cout << "Correct password\n";
				send(clientSocket, "Correct password. Logging in...", 32, 0);
				type = 0;
			}
			else if (accountExists(name, accounts)) {
				std::cout << "Incorrect password\n";
				send(clientSocket, "Incorrect password. Disconnecting...", 37, 0);
				closesocket(clientSocket);
				break;
			}
			else {
				createAccount(accounts, name, pass);
				std::cout << "New acc created\n";
				send(clientSocket, "New account created.", 21, 0);
				type = 0;
			}
		case 0:
			//wait for client to send data
			//recieve data from clientSocket to buffer variable with a buffer length of 4096 in this case, last argument is flags to change 
			//how fucntion works, use 0 for standard
			//recv() directly changes the buffer to the data, but it also returns whether the 
			//function failed or not with a code, which means that the data is written to buf and a return value is returned to bytesRecieved
			bytesRecieved = recv(clientSocket, buf, 4096, 0);
			if (bytesRecieved == SOCKET_ERROR) {
				std::cerr << "Error in recv()\n";
				closesocket(clientSocket);
				break;
			}
			if (bytesRecieved == 0) {
				std::cout << "Client disconnected\n";
				closesocket(clientSocket);
				break;
			}
			for (SOCKET socket : sockets) {
				send(socket, name, sizeof(name), 0);
				send(socket, buf, bytesRecieved + 1, 0);
			}
		}
	}
}

// Waits for connection and appends the connection to the sockets vector
void waitForConnection(SOCKET listeningSocket, std::vector<SOCKET>& sockets) {
	// Wait for connection
	sockaddr_in client;
	int clientSize = sizeof(client);

	//the connection happens here, so accept accepts the client socket to the listening socket
	//NEEDS ASYNC TO BE ABLE TO CONSTANTLY ACCEPT CLIENTS
	SOCKET clientSocket = accept(listeningSocket, (sockaddr*)&client, &clientSize);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "Client socket is invalid!\n";
		return;
	}

	// MAXHOST is the max size of a domain name, MAXSERV is the same but for service
	char host[NI_MAXHOST];		//client's remote name (sometimes works)
	char service[NI_MAXSERV];	//Serivce (i.e. port) the client is connected on
	//ZeroMemory just initializes the a variable (1st argument) to zero.
	//The second argument is the length (the size of block of memory to fill with zeroes, in bytes)
	ZeroMemory(host, NI_MAXHOST);
	ZeroMemory(service, NI_MAXSERV);

	//getnameinfo returns 0 if successful
	//If we can get the domain name (host) and port (service) of the client (client):
	if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
		std::cout << host << " connected on port " << service << "\n";
	}
	//Else if you can't get the domain name, use the IP Address of the client
	//inet_ntop converts an ip (in our case IPV4 IP (AF_INET)) to a standard string
	else {
		inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
		std::cout << host << " connected on port " << ntohs(client.sin_port) << "\n";
	}

	sockets.push_back(clientSocket);
	recieveData(clientSocket, sockets);
}

void main() {
	// Load accounts
	loadVectorFromFile(accounts);

	// Init winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2); //Winsock verson

	int wsOk = WSAStartup(ver, &wsData); //WSASTartup takes 2 arguments, the version which is an input and the WSADATA which is an output, the function writes to WSADATA

	//WSAStartup returns some codes into wsOk, 0 means the function worked correctly, any other code means a fail
	if (wsOk != 0) {
		std::cerr << "Can't initialize winsock!\n";
		return;
	}


	// Create socket
	// Code below means make a socket called listening on IPV4(AF_INET), make it TCP(SOCK_STREAM), no clue what the 0 is for
	// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);

	if (listening == INVALID_SOCKET) {
		std::cerr << "Can't create a socket!\n";
		return;
	}


	// Bind ip address and port to socket
	// Code below means, bind any (INADDR_ANY) ipv4 (AF_INET) ip address and port to socket
	sockaddr_in hint; //sockaddr_in means socket address input
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	hint.sin_addr.S_un.S_addr = INADDR_ANY; //could also use inet_pton in place of INADDR_ANY
	bind(listening, (sockaddr*)&hint, sizeof(hint)); //bind needs the hint to be a reference to a sockaddr type, too complicated to ask why.
	// so, the listening socket is going to be used to send things over and recieve things


	// Set socket for listening
	listen(listening, SOMAXCONN); //listen tells winsock that a socket is for listening, so we tell winsock that listening is for listening, SOMAXCONN is just the number of max connections

	// Wait for connection and start recieving data
	// This will create 15 potential connections, do while true for infinite (caution: can lead to tremendous lag and system crash)
	for (int x{ 0 }; x < 15; x++) {
		//auto con = std::async(std::launch::async, waitForConnection, listening);
		std::thread *threadPointer = new std::thread(waitForConnection, listening, std::ref(sockets)); //can't use &here, due to thread stuff, std ref ensures pass by reference
	}

	while (true) {};//this keeps the program running

	//Cleanup on end
	WSACleanup();

	/*  This is here just so you can start over if you'd like --- end of actual code
	// Wait for connection
	sockaddr_in client;
	int clientSize = sizeof(client);

	//the connection happens here, so accept accepts the client socket to the listening socket
	//NEEDS ASYNC TO BE ABLE TO CONSTANTLY ACCEPT CLIENTS
	SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "Client socket is invalid!\n";
		return;
	}

	// MAXHOST is the max size of a domain name, MAXSERV is the same but for service
	char host[NI_MAXHOST];		//client's remote name (sometimes works)
	char service[NI_MAXSERV];	//Serivce (i.e. port) the client is connected on
	//ZeroMemory just initializes the a variable (1st argument) to zero.
	//The second argument is the length (the size of block of memory to fill with zeroes, in bytes)
	ZeroMemory(host, NI_MAXHOST);
	ZeroMemory(service, NI_MAXSERV);

	//getnameinfo returns 0 if successful
	//If we can get the domain name (host) and port (service) of the client (client):
	if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
		std::cout << host << " connected on port " << service << "\n";
	}
	//Else if you can't get the domain name, use the IP Address of the client
	//inet_ntop converts an ip (in our case IPV4 IP (AF_INET)) to a standard string
	else {
		inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
		std::cout << host << " connected on port " << ntohs(client.sin_port) << "\n";
	}


	// Close listening socket (we connected, we don't need to listen anymore 
	// (you can keep it open if you want more than 1 machine to connect, but need code rework))
	// closesocket(listening);


	// While loop: accept and echo message back to client
	// NOTE:
	// You should really make this asynchrous, so you can actually do stuff with
	// more than 1 pc and more than one data at a time
	// NEED ASYNC
	char buf[4096]; //buffer should be larger in most cases, this is in bytes, so its 4kb

	while (true) {
		//zero the buffer and get it ready to recieve
		ZeroMemory(buf, 4096);

		//wait for client to send data
		//recieve data from clientSocket to buffer variable with a buffer length of 4096 in this case, last argument is flags to change how fucntion works, use 0 for standard
		//recv() directly changes the buffer to the data, but it also returns whether the function failed or not with a code, which means that the data is written to buf and a return value is returned to bytesRecieved
		int bytesRecieved = recv(clientSocket, buf, 4096, 0);
		if (bytesRecieved == SOCKET_ERROR) {
			std::cerr << "Error in recv()\n";
			break;
		}
		if (bytesRecieved == 0) {
			std::cout << "Client disconnected\n";
			break;
		}
		
		//when you recieve the cleints data, send them back something
		//in this case we are sending the same message we recieved
		//https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-send
		send(clientSocket, buf, bytesRecieved + 1, 0);

		//in this case we are sending back a custom message - broken currently, has to be +2 to recieve \n, also sends things twice
		//const char* messageToSendBack{ "Recieved!\n" };
		//send(clientSocket, messageToSendBack, sizeof(messageToSendBack) + 1, 0);
	}

	

	// Close client socket
	closesocket(clientSocket);
	*/

	// Shutdown and clean up winsock
	//WSACleanup();

}