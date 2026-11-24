#include <cstdio>
#include <iostream>
#include <string>
using namespace std;

#ifdef _WIN32
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
using Socket = SOCKET;
constexpr Socket invalid_socket = INVALID_SOCKET;
void close_socket(Socket sock) { ::closesocket(sock); }
#else
#include <csignal>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using Socket = int;
constexpr Socket invalid_socket = -1;
void close_socket(Socket sock) { ::close(sock); }
#endif

bool socket_error(const char* operation) {
#ifdef _WIN32
	const int code = ::WSAGetLastError();
	cerr << operation << " failed, (winsock error " << code << ")\n";
#else
	Perror(operation);
#endif
	return false;
}

bool send_byte(Socket sock, char byte) {
    const auto count = ::send(sock, &byte, 1, 0);
    if (count < 0) return socket_error("send");
    return count == 1;
}

bool expect_byte(Socket sock, char expected) {
    char byte = '\0';
    const auto count = ::recv(sock, &byte, 1, 0);
    if (count < 0) return socket_error("recv");
    if (count == 0) {
        cerr << "Peer closed socket before sending expected byte.\n";
        return false;
    }
    if (byte != expected) {
        cerr << "Expected byte is " << expected << ", received " << byte << "\n";
        return false;
    }
    return true;
}

bool run_broker(Socket listener, const sockaddr* address, int length) {
    if (::bind(listener, address, length) != 0) return socket_error("bind");
    if (::listen(listener, 1) != 0) return socket_error("listen");

    cout << "Broker waiting for a worker..." << endl;
    const Socket worker = ::accept(listener, nullptr, nullptr);
    if (worker == invalid_socket) return socket_error("accept");

    const bool acknowledged = send_byte(worker, 'P') && expect_byte(worker, 'A');
    close_socket(worker);
    if (acknowledged) cout << "Broker received A (ACK)\n";
    return acknowledged;
}

bool run_worker(Socket broker, const sockaddr* address, int length) {
    if (::connect(broker, address, length) != 0) return socket_error("connect");

    if (!expect_byte(broker, 'P')) return false;
    cout << "Worker received P\n";

    if (!send_byte(broker, 'A')) return false;
    cout << "Worker sent A (ACK)\n";
    return true;
}

int main(int argc, char* argv[])
{
    const string role = argc == 2 ? argv[1] : "";
    if (role != "broker" && role != "worker") {
        cerr << "Usage: lopback_ack broker|worker\n";
        return 1;
    }
#ifdef _WIN32
    WSADATA data{};
    const int status = WSAStartup(MAKEWORD(2, 2), &data);
    if (status != 0) {
        cerr << "WSAStartup failed with error code " << status << "\n";
        return 1;
    }
#else
    signal(SIGPIPE, SIG_IGN);
#endif

    sockaddr_in endpoint{};
    endpoint.sin_family = AF_INET;
    endpoint.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    endpoint.sin_port = htons(50555);
    const auto* address = reinterpret_cast<const sockaddr*>(&endpoint);
    const int length = static_cast<int>(sizeof(endpoint));

    bool ok = false;
    const Socket sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == invalid_socket) {
        socket_error("socket");
    }
    else {
        ok = role == "broker" ? run_broker(sock, address, length) : run_worker(sock, address, length);
        close_socket(sock);
    }
#ifdef _WIN32
    ::WSACleanup();
#endif
    return ok ? 0 : 1;
}

