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

#include "header/communication_layer.h"

int dataSocket;
int configSocket;

void signalCallbackHandler(int signum) {
    std::cout << "Caught signal " << signum << std::endl;
    close(dataSocket);
    close(configSocket);
    exit(signum);
}

int main() {
    // handle ctrl+c signal
    signal(SIGINT, signalCallbackHandler);
    srand(time(0));

    std::thread t1(readConfig, std::ref(configSocket));
    t1.detach();

    // Create a socket for the client
    dataSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (dataSocket == -1) {
        std::cerr << "Error creating client socket." << std::endl;
        return 1;
    }

    // Define the server address and port to connect to
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(DATA_PORT);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(dataSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error connecting to the server." << std::endl;
        return 1;
    }

    std::cout << "test.." << std::endl;

    for(int i=0; i<3; i++) {
        // Send data to the server
        int rnd = rand() % 10 + 5;
        std::cout << "generated random number : " << rnd << std::endl;
        std::string dataToSend = std::to_string(rnd);
        if (send(dataSocket, dataToSend.c_str(), dataToSend.size(), 0) == -1) {
            std::cerr << "Error sending data." << std::endl;
        }
        std::cout << "gonna sleep" << std::endl;
        sleep(5);
        std::cout << "woke up" << std::endl;
    }

    while(1){}

    // Close the client socket
    close(dataSocket);

    std::cout << "bye!!" << std::endl;

    return 0;
}
