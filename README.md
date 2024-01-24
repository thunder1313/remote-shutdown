# Program to remotely shutdown clients

## Description
The program allows clients to connect to the server and shutdown other clients. Administrators can add permissions for clients to shutdown other clients.

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
By default the client with client_id = 0 is administrator.

## Project 