#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 3
#define HEARTBEAT_TIMEOUT 5

int client_sockets[MAX_CLIENTS];
int client_ids[MAX_CLIENTS];
int num_clients = 0;
int leader_id = -1;

struct ClientActivity {
    int id;
    time_t last_activity;
};

struct ClientActivity client_activity[MAX_CLIENTS];

void handle_client_disconnection(int client_index) {
    printf("Client %d disconnected.\n", client_ids[client_index]);
    close(client_sockets[client_index]);
    for (int i = client_index; i < num_clients - 1; ++i) {
        client_sockets[i] = client_sockets[i + 1];
        client_ids[i] = client_ids[i + 1];
    }
    num_clients--;
}

void update_client_activity(int client_id) {
    for (int i = 0; i < num_clients; ++i) {
        if (client_ids[i] == client_id) {
            client_activity[i].last_activity = time(NULL);
            break;
        }
    }
}

void check_disconnected_clients() {
    time_t current_time = time(NULL);
    for (int i = 0; i < num_clients; ++i) {
        if (current_time - client_activity[i].last_activity > HEARTBEAT_TIMEOUT) {
            handle_client_disconnection(i);
            if (client_ids[i] == leader_id) {
                leader_id = -1;
                for (int j = 0; j < num_clients; ++j) {
                    if (client_ids[j] > leader_id) {
                        leader_id = client_ids[j];
                    }
                }
                printf("Current leader: %d\n", leader_id);
                break; // Break out of the loop after updating the leader
            }
        }
    }
}

int main() {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        int client_id = num_clients + 1;
        client_sockets[num_clients] = new_socket;
        client_ids[num_clients] = client_id;
        num_clients++;

        printf("Client %d connected.\n", client_id);

        char id_msg[20];
        sprintf(id_msg, "Your ID is: %d\n", client_id);
        send(new_socket, id_msg, strlen(id_msg), 0);

        if (client_id > leader_id) {
            leader_id = client_id;
            printf("Current leader: %d\n", leader_id);
        }

        // Update client activity upon connection
        update_client_activity(client_id);

        for (int i = 0; i < num_clients; ++i) {
            if (client_sockets[i] > 0) {
                char buffer[1024] = {0};
                if (recv(client_sockets[i], buffer, 1024, MSG_PEEK | MSG_DONTWAIT) == 0) {
                    handle_client_disconnection(i);
                    if (client_id == leader_id) {
                        leader_id = -1;
                        for (int j = 0; j < num_clients; ++j) {
                            if (client_ids[j] > leader_id) {
                                leader_id = client_ids[j];
                            }
                        }
                        printf("Current leader: %d\n", leader_id);
                    }
                } else {
                    update_client_activity(client_ids[i]);
                }
            }
        }
        check_disconnected_clients();
    }

    return 0;
}