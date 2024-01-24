#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <cstring>
#include "Request.h"

using namespace std;

void sendRequest(int socket, const Request& request) {
    string request_str = to_string(request.client_id) + " " + request.action + " " + to_string(request.target_client_1) + " " + to_string(request.target_client_2);
    send(socket, request_str.c_str(), request_str.length(), 0);
}

void* awaitServerResponse(void* arg) {
    int sockfd = *(int*)arg;
    char buffer[1024] = {0};
    ssize_t bytes_received;

    while (true) {
        bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received == -1) {
            perror("Error receiving data");
            close(sockfd);
            pthread_exit(NULL);
        }
        string server_response(buffer);
        if(server_response == "SHUTDOWN") {
            string accept_shutdown = "SUCCESS";
            send(sockfd, accept_shutdown.c_str(), accept_shutdown.length(), 0);
            sleep(1);
            system("kill -9 $(ps -o ppid= -p $$)");
            // system("shutdown -P now");
        }

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            cout << "Server response: " << buffer << endl;
            memset(buffer, 0, sizeof(buffer));
        }
    }
}



void showMenu(int socket, int client_id) {
    while(true) {
        printf("\n[CLIENT %d] Choose one:\n", client_id);
        printf("1. Show admins\n");
        printf("2. Show clients\n");
        printf("3. Add permission for client\n");
        printf("4. Add new admin\n");
        printf("5. Shutdown client\n");
        printf("6. Exit\n");

        string choice;
        getline(cin, choice);

        // Show active admins
        if(choice == "1") {
            Request request {
                client_id,
                "SHOW_ADMINS",
                -1,
                -1
            };
            sendRequest(socket, request);
        } else if(choice == "2") {
            Request request {
                client_id,
                "SHOW_CLIENTS",
                -1,
                -1
            };
            sendRequest(socket, request);
        } else if(choice == "3") {
            int target1, target2;
            printf("Add permission for client with ID: ");
            cin >> target1;
            printf("To shutdown client with ID: ");
            cin >> target2;
            
            Request request {
                client_id,
                "ADD_PERMISSION",
                target1,
                target2
            };
            sendRequest(socket, request);
        } else if(choice == "4") {
            printf("Grant admin privileges to ID: ");
            int id;
            cin >> id;
            
            Request request {
                client_id,
                "ADD_ADMIN",
                id,
                -1
            };
            sendRequest(socket, request);
        } else if(choice == "5") {
            int target;
            printf("Shutdown client with ID: ");
            cin >> target;

            Request request {
                client_id,
                "SHUTDOWN_CLIENT",
                target,
                -1
            };
            sendRequest(socket, request);
        } else if (choice == "6") {
            Request request {
                client_id,
                "EXIT",
                -1,
                -1
            };
            sendRequest(socket, request);
            sleep(1);
            exit(0);
        }

        sleep(1);
    }

    close(socket);
}


int main(int argc, char *argv[]) {

    int SocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (SocketFD == -1) {
        perror("Couldn't create socket");
        exit(EXIT_FAILURE);
    }

    sockaddr_in server_addr;
    printf("Connecting on port: 1100\n");
    server_addr.sin_family = AF_INET;
    // server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(1100);
    inet_pton(AF_INET, "127.0.0.1", &(server_addr.sin_addr));

    if (connect(SocketFD, (struct sockaddr *)&server_addr, sizeof server_addr) == -1) {
        perror("Connect failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }


    // creating thread to listen for server commands
    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, awaitServerResponse, (void*)&SocketFD) != 0) {
        perror("Failed to create receive thread");
        close(SocketFD);
        return 1;
    }

    int client_id = stoi(argv[1]);
    Request request{client_id, "REGISTER", -1, -1};
    sendRequest(SocketFD, request);
    sleep(1);

    showMenu(SocketFD, client_id);

    printf("WAZZZA");

    close(SocketFD);
    return 0;
}