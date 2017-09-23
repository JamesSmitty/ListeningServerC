#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sstream>

#define MYPORT "3490"   //Port to listen on
#define BACKLOG 1000    //number of connections allowed on the incoming queue
#define RECVBUFFER 3000 //the buffer size for receiving data from client

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int sendHelper(int sockfd, const char *msg) //streamlines the sending
{
    int stat = send(sockfd, msg, strlen(msg), 0);
    if (stat == -1)
    {
        perror("send");
    }
    else
    {
        printf("%d bytes of %lu total were sent through socketfd #%d\n", stat, strlen(msg), sockfd);
    }
    return stat;
}

const char *recvGet(int sockfd, int buffersize)
{ //receive the command and return the file contents requested
    char recvBuffer[buffersize];
    int stat = recv(sockfd, recvBuffer, buffersize, 0);
    printf("Client: %s\n", recvBuffer);
    if (stat == -1)
    {
        perror("recv");
    }
    else
    {
        printf("%d bytes were received through #%d\n", stat, sockfd);
        std::string str(recvBuffer);
        std::ifstream t(str.substr(4).c_str()); //slice off the 'GET ' from filepath
        std::stringstream buffer;
        buffer << t.rdbuf();
        return (buffer.str()).c_str();
    }
    return "error receiving information from client";
}

int main()
{
    printf("Starting server setup...\n");

    int listenSocketfd;
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo; // will point to the results

    memset(&hints, 0, sizeof hints); // make sure the structure is empty to fill with info
    hints.ai_family = AF_UNSPEC;     // We dont care if the address is IPV4 or IPV6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // Automatically assign the address of my local host to the socket structures

    /*
int getaddrinfo(const char *node,     // e.g. "www.example.com" or IP
                const char *service,  // e.g. "http" or port number
                const struct addrinfo *hints, //addrinfo structure that specifies criteria for selecting the socket address structures returned in the list pointed to by res
                struct addrinfo **res);
                */

    if ((status = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) //if there is an error with allocating and initializing the linked list of addrinfo structures, then return an error message and exit
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    //servinfo is now a pointer to a linked list of addrinfo structs

    //Use servinfo here until it is of no use: ---------------------------------------------------------------------------------------------------------

    //loop through the linked list and bind to the first working addrinfo struct
    struct addrinfo *p;
    for (p = servinfo; p != NULL; p = p->ai_next)
    {

        //int socket(int domain, int type, int protocol) returns the socket descriptor to be used in later system calls
        if ((listenSocketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        { //if there is an issue setting up a socket, print the error and exit
            perror("server: socket");
            continue;
        }
        //int setsockopt(int socket, int level, int option_name,const void *option_value, socklen_t option_len) set the option specified by the option_name argument, at the protocol level specified by the level argument, to the value pointed to by the option_value argument for the socket associated with the file descriptor specified by the socket argument.
        int optval = 1;
        if (setsockopt(listenSocketfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) //allows bind() to reuse local addresses
        {
            perror("setsockopt");
            exit(1);
        }
        // int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
        if ((status = bind(listenSocketfd, servinfo->ai_addr, servinfo->ai_addrlen)) != 0)
        { //if there is an error with binding the socket to a port on the host computer, print the error and exit
            close(listenSocketfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    //Below this point servinfo is of no use so free the linked list from memory -----------------------------------------------------------------------
    freeaddrinfo(servinfo);

    //int listen(int sockfd, int backlog);
    if ((status = listen(listenSocketfd, BACKLOG)) != 0)
    { //if there is an issue listening on a specific port, print the error and exit
        perror("listen");
        exit(1);
    }

    socklen_t lenSockAddr = sizeof(struct sockaddr);
    struct sockaddr_in foo;
    getsockname(listenSocketfd, (struct sockaddr *)&foo, &lenSockAddr);
    fprintf(stderr, "Server is listening on %s:%d\n", inet_ntoa(foo.sin_addr), ntohs(foo.sin_port));

    //int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

    //main accept loop
    struct sockaddr_storage client_addr; //connector's address information
    socklen_t client_addr_size;          //size of the peer
    int new_fd;                          //new connection put on new_fd
    char s[INET6_ADDRSTRLEN];            // string that is the length of an ipv6 address
    client_addr_size = sizeof(client_addr);
    
    while (true) //infinite serving loop
    {
        new_fd = accept(listenSocketfd, (struct sockaddr *)&client_addr, &client_addr_size); //accept new connection and save the socket fd for this connection
        if (new_fd == -1) //check for error
        {
            perror("accept");
            continue;
        }

        inet_ntop(client_addr.ss_family, //convert ip address from binary to a human readable form
                  get_in_addr((struct sockaddr *)&client_addr),
                  s,
                  sizeof(s));

        printf("Connection received from %s\n", s);

        const char *fileContents = recvGet(new_fd, RECVBUFFER); //receive information from client, get the file contents, and save them

        sendHelper(new_fd, fileContents); //send file contents to client
        printf("Connection closed\n");
        close(new_fd); //close the socket and move to next client
    }
    return 0;
}