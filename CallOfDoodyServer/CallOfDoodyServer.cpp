
// Server

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <sstream>
#include <signal.h>
#include <thread>
#include <vector>

#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using std::cout;
using std::endl;
using std::cin;
using std::string;
using std::vector;

struct CallOfDoodyServer
{
	SOCKET socket;
	bool stop;
	bool running;
} g_Server;

struct CallOfDoodyClient
{
	SOCKET socket;
	struct sockaddr_in addr;
};

string format_ip(const struct sockaddr_in& addr)
{
	string addrStr;
	for (int i = 0; i < 4; ++i)
	{
		addrStr += std::to_string((addr.sin_addr.S_un.S_addr >> (i * 8)) & 0xff);
		if (i < 3)
			addrStr += ".";
	}

	return addrStr;
}

void assemble_sockaddr_in(struct sockaddr_in& addr, ADDRESS_FAMILY family, ULONG address, USHORT port)
{
	memset((void*)&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = family;
	addr.sin_addr.S_un.S_addr = address;
	addr.sin_port = htons(port);
}

bool socket_has_data(SOCKET sock, unsigned int timeoutMS = 100)
{
	timeval to;
	to.tv_sec = 0;
	to.tv_usec = timeoutMS * 1000;

	fd_set sockets;
	FD_ZERO(&sockets);
	FD_SET(sock, &sockets);
	if (select(sock + 1, &sockets, 0, 0, &to) == -1)
		return false;

	return FD_ISSET(sock, &sockets) == 1;
}

SOCKET determine_highest_client_socket(const vector<CallOfDoodyClient>& clients)
{
	SOCKET highest = INVALID_SOCKET;
	for (auto itClient = clients.begin(); itClient != clients.end(); ++itClient)
	{
		if (highest == INVALID_SOCKET || itClient->socket > highest)
			highest = itClient->socket;
	}

	return highest;
}

void do_server()
{
	int err = 0;

	cout << "Starting server..." << endl;

	g_Server.stop = false;

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// 1. Assemble addresses
	struct sockaddr_in serverAddr, sourceAddr;
	assemble_sockaddr_in(serverAddr, AF_INET, inet_addr("127.0.0.1"), 4009);
	assemble_sockaddr_in(sourceAddr, AF_INET, INADDR_ANY, 4009);

	// 2. socket()
	g_Server.socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_Server.socket == INVALID_SOCKET)
	{
		cout << "Failed socket()" << endl;
		return;
	}

	// 3. bind()
	err = bind(g_Server.socket, (struct sockaddr*)&sourceAddr, sizeof(sourceAddr));
	if (err != 0)
	{
		cout << "Failed bind()" << endl;
		return;
	}

	// 4. listen()
	err = listen(g_Server.socket, 10);
	if (err != 0)
	{
		cout << "Failed listen()" << endl;
		return;
	}


	unsigned int recvBufSz = 1024;
	char recvBuf[1024];	

	vector<CallOfDoodyClient> clients;
	fd_set clientSocketSet;
	FD_ZERO(&clientSocketSet);
	SOCKET highestClientSocket = INVALID_SOCKET;

	g_Server.running = true;
	cout << "Waiting for clients to connect..." << endl;

	while (!g_Server.stop)
	{
		if (socket_has_data(g_Server.socket, (highestClientSocket != INVALID_SOCKET ? 0 : 100)))
		{
			// Accept the connection request
			CallOfDoodyClient client;
			int addrSz = sizeof(client.addr);
			client.socket = accept(g_Server.socket, (struct sockaddr*)&client.addr, &addrSz);
			if (client.socket > highestClientSocket || highestClientSocket == INVALID_SOCKET)
				highestClientSocket = client.socket;

			clients.push_back(client);
			FD_SET(client.socket, &clientSocketSet);

			cout << "!!! " << format_ip(client.addr) << " connected" << endl;
		}

		// There are clientSockets
		if (highestClientSocket != INVALID_SOCKET)
		{
			timeval to;
			to.tv_sec = 0;
			to.tv_usec = 100 * 1000; // 100ms

			fd_set readySocks = clientSocketSet;
			select(highestClientSocket + 1, &readySocks, 0, 0, &to);

			for (auto itClient = clients.begin(); itClient != clients.end();)
			{
				if (FD_ISSET(itClient->socket, &readySocks) != 1)
				{
					++itClient;
					continue;
				}

				int nbytes = recv(itClient->socket, recvBuf, recvBufSz - 1, 0);
				if (nbytes <= 0)
				{
					closesocket(itClient->socket);
					
					cout << "!!! " << format_ip(itClient->addr) << " (" << std::to_string(itClient->socket) << ") disconnected" << endl;
					
					FD_CLR(itClient->socket, &clientSocketSet);
					itClient = clients.erase(itClient);
					highestClientSocket = determine_highest_client_socket(clients);
					continue;
				}
				else
				{
					recvBuf[nbytes] = 0;
					cout << format_ip(itClient->addr) << " said: " << recvBuf << endl;
				}

				++itClient;
			}
		}
	}

	for (auto itClient = clients.begin(); itClient != clients.end(); ++itClient)
	{
		closesocket(itClient->socket);
	}

	closesocket(g_Server.socket);
	WSACleanup();

	cout << "Stopped server." << endl;
	g_Server.running = false;
}




void main()
{
	int err = 0;

	std::thread serverThread(do_server);

	for (;;)
	{
		string input;
		std::getline(cin, input);

		if (input == "stop")
		{
			g_Server.stop = true;
			serverThread.join();
			break;
		}
		else
		{
			cout << "Invalid command" << endl;
		}
	}
}
