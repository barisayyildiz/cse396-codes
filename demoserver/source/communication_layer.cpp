#include "../header/communication_layer.h"
#include "../header/scanner.h"

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>

// for random number generation
#include <cstdlib>  
#include <ctime>

int dataSocket;
int configSocket;

// reads all kinds of messages
/*
    COMMAND CANCEL
    precision 256 100
    four_points 0.0 0.0 100.0 0.0 100.0 100.0 0.0 100.0
*/
int readConfig() {
    int socketId = socket(AF_INET, SOCK_STREAM, 0);
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
        std::cerr << "Error binding readConfig socket." << std::endl;
        return 1;
    }

    // Listen for incoming connections
    if (listen(socketId, SOMAXCONN) == -1) {
        std::cerr << "Error listening for incoming connections." << std::endl;
        return 1;
    }

    char buffer[BUFFER_SIZE];

    std::cout << "Server is listening for configuration updates..." << std::endl;

    int clientSocket;
    sockaddr_in clientAddress;
    socklen_t clientAddressSize = sizeof(clientAddress);

    Configuration config;

    while(true) {
        clientSocket = accept(socketId, (struct sockaddr*)&clientAddress, &clientAddressSize);
        if (clientSocket == -1) {
            std::cerr << "Error accepting the connection." << std::endl;
            return 1;
        }

        while(true) {
            memset(buffer, '\0', BUFFER_SIZE);
            int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            if(bytesRead == 0) {
                break;
            }
            if(bytesRead == -1) {
                close(clientSocket);
                break;
            }
            std::cout << buffer << std::endl;
            readConfigurationsFile("configurations.txt", config);

            char* token = strtok(buffer, " ");
            if(strcmp(token, "precision") == 0) {
                // precision 256 100
                token = strtok(NULL, " ");
                config.horizontal_precision = atoi(token);
                token = strtok(NULL, " ");
                config.vertical_precision = atoi(token);
            } else if(strcmp(token, "four_points") == 0) {
                // four_points 0.0 0.0 100.0 0.0 100.0 100.0 0.0 100.0
                token = strtok(NULL, " ");
                config.top_left_x = atof(token);
                token = strtok(NULL, " ");
                config.top_left_y = atof(token);
                token = strtok(NULL, " ");
                config.top_right_x = atof(token);
                token = strtok(NULL, " ");
                config.top_right_y = atof(token);
                token = strtok(NULL, " ");
                config.bottom_right_x = atof(token);
                token = strtok(NULL, " ");
                config.bottom_right_y = atof(token);
                token = strtok(NULL, " ");
                config.bottom_left_x = atof(token);
                token = strtok(NULL, " ");
                config.bottom_left_y = atof(token);
            } else if(strcmp(token, "command") == 0) {
                token = strtok(NULL, " ");
                if(strcmp(token, "cancel") == 0) {
                    pthread_mutex_lock(&scannerStateMutex);
                    scannerState = FINISHED;
                    pthread_mutex_unlock(&scannerStateMutex);
                }
            }

            writeConfigurationsFile("configurations.txt", config);
        }
    }
    
    close(socketId);
}

void handleClient(int& clientSocket) {
    char buffer[BUFFER_SIZE];

    while(1) {
        // scanner is at idle state
        std::cout << "inside main loop\n";

        // waiting for START command
        memset(buffer, '\0', BUFFER_SIZE);
        recv(clientSocket, buffer, BUFFER_SIZE, 0);

        std::cout << "buffer: " << buffer << std::endl;

        while(1) {
            std::thread tScanner(mainScanner, std::ref(clientSocket));
            tScanner.join();
            break;
        }
        std::cout << "scanning has finished\n";
    }

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
