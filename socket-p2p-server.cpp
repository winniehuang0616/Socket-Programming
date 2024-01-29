#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
 
//the thread function
void *connection_handler(void *);

// Structure to hold client information
struct ClientInfo {
    char name[50];
    char ip[16];
    char port[10];
    int money;
    int publicKey;
    int sock;
};

// Array to store client information
struct ClientInfo clients[10];  // Assuming a maximum of 10 clients for simplicity
int numClients = 0;  // Current number of clients

// Function to handle client registration
int handleRegistration(char *name, int sock) {
    for (int i = 0; i < numClients; i++) {
        if (strcmp(clients[i].name, name) == 0) {
            // Client name already exists
            char response[] = "210 FAIL";
            write(sock, response, strlen(response));
            return 0;
        }
    }

    // Client name doesn't exist, register the client
    strcpy(clients[numClients].name, name);
    strcpy(clients[numClients].ip, "");
    strcpy(clients[numClients].port, "");
    clients[numClients].money = 10000;  // Initial money
    clients[numClients].publicKey = numClients + 1;  // Assuming public key is based on the client's position in the array
    clients[numClients].sock = sock;
    numClients++;

    // Send registration response
    char response[50];
    printf("REGISTER#%s\n", name);
    sprintf(response, "100 OK\n");
    write(sock, response, strlen(response));

    return 1;
}

// Function to handle client connection request
void handleConnectionRequest(char *name, char *port, int sock) {
    bool clientFound = false;
    for (int i = 0; i < numClients; i++) {
        if (strcmp(clients[i].name, name) == 0) {
            clientFound = true;
            // Update client information with IP and port
            strcpy(clients[i].ip, "127.0.0.1");  // Assuming a static IP for simplicity
            strcpy(clients[i].port, port);

            char listInfo[200];
            printf("%s#%s\nOnline num: %d", name,port,numClients);
            sprintf(listInfo, "%d\npublic key\n%d", clients[0].money, numClients);


            char clientInfo[100];
            clientInfo[0] = '\0';
            for (int i = 0; i < numClients; i++) {
                if (i==numClients-1) {
                    printf("\n%s#%s#%s\n", clients[i].name, clients[i].ip, clients[i].port);
                    sprintf(clientInfo + strlen(clientInfo), "\n%s#%s#%s\n", clients[i].name, clients[i].ip, clients[i].port);
                }
                else {
                    printf("\n%s#%s#%s", clients[i].name, clients[i].ip, clients[i].port);
                    sprintf(clientInfo + strlen(clientInfo), "\n%s#%s#%s", clients[i].name, clients[i].ip, clients[i].port);
                
                }
            }

            char response[500];
            // Copy listInfo to response
            snprintf(response, sizeof(response), "%s", listInfo);
            // Concatenate clientInfo to response
            snprintf(response + strlen(response), sizeof(response) - strlen(response), "%s", clientInfo);

            write(sock, response, strlen(response));
            return;
        }
    }

    if (!clientFound) {
        // Client not found
        char response[50];
        sprintf(response, "220 AUTH FAIL\n");
        write(sock, response, strlen(response));
    }
}

// Function to handle client list request
void handleListRequest(int sock) {
    int money;
    for (int i = 0; i < numClients; i++) {
        if (clients[i].sock == sock) {
            money = clients[i].money;
            break;
        }
    }

    char listInfo[200];
    printf("List\n");
    sprintf(listInfo, "List\n%d\npublic key\n%d", money, numClients);

    char clientInfo[100];
    clientInfo[0] = '\0';
    for (int i = 0; i < numClients; i++) {
        if (i==numClients-1) {
            sprintf(clientInfo + strlen(clientInfo), "\n%s#%s#%s\n", clients[i].name, clients[i].ip, clients[i].port);
        }
        else {
            sprintf(clientInfo + strlen(clientInfo), "\n%s#%s#%s", clients[i].name, clients[i].ip, clients[i].port);
        }
    }
    char response[500];
    // Copy listInfo to response
    snprintf(response, sizeof(response), "%s", listInfo);
    // Concatenate clientInfo to response
    snprintf(response + strlen(response), sizeof(response) - strlen(response), "%s", clientInfo);
    write(sock, response, strlen(response));
}

