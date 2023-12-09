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

std::string getIpAddress() {
    const char* interface = "wlan0";

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) { // Child process
        // Close the read end of the pipe
        close(pipe_fd[0]);

        // Redirect stdout to the write end of the pipe
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);

        // Execute the command using execl
        execl("/bin/sh", "sh", "-c", ("ifconfig " + std::string(interface) + " | grep 'inet ' | awk '{print $2}'").c_str(), NULL);

        // If execl fails
        perror("execl");
        exit(1);
    } else { // Parent process
        // Close the write end of the pipe
        close(pipe_fd[1]);

        // Read the output from the pipe
        char buffer[128];
        ssize_t bytesRead = read(pipe_fd[0], buffer, sizeof(buffer));
        close(pipe_fd[0]);

        std::string ipAddress(buffer, bytesRead - 1);
        return ipAddress;
    }
}
