
// Client

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <string>
#include <sstream>

#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define RETURN_ON_COND(x, log) if ((x)) { cout << log << endl; return; }

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

void do_client(const string& addr)
{
	cout << "Quering '" << addr << "'..." << endl;

	int err;
	WSADATA wsaData;

	err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	RETURN_ON_COND(err != 0, "Failed WSAStartup");




	struct addrinfo hints;
	
	SecureZeroMemory((PVOID)&hints, sizeof(struct addrinfo));	
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;


	// 1. getaddrinfo()
	struct addrinfo* serverinfo;
	err = getaddrinfo(addr.c_str(), "4009", &hints, &serverinfo);
	RETURN_ON_COND(err != 0, "Failed getaddrinfo");

	// 2. socket()
	SOCKET sock = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol);
	RETURN_ON_COND(sock == INVALID_SOCKET, "Failed socket()");

	// 3. connect()
	err = connect(sock, serverinfo->ai_addr, (int)serverinfo->ai_addrlen);
	RETURN_ON_COND(err == SOCKET_ERROR, "Failed connect()");

	cout << "Connected to " << format_ip(*(struct sockaddr_in*)&serverinfo->ai_addr) << endl;

	while (true)
	{
		string msg;
		getline(cin, msg);

		if (msg == "exit")
			break;

		err = send(sock, msg.c_str(), msg.length(), 0);
		if (err == -1)
		{
			cout << "Failed send()" << endl;
			break;
		}
	}



	closesocket(sock);
	freeaddrinfo(serverinfo);
	WSACleanup();
}

void main()
{
	cout << "Enter address: ";

	string addr;
	getline(cin, addr);

	cout << endl;

	do_client(addr);
}
