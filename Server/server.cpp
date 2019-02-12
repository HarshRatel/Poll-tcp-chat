#include <iostream>
#include <algorithm>
#include <set>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#define POLL_SIZE 2048
#define PORT 6666
int set_nonblock(int fd)
{
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}

int main(int argc, char** argv)
{
    int masterSocket;
    int opt = 1;
    std::set<int> slaveSockets;

    struct sockaddr_in sockAddr;
    if ((masterSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// Forcefully attaching socket to the port 8080
	if (setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
												  &opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = INADDR_ANY;
	sockAddr.sin_port = htons( PORT );

	if (bind(masterSocket, (struct sockaddr *)&sockAddr,
								 sizeof(sockAddr))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

    set_nonblock(masterSocket);

    listen(masterSocket, SOMAXCONN);

    struct pollfd polls[POLL_SIZE];

    polls[0].fd = masterSocket;
    polls[0].events = POLLIN;



    while(true)
    {
        unsigned int index = 1;
        for(auto iter = slaveSockets.begin();
                    iter != slaveSockets.end();
                    ++iter)
        {
            polls[index].fd = *iter;
            polls[index++].events = POLLIN;
        }

        unsigned int pollsSize = 1 + slaveSockets.size();

        poll(polls, pollsSize, -1);

        for(unsigned int i = 0; i < pollsSize; ++i)
        {
            if(polls[i].revents == POLLIN)
            {
                if (i)
                {
                    static char buffer[1024];;
                    int recvSize = recv(polls[i].fd, buffer, 1024, MSG_NOSIGNAL);
                    std::cout << buffer << std::endl;
                    if((recvSize == 0) && (errno != EAGAIN))
                    {
                        shutdown(polls[i].fd, SHUT_RDWR);
                        close(polls[i].fd);
                        slaveSockets.erase(polls[i].fd);
                    }
                    else if(recvSize > 0)
                    {
                        send(polls[i].fd, buffer, recvSize, MSG_NOSIGNAL);
                    }
                }
                else
                {
                    int slaveSocket = accept(masterSocket, 0, 0);
                    set_nonblock(slaveSocket);
                    slaveSockets.insert(slaveSocket);
                }
            }
        }
    }
}

