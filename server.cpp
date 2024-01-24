#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <unistd.h>
#include <pthread.h>
#include <vector>
#include <mutex>
#include "Request.h"
#include "Data.h"

using namespace std;

Request parseRequest(const char* buffer) {
    istringstream iss(buffer);
    Request request;
    iss >> request.client_id >> request.action >> request.target_client_1 >> request.target_client_2;
    return request;
}

bool checkAdminPrivileges(int id_to_check, vector<int> admin_ids) {
    for (int id : admin_ids) {
        if (id == id_to_check) {
            return true;
        }
    }
    return false;
}

bool checkPermissions(vector<int>& check_vector, int id_to_check) {
    for(int id : check_vector) {
        if(id == id_to_check) return true;
    }
    return false;
}

void registerClient(Request& request, Data& thread_data) {
    string response = "Connected successfully!";
    int client_id = request.client_id;
    int client_socket = thread_data.client_socket;

    cout << "[REGISTER] Client with ID " << to_string(client_id) << " registered" << endl << endl;

    {
        lock_guard<mutex> lock(thread_data.data_mutex);
        // add client_id to shared client_ids vector
        thread_data.client_ids.push_back(client_id);
        // add client_id and it's socket number to socket_map
        thread_data.socket_map[client_id] = client_socket;
        // add permission for client to shutdown itself
        thread_data.clients_permission_map[client_id].push_back(client_id);
        // reset the request action
        request.action = "";
    }
    // send a response to client
    send(client_socket, response.c_str(), response.length(), 0);
}

void addAdmin(const Request& request, Data& thread_data){
    // check for admin privileges
    if(checkAdminPrivileges(request.client_id, thread_data.admin_ids)){
        lock_guard<mutex> lock(thread_data.data_mutex);
    
        // add admin to admin_ids
        thread_data.admin_ids.push_back(request.target_client_1);
        string response = "Added client with ID " + to_string(request.target_client_1) + " to admin list";
        send(thread_data.client_socket, response.c_str(), response.length(), 0);
    } else {
        string response = "You don't have admin privileges to do this action\n";
        send(thread_data.client_socket, response.c_str(), response.length(), 0);
    }
}

void showAdmins(const Request& request, Data& thread_data) {
    int client_id = request.client_id;
    int client_socket = thread_data.client_socket;
    vector<int>& admin_ids = thread_data.admin_ids;

    if(checkAdminPrivileges(client_id, admin_ids)){
        lock_guard<mutex> lock(thread_data.data_mutex);
    
        string response = "Admin IDs: [";

        for(int id : admin_ids) {
            response += to_string(id) + ", ";
        }

        response = response.substr(0, response.size() - 2) + "]";
        send(client_socket, response.c_str(), response.length(), 0);
    } else {
        string response = "You don't have admin privileges to do this action\n";
        send(client_socket, response.c_str(), response.length(), 0);
    }
}

void showClients(const Request& request, Data& thread_data) {
    int client_id = request.client_id;
    int client_socket = thread_data.client_socket;
    vector<int>& admin_ids = thread_data.admin_ids;
    vector<int>& client_ids = thread_data.client_ids;

    if(checkAdminPrivileges(client_id, admin_ids)) {
        string response = "Active client ids: [";

        lock_guard<mutex> lock(thread_data.data_mutex);
        for(int id : client_ids) {
            response = response + to_string(id) + ", ";
        }

        response = response.substr(0, response.size() - 2) + "]";
        send(client_socket, response.c_str(), response.length(), 0);
    } else {
        string response = "You don't have admin privileges to do this action\n";
        send(client_socket, response.c_str(), response.length(), 0);
    }
    
}

void addPermission(const Request& request, Data& thread_data) {
    int client_id = request.client_id;
    int target_client_1 = request.target_client_1;
    int target_client_2 = request.target_client_2;

    int client_socket = thread_data.client_socket;
    vector<int>& admin_ids = thread_data.admin_ids;
    vector<int>& client_ids = thread_data.client_ids;
    unordered_map<int, vector<int> >& clients_permission_map = thread_data.clients_permission_map;

    string response;
    
    lock_guard<mutex> lock(thread_data.data_mutex);

    // check for admin privileges
    if (checkAdminPrivileges(client_id, admin_ids)) {
        // check if target_client_1 already has a privilege to shutdown target_client_2
        if(!checkPermissions(clients_permission_map[target_client_1], target_client_2)) {
            response = "Permission added!";
            clients_permission_map[target_client_1].push_back(target_client_2);
        } else {
            string response = "This user already has this privilege\n";
        }
    } else {
        string response = "You don't have admin privileges to do this action\n";
    }
    send(client_socket, response.c_str(), response.length(), 0);
}

void deleteClient(Data& thread_data, int id_to_delete) {
    cout <<"Deleting client with ID: " << to_string(id_to_delete) << endl;

    vector<int>& client_ids = thread_data.client_ids;

    for (auto client = client_ids.begin(); client != client_ids.end(); ++client) {
        if (*client == id_to_delete) {
            client_ids.erase(client);
            break; 
        }
    }
}


