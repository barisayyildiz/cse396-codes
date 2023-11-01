#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    int serverSocket, clientSocket;
    int port = 12345; // Port number for communication

    // Create a socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    // Bind the socket
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error binding the socket" << std::endl;
        return -1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Error listening" << std::endl;
        return -1;
    }

    std::cout << "Server is listening on port " << port << std::endl;

    // Accept a client connection
    struct sockaddr_in clientAddress;
    socklen_t clientAddressSize = sizeof(clientAddress);
    clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);

    if (clientSocket == -1) {
        std::cerr << "Error accepting client connection" << std::endl;
        return -1;
    }

    char buffer[1024];
    ssize_t bytesRead;

    while (true) {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    
        if (bytesRead <= 0) {
            break; // Connection closed
        }

        // Process the received data (you can replace this with your logic)
        std::string receivedData(buffer, bytesRead);
        std::cout << "Received: " << receivedData << std::endl;
    }

    close(clientSocket);
    close(serverSocket);

    return 0;
}
