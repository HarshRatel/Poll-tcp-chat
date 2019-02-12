#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>

#define PORT 6666

int GetMessage(int sock)
{
	char buffer[1024];
	int recvSize;

	while(1)
	{
		recvSize = recv(sock, buffer, 1024, MSG_NOSIGNAL);
		if (recvSize < 0)
		{
		   shutdown(sock, SHUT_RDWR);
		   close(sock);

		   throw std::runtime_error("Server connection refused");
		}

		if (recvSize > 0)
			 std::cout << buffer << std::endl;
	}

	return 0;
}

int SendInput(int sock)
{
	while(1)
	{	char buffer[1024];

		std::cin >> buffer;

		send(sock, buffer, sizeof(buffer), MSG_NOSIGNAL);
	}

	return 0;
}

int main(int argc, char **argv)
{
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    std::cout << "Started" << std::endl;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(sock, (sockaddr *)&addr, sizeof(addr)) == -1)
    	throw std::runtime_error("connect() error");

    std::thread reading(GetMessage, std::ref(sock));
    std::thread sending(SendInput, std::ref(sock));

    reading.join();
    sending.join();

    shutdown(sock, SHUT_RDWR);
    close(sock);

    return 0;
}