void shutdownClient(const Request& request, Data& thread_data) {
    char buffer[1024] = {0};

    int client_id = request.client_id;
    int target_client_1 = request.target_client_1;
    int client_socket_to_shutdown = thread_data.socket_map[target_client_1];

    int client_socket = thread_data.client_socket;
    vector<int>& admin_ids = thread_data.admin_ids;
    unordered_map<int, vector<int> >& clients_permission_map = thread_data.clients_permission_map;

    string response;

    lock_guard<mutex> lock(thread_data.data_mutex);

    // check whether client wants to shutdown itself
    if (target_client_1 == client_id) {
        response = "[SHUTDOWN] If you want to shutdown yourself use 6th option Exit in the menu!";
    } else if (
        // check if user is admin or has permission to shutdown client
        checkAdminPrivileges(client_id, admin_ids) ||
        checkPermissions(clients_permission_map[client_id], target_client_1)
        ) {
            string shutdown = "SHUTDOWN";
            int result = send(client_socket_to_shutdown, shutdown.c_str(), shutdown.length(), 0);
            
        // close(client_socket_to_shutdown);
        response = "[SHUTDOWN] Shutdown success!";
    } else {
        response = "[SHUTDOWN] You don't have permission to shutdown this client!";
    }

    send(thread_data.client_socket, response.c_str(), response.length(), 0);
    sleep(2);
}

void exitClient(const Request& request, Data& thread_data) {
    cout << "[EXIT] Client with ID: " << to_string(request.client_id) << " exited" << endl;

    lock_guard<mutex> lock(thread_data.data_mutex);
    thread_data.removeClient(request.client_id);

    close(thread_data.client_socket);
}

void* checkClientConnection(void* arg) {
    Data* thread_data = (Data*)arg;

    while (true) {
        sleep(5);
        {
            lock_guard<mutex> lock(thread_data->data_mutex);
            auto it = thread_data->socket_map.begin();
            while (it != thread_data->socket_map.end()) {
                int client_id = it->first;
                int client_sock = it->second;

                char buffer[1];
                int result = recv(client_sock, buffer, 1, MSG_DONTWAIT);

                if (result == 0) {
                    if (result == 0) {
                        cout << "Client with id: " << to_string(client_id) << " disconnected " << endl;
                    } else {
                        perror("Recv failed");
                    }
                    thread_data->removeClient(client_id);
                    it = thread_data->socket_map.erase(it);
                    continue;
                }
                ++it;
            }
        }
    }
}


void* handleClient(void* arg) {
    Data* thread_data = (Data*)arg;
    int client_sock = thread_data->client_socket;
    unordered_map<int, int>& client_id_sock_map = thread_data->socket_map;
    char buffer[1024];
    memset(&buffer, 0, sizeof(buffer));

    while (true) {
        int result = recv(client_sock, &buffer, sizeof(buffer), 0);
        Request request = parseRequest(buffer);
        
        if(request.action == "ADD_ADMIN") {   
            addAdmin(request, *thread_data);

        } else if (request.action == "SHOW_ADMINS") {
            showAdmins(request, *thread_data);

        } else if (request.action == "REGISTER") {
            registerClient(request, *thread_data);

        } else if (request.action == "ADD_PERMISSION") {
            addPermission(request, *thread_data);

        } else if (request.action == "SHUTDOWN_CLIENT") {
            shutdownClient(request, *thread_data);

        } else if (request.action == "SHOW_CLIENTS") {
            showClients(request, *thread_data);

        } else if (request.action == "EXIT") {
            exitClient(request, *thread_data);
            break;
        } else if (request.action == "SUCCESS") {
            printf("SHUTDOWN SUCCESS");
            break;
        } else {
            break;
        }
    }
    return nullptr;
}


int main(int argc, char *argv[]) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Couldn't create socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    int port = 1100;
    memset(&server_addr, 0, sizeof server_addr);

    server_addr.sin_family = AF_INET;
    // server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof server_addr) == -1) {
        perror("Bind failed");
        return 1;
    }

    if (listen(server_socket, 10) == -1) {
        perror("Listen failed");
        return 1;
    }

    printf("Server online, listening on port %d \n", port);

    vector<pthread_t> thread_ids;
    vector<int> admin_ids;
    vector<int> client_ids;
    unordered_map< int, vector<int> > clients_permission_map;
    unordered_map<int, int> socket_map;
    admin_ids.push_back(0);

    while (true) {
        int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket == -1) {
            perror("Accept failed");
            continue;
        } else {
            printf("[CONNECT] New client connected with socket ID: %i\n", client_socket);
        }

        Data* thread_data = new Data(admin_ids, client_ids, clients_permission_map, socket_map, client_socket);

        pthread_t connection_thread;
        if (pthread_create(&connection_thread, NULL, checkClientConnection, (void*)thread_data) != 0) {
            perror("Failed to create thread: connection_thread");
            close(server_socket);
            return 1;
        }
        thread_ids.push_back(connection_thread);
        pthread_detach(connection_thread);

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handleClient, (void*)thread_data) != 0) {
            perror("Failed to create thread: client_thread");
            close(client_socket);
            delete thread_data;
            continue;
        }

        thread_ids.push_back(client_thread);
        pthread_detach(client_thread);
    }

    // waiting for all threads go down
    for (pthread_t thread_id : thread_ids) {
        pthread_join(thread_id, NULL);
    }

    close(server_socket);
    return EXIT_SUCCESS;  
}
