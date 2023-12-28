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
#include <future>

// for random number generation
#include <cstdlib>  
#include <ctime>

int dataSocket;
int configSocket;
std::vector<ClientNode> clients;

void handleClientConfigSocket(int serverSocket, int configSocket, int calibrationImageSocket) {
    char buffer[BUFFER_SIZE];
    char tmpBuffer[BUFFER_SIZE];
    Configuration config;

    while (true) {
        memset(buffer, '\0', BUFFER_SIZE);
        int bytesRead = recv(configSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead == 0) {
            // Client disconnected
            break;
        } else if (bytesRead == -1) {
            // Handle error
            std::cerr << "Error receiving data from client." << std::endl;
            close(configSocket);
            break;
        }

        std::cout << buffer << std::endl;
        readConfigurationsFile("configurations.txt", config);

        memset(tmpBuffer, '\0', BUFFER_SIZE);
        strcpy(tmpBuffer, buffer);

        char* token = strtok(buffer, " ");
        if(strcmp(token, "precision") == 0) {
            // precision 256 100
            token = strtok(NULL, " ");
            config.horizontal_precision = atoi(token);
            token = strtok(NULL, " ");
            config.vertical_precision = atoi(token);
        } else if(strcmp(token, "four_points") == 0) {
            // four_points 0.0 0.0 100.0 100.0
            token = strtok(NULL, " ");
            config.top_left_x = atof(token);
            token = strtok(NULL, " ");
            config.top_left_y = atof(token);
            token = strtok(NULL, " ");
            config.bottom_right_x = atof(token);
            token = strtok(NULL, " ");
            config.bottom_right_y = atof(token);
            broadcastMessage(tmpBuffer);
        } else if(strcmp(token, "command") == 0) {
            token = strtok(NULL, " ");
            if(strcmp(token, "cancel") == 0) {
                pthread_mutex_lock(&scannerStateMutex);
                scannerState = CANCELLED;
                pthread_mutex_unlock(&scannerStateMutex);
                broadcastMessage("scanner_state CANCELLED");
            } else if(strcmp(token, "start") == 0) {
                pthread_mutex_lock(&scannerStateMutex);
                scannerState = RUNNING;
                pthread_mutex_unlock(&scannerStateMutex);
                broadcastMessage("scanner_state RUNNING");

                std::thread tScanner(mainScanner, std::ref(serverSocket));
                tScanner.detach();
            } else if(strcmp(token, "calibration_image") == 0) {
                sendImageForCalibration(calibrationImageSocket);
            }
        }

        writeConfigurationsFile("configurations.txt", config);
    }
}


// TODO: clientlara mesaj broadcast edecek bir fonksiyon
void broadcastMessage(const char* message) {
    char buffer[BUFFER_SIZE];
    for(int i=0; i<clients.size(); i++) {
        memset(buffer, '\0', BUFFER_SIZE);
        sprintf(buffer, "%s", message);
        std::cout << "broadcastmessage: " << buffer << std::endl;
        send(clients.at(i).broadcastSocket, buffer, BUFFER_SIZE, 0);
    }
}


void handleClient(int& serverSocket) {
    char buffer[BUFFER_SIZE];

    while(1) {
        // scanner is at idle state
        // std::cout << "inside main loop\n";

        bool scannerActive = false;
        pthread_mutex_lock(&scannerStateMutex);
        if(scannerState == RUNNING) {
            scannerActive = true;
        }
        pthread_mutex_unlock(&scannerStateMutex);

        if(!scannerActive) {
            usleep(1000);
            continue;
        }

        while(1) {
            std::thread tScanner(mainScanner, std::ref(serverSocket));
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
