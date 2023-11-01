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
// for signal handling
#include <signal.h>
#include <fstream>

#define DATA_PORT 12345

int fileSocket;

int main() {
    std::cout << "hello world" << std::endl;

    fileSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (fileSocket == -1) {
        std::cerr << "Error creating client socket." << std::endl;
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(DATA_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(fileSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error binding server socket.";
        return 1;
    }

    if (listen(fileSocket, SOMAXCONN) == -1) {
        std::cerr << "Error listening for incoming connections.";
        return 1;
    }

    std::cout << "server is listening...\n";

    sockaddr_in clientAddress;
    socklen_t clientAddressSize = sizeof(clientAddress);
    int clientSocket = accept(fileSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
    if (clientSocket == -1) {
        std::cerr << "Error accepting the connection.";
        return 1;
    }

    std::ofstream file("received3d.obj", std::ios::binary);

    while (true) {
        char buffer[1024]; // Buffer to store received data
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        std::cout << "bytesRead: " << bytesRead << std::endl;
        if (bytesRead == -1 || bytesRead == 0) {
            std::cerr << "error receiving data.";
            close(fileSocket);
            break;
        }
        file.write(buffer, bytesRead);
    }


    file.close();
    close(fileSocket);
    close(clientSocket);

    return 0;
}
