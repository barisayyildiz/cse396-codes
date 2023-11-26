#include "../header/communication_layer.h"

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>

// for random number generation
#include <cstdlib>  
#include <ctime> 

int dataSocket;
int configSocket;
char buffer[BUFFER_SIZE];

int readConfig(int& socketId) {
    socketId = socket(AF_INET, SOCK_STREAM, 0);
    if (socketId == -1) {
        std::cerr << "Error creating server socket." << std::endl;
        return 1;
    }

    // Define the server address and port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(CONFIG_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the server socket to the address and port
    if (bind(socketId, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error binding server socket." << std::endl;
        return 1;
    }

    // Listen for incoming connections
    if (listen(socketId, SOMAXCONN) == -1) {
        std::cerr << "Error listening for incoming connections." << std::endl;
        return 1;
    }

    std::cout << "Server is listening for incoming connections..." << std::endl;

    while (true) {
        // Accept incoming connections
        int clientSocket;
        sockaddr_in clientAddress;
        socklen_t clientAddressSize = sizeof(clientAddress);

        clientSocket = accept(socketId, (struct sockaddr*)&clientAddress, &clientAddressSize);
        if (clientSocket == -1) {
            std::cerr << "Error accepting the connection." << std::endl;
            return 1;
        }

        std::cout << "debug1" << std::endl;

        char buffer[1024]; // Buffer to store received data
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead == -1) {
            std::cerr << "Error receiving data." << std::endl;
            close(clientSocket);
            std::cout << "debug2" << std::endl;
            continue;
        }

        // Process the received data (echo back to the client)
        std::string receivedData(buffer, bytesRead);
        std::cout << "Received: " << receivedData << std::endl;

        // Send the data back to the client
        if (send(clientSocket, buffer, bytesRead, 0) == -1) {
            std::cerr << "Error sending data." << std::endl;
        }

        std::cout << "debug3" << std::endl;

        // Close the client socket
        close(clientSocket);
    }

    std::cout << "debug4" << std::endl;

    // Close the server socket (usually never reached in this example)
    close(socketId);
    
    return 0;
}
