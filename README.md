# Program to remotely shutdown clients

## Description
The program allows clients to connect to the server and shutdown other clients. Administrators can add permissions for clients to shutdown other clients, as well as check current connected clients and admins.

## How to run
1. Compile the server and client code:
```zsh
g++ server.cpp -o server
g++ client.cpp -o client
```
2. Run the server from the directory where you have compiled server code from the step earlier:
```bash
./server
```
3. In other terminal window go to your project directory and run a client with a specific client_id:
```bash
./client <client_id> # for example: ./client 0
```
**By default the client with client_id = 0 is administrator.**

## Project
When client connects to the server it gets the confirmation that connection was successful. Then, the client has following options:
1. Show admins (only available for client with admin privileges)
2. Show clients (only available for client with admin privileges)
3. Add admin (only available for client with admin privileges)
4. Add permission (only available for client with admin privileges)
5. Shutdown client (only available for client with admin privileges or client with permission to shutdown chosen client)
6. Exit

When client tries to perform an action without needed privileges, the server sends proper information about this, such as "You don't have admin privileges to perform this action".

When client executes the 'Shutdown client' option on *target_client* then the 'SHUTDOWN' message is sent to *target_client*. The receive thread on the client side is programmed to shutdown itself when it receives this message.

When the server detects the disconnection, it prints proper message - 'Client with ID *disconnected_client_id* disconnected'. When the client is exited via 6th option on the menu, the server prints 'Client with ID *disconnected_client_id* exited'
