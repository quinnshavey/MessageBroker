// broker.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <arpa/inet.h>
#include <cstdio>
#include <sys/socket.h>
#include <unistd.h>
using namespace std;

int main()
{
    sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(5000);

	if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) != 1) {
		fputs("Could not set the loopback address\n", stderr);
		return 1;
	}

	const auto* endpoint = reinterpret_cast<const sockaddr*>(&address);
    cout << "Broker\n";
}

