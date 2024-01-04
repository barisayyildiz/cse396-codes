#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    int clientSocket;

    int port = 12345; // Port number to connect to

    // Create a socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    // Connect to the server
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Connect to localhost (127.0.0.1)

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error connecting to the server" << std::endl;
        return -1;
    }

    for(int i=0; i<10; i++) {
        std::string data = "Data " + std::to_string(i);

        // Send data to the server
        ssize_t bytesSent = send(clientSocket, data.c_str(), data.length(), 0);

        if (bytesSent == -1) {
            std::cerr << "Error sending data" << std::endl;
            break;
        }

        std::cout << "Sent: " << data << std::endl;
        sleep(1); // Sleep for 1 second between each send
    }

    close(clientSocket);

    return 0;
}