// Function to handle money transfer request
void handleTransferRequest(char *sender, int amount, char *receiver, int sock) {
    int socket;
    for (int i = 0; i < numClients; i++) {
        if (strcmp(clients[i].name, sender) == 0) {
            // Deduct money from sender
            clients[i].money -= amount;
            socket = clients[i].sock;
        } else if (strcmp(clients[i].name, receiver) == 0) {
            // Add money to receiver
            clients[i].money += amount;
            //receiver's socket == sock
        }
    }
    // Send transfer response
    char response[] = "Transfer OK!\n";
    write(socket, response, strlen(response));
}

void handleExitRequest(int sock) {
    // Find the client with the given socket descriptor
    int index = -1;
    for (int i = 0; i < numClients; i++) {
        if (clients[i].sock == sock) {
            index = i;
            break;
        }
    }

    if (index != -1) {
        // Remove the client from the array
        for (int i = index; i < numClients - 1; i++) {
            clients[i] = clients[i + 1];
        }

        // Decrement the number of clients
        numClients--;
        printf("Online num: %d\n",numClients);

        char clientInfo[100];
        for (int i = 0; i < numClients; i++) {
            if (numClients==0) {break;}
            if (i==numClients-1) {
                printf("%s#%s#%s\n", clients[i].name, clients[i].ip, clients[i].port);
                sprintf(clientInfo + strlen(clientInfo), "%s#%s#%s", clients[i].name, clients[i].ip, clients[i].port);
            }
            else {
                printf("%s#%s#%s", clients[i].name, clients[i].ip, clients[i].port);    
                sprintf(clientInfo + strlen(clientInfo), "%s#%s#%s\n", clients[i].name, clients[i].ip, clients[i].port);        
            }
        }

        if (numClients==0) {
            char response[] = "Exit";
            write(sock, response, strlen(response));
        } else {
            write(sock, clientInfo, strlen(clientInfo));
        }
    }
}

void handleError(int sock) {
    char response[] = "230 Input Format Error\n";
    write(sock, response, strlen(response));
}
 
int main(int argc, char const *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);

    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    c = sizeof(struct sockaddr_in);
	pthread_t thread_id;
	
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
         
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}

// The thread function
void *connection_handler(void *socket_desc) {
    int sock = *(int *)socket_desc;
    int read_size;
    char client_message[2000];

    // Receive messages from client
    while ((read_size = recv(sock, client_message, 2000, 0)) > 0) {
        // End of string marker
        client_message[read_size] = '\0';

        // Parse and handle client requests
        char name[50]; // Assuming a maximum name length of 50 characters
        char port[10];
        char receiver[50];
        char sender[50];
        int amount;

        if (sscanf(client_message, "REGISTER#%[^#]", name) == 1) {
            handleRegistration(name, sock);
        } else if (sscanf(client_message, "%[^#]#%d#%[^#]", sender, &amount, receiver) == 3) {
            // Handling the format "<NAME1>#<MONEY>#<NAME2>"
            printf("%s\n", client_message);
            handleTransferRequest(sender, amount, receiver, sock);
        } else if (strcmp(client_message, "List") == 0) {
            handleListRequest(sock);
        } else if (sscanf(client_message, "%[^#]#%[^#]", name, port) == 2) {
            // Handling the format "<NAME>#<PORT>"
            handleConnectionRequest(name, port, sock);
        } else if (strcmp(client_message, "Exit") == 0) {
            handleExitRequest(sock);
        } else {
            handleError(sock);
            printf("%s\n", client_message);
        }
    }

    if (read_size == -1) {
        puts("***********Session***********\nConnect closed\n");
    } 

    return 0;
}
 
