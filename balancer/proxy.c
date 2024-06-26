#include "proxy.h"

/* Creation of sockets.
=======================================================================
| Sets up a socket for the proxy, configuring it with IPv4 and TCP,   |
| binding it to a specified IP and port, and setting it to listen     |
| for incoming connections.                                           |
=======================================================================
*/
int createSocket(char* ip, char* port, int queue)
{
    struct addrinfo hints, *res;
    
    // Load up the address structs with getaddrinfo():
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // use IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me
 
    if (getaddrinfo(ip, port, &hints, &res) != 0){
        perror("GET ADDRESS INFO");
        return -1;
    }
 
    // Define the socket file descriptor to listen for incomming connections
    int sockfd;
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0){
        perror("SOCKET");
        return -1;
    }
 
    // Bind the socketfd to the port
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) != 0){
        perror("BINDING");
        return -1;
    }
 
    // Define the size of the queue to listen for incomming connections
    if (listen(sockfd, queue) != 0){
        perror("LISTENING");
        return -1;
    }
    freeaddrinfo(res);
    return sockfd;
}

// Acceptance.
int acceptConnection(int sockfd)
{
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);

    int clientfd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

    if (clientfd == -1)
    {
        perror("ACCEPT");
        return 1;
    }

    return clientfd;
}

//Conects to a server specified by IP and port
int connectSocket(char *ip, char *port)
{
    struct addrinfo hints, *res;

    // first, load up the address structs with getaddrinfo():

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // use IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    if (getaddrinfo(ip, port, &hints, &res) != 0)
    {
        perror("GET ADDRES INFO");
        return -1;
    }

    // define the socket file descriptor to listen for incomming connections

    int sockfd;

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (sockfd < 0)
    {
        perror("SOCKET");
        return -1;
    }

    connect(sockfd, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);

    return sockfd;
}

/* Translation of header request from client to real HTTP request for server.
======================================================================================
|                                INFORMATION RECIEVED:                               |
|                            GET www.mipagina.com HTTP/1.1                           |
|                                                                                    |
|                                  REQUEST TO SERVER:                                |
|                            GET / HTTP/1.1                                          |
|                            Host: www.mipagina.com                                 |
|                            Connection: close                                       |
|                            (Other headers -if nedeed-)                             |
======================================================================================
*/
void format_http_request(const char *input, char *output)
{
    char method[10];    // Buffer for the HTTP method
    char domain[256];   // Buffer for the domain/host
    char version[16];   // Buffer for the HTTP version
    char *resource;     // Pointer to the resource path within the domain string

    // Scan the input string to extract the components.
    // Expected format: "method domain version"
    int scan_count = sscanf(input, "%9s %255s %15s", method, domain, version);

    // Check if the scanning was successful (we expect 3 successful scans).
    if (scan_count == 3) {
        // Find if there's a specific resource path given after the domain.
        resource = strchr(domain, '/');
        if (resource == NULL) {
            // No specific resource path provided, default to root.
            resource = "/";
            // Write a formatted HTTP request and Host header to the output buffer.
            sprintf(output, "%s %s %s\r\nHost: %s\r\n", method, resource, version, domain);
        } else {
            // Resource path found, split the domain and resource for proper Host header.
            *resource = '\0';  // Split the string into domain and resource.
            resource++;        // Move past the '/' to get the correct resource path.
            // Write a formatted HTTP request and Host header to the output buffer.
            sprintf(output, "%s /%s %s\r\nHost: %s\r\n", method, resource, version, domain);
        }
    } else {
        // If scanning failed, copy a default error message into the output.
        strcpy(output, "Invalid request format\r\n");
    }
}

int client = 1;
char server1[] = "18.207.213.167";
char server2[] = "52.71.80.136";
char server3[] = "54.146.184.150";

//Load Balancer with Round Robin
void selectServer(int clientfd, char* httpRequest)
{
    int apachefd;
    switch (client)
    {
    case 1:
        apachefd = connectSocket(server1, "80");
        apachefd++;
        break;
    case 2:
        apachefd = connectSocket(server2, "80");
        apachefd++;
        break;
    case 3:
        apachefd = connectSocket(server3, "80");
        apachefd = 1;
        break;
    default:
        break;
    }

    // Send the header translated to Apache
    send(apachefd, httpRequest, 10000, 0);

    // Recieve the answer of Apache
    char httpResponse[10000];
    recv(apachefd, httpResponse, 10000, 0);

    // Close conection with Apachee
    close(apachefd);

    // Send the answer to the client
    send(clientfd, httpResponse, 10000, 0);

    // Close the conection with the client
    close(clientfd);
}

char quit[5];
int stop = 1;


//Server-side controls
int handleServer(void *args)
{
    while (1)
    {
        printf("Enter 'quit' to exit: ");
        fgets(quit, sizeof(quit), stdin);
        // Remove newline character if present
        quit[strcspn(quit, "\n")] = 0;

        if (strcmp(quit, "quit") == 0)
        {
            stop = 0;
            printf("Exiting...\n");
            break;
        }
    }

    return 0;
}

int close_server()
{
    return stop;
}