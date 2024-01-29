#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#define MAX_CONNECTIONS 10

int sockfd;
int peer_sockets[MAX_CONNECTIONS];
int peer_ports[MAX_CONNECTIONS];
int num_connections = 0;

int d_PORT;
char d_ADRR[20];
int PORT_server;
pthread_t tid;
bool shouldRun = true; // 控制phtread

void sending();
void receiving(int server_fd);
void *receive_thread(void *server_fd);
char autoMessage[2000] = {0};

int findPeerIndex(int port) {
    for (int i = 0; i < num_connections; ++i) {
        if (peer_ports[i] == port) {
            return i;
        }
    }
    return -1;  // Not found
}

int createConnection(int port, const char* addr) {
    struct sockaddr_in serv_addr;
    int sock;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(addr);
    serv_addr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return -1;
    }

    // Add the new connection to the list
    peer_sockets[num_connections] = sock;
    peer_ports[num_connections] = port;
    ++num_connections;

    return sock;
}


int main(int argc, char const *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <Port>\n", argv[0]);
        exit(1);
    }

    strcpy(d_ADRR, argv[1]);
    PORT_server = atoi(argv[2]);

    int server_fd;
    struct sockaddr_in address;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    while(1) {
        
        int peerIndex = findPeerIndex(PORT_server);
        int sock;

        if (peerIndex == -1) {
            // Peer not found, create a new connection
            sock = createConnection(PORT_server, d_ADRR);
            if (sock == -1) {
                printf("Error creating a new connection\n");
                return 0;
            }
        } else {
            // Use the existing connection
            sock = peer_sockets[peerIndex];
        }

        // Send and receive messages with the peer
        char message[100];
        char name[50];
        int port;
        printf("Enter message: ");
        scanf("%s", message);

        size_t len = strlen(message);
        if (len > 0 && message[len - 1] == '\n') {
            message[len - 1] = '\0';
        }

        if (strcmp(message, "Exit") == 0) {  
            shouldRun = false;
            pthread_cancel(tid);
            
            send(sock, message, strlen(message), 0);
            char receiveMessage[100] = {};
            recv(sock, receiveMessage, sizeof(receiveMessage), 0);
            printf("%s\n", receiveMessage);
            printf("***********Session***********\nConnect closed\n"); 
            break;
        } 
        else if (sscanf(message, "%*[^#]#%*[^#]#%*[^#]") == 0) {

            int portDes;
            printf("Enter your destenation port:");
            scanf("%d", &portDes);

            const char addr[] = "127.0.0.1";
            sock = createConnection(portDes, addr);
            if (sock == -1) {
                printf("Error creating a new connection\n");
                return 0;
            }
            send(sock, message, strlen(message), 0);

            peerIndex = findPeerIndex(PORT_server);
            sock = peer_sockets[peerIndex];

            char receiveMessage[100] = {};
            recv(sock, receiveMessage, sizeof(receiveMessage), 0);
            printf("\n--------------------");
            printf("\n%s", receiveMessage);

            printf("--------------------");

            printf("\nUpdate List\n");
            send(sock, "List", strlen("List"), 0);
            memset(receiveMessage, 0, sizeof(receiveMessage));
            recv(sock, receiveMessage, sizeof(receiveMessage), 0);
            printf("%s", receiveMessage);
            printf("--------------------\n");
        } 
        else {
            send(sock, message, strlen(message), 0);
            printf("--------------------\n");
            printf("Awaiting server response...\n");
            char receiveMessage[100] = {};
            recv(sock, receiveMessage, sizeof(receiveMessage), 0);
            printf("%s", receiveMessage);
            printf("--------------------\n");
            if (sscanf(message, "%[^#]#%d", name, &port) == 2) {
                if (!strcmp(receiveMessage,"220 AUTH FAIL\n")==0) {
                    // Forcefully attaching socket to the port
                    address.sin_family = AF_INET;
                    address.sin_addr.s_addr = inet_addr("127.0.0.1");
                    address.sin_port = htons(port);

                    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
                    {
                        perror("bind failed");
                        exit(EXIT_FAILURE);
                    }
                    if (listen(server_fd, 5) < 0)
                    {
                        perror("listen");
                        exit(EXIT_FAILURE);
                    }

                    //Creating thread to keep receiving message in real time
                    pthread_create(&tid, NULL, &receive_thread, &server_fd);
                }  
            }
        }
    }

    //關閉所有連線
    for (int i = 0; i < num_connections; i++) {
        close(peer_sockets[i]);
    }
    return 0;
}


//Calling receiving every 2 seconds
void *receive_thread(void *server_fd)
{
    int s_fd = *((int *)server_fd);
    while (shouldRun)
    {
        receiving(s_fd);
    }
    return NULL;
}

//Receiving messages on our port
void receiving(int server_fd)
{
    struct sockaddr_in address;
    char buffer[2000] = {0};
    int addrlen = sizeof(address);
    fd_set current_sockets, ready_sockets;

    //Initialize my current set
    FD_ZERO(&current_sockets);
    FD_SET(server_fd, &current_sockets);
    int k = 0;
    while (1)
    {
        if (!shouldRun) {
            break;
        }

        k++;
        ready_sockets = current_sockets;

        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0)
        {
            perror("Error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &ready_sockets))
            {

                if (i == server_fd)
                {
                    // Handle server socket
                    int client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    FD_SET(client_socket, &current_sockets);
                    
                }
                else
                {
                    // handle peer socket
                    int portServer = PORT_server;
                    int peerIndex = findPeerIndex(portServer);
                    int sock;
                    if (shouldRun){
                        recv(i, buffer, sizeof(buffer), 0);
                        printf("\n%s\n", buffer);
                        printf("Auto Transfer To Tracker\n");
                        printf("--------------------\n");
                        peerIndex = findPeerIndex(portServer);
                        sock = peer_sockets[peerIndex];
                        send(sock, buffer, strlen(buffer), 0);
                    }
                }
                
            }
        }
        if (k == (FD_SETSIZE * 2))
            break;
    }
}