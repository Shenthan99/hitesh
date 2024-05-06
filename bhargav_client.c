
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define SERVER_IP "172.28.91.104" // Change this to server IP
#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsa;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    // Create socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }
    printf("Socket created successfully\n");

    // Initialize server address struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    printf("Attempting to connect to server...\n");

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connection failed\n");
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }
    printf("Connected to server\n");

    // Example: Send message to server
    char message[] = "Hello, server!";
    send(client_socket, message, strlen(message), 0);
    printf("Message sent to server: %s\n", message);

    // Example: Receive response from server
    int recv_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (recv_bytes > 0) {
        buffer[recv_bytes] = '\0'; // Null-terminate the received data
        printf("Received response from server: %s\n", buffer);
    }

    // Close socket
    closesocket(client_socket);
    WSACleanup();

    return 0;
}

