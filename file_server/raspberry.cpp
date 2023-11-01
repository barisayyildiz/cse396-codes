#include <iostream>
#include <fstream>  // Include the <fstream> header for file handling
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <cstdlib>
#include <ctime>
#include <signal.h>

int fileSocket;

#define DATA_PORT 12345

int main() {
    std::cout << "hello world" << std::endl;

    if ((fileSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(DATA_PORT);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fileSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error connecting to the server." << std::endl;
        return 1;
    }

    std::string file_path = "3d.obj";

    // Open the file for reading
    std::ifstream file("3d.obj", std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open the file." << std::endl;
        return 1;
    }

    const int chunk_size = 1024;
    char buffer[chunk_size];

    while (!file.eof()) {
        file.read(buffer, chunk_size);
        int bytes_read = file.gcount();

        if (bytes_read > 0) {
            send(fileSocket, buffer, bytes_read, 0);
        }
    }

    file.close();
    close(fileSocket);

    return 0;
}
