#include <iostream>
#include <string.h>
#include <vector>
#include <string>
#include <fstream>
#include <string>
#include <sstream>
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

#define BUFFER_SIZE 1024

int dataSocket;
int configSocket;

void signalCallbackHandler(int signum) {
    std::cout << "Caught signal " << signum << std::endl;
    close(dataSocket);
    close(configSocket);
    exit(signum);
}

// RUNNING

// readData, her tur sonunda
// 34,256 -> 34/256 tur
// 250 -> bu turda işlenen nokta sayısı
// -178,0,61 -> 1.nokta
// -174,0,67 -> 2.nokta
// -182,0,56 -> 3.nokta
// ...


// 35,256 -> 35/256 tur
// 247 -> bu turda işlenen nokta sayısı
// -178,0,61 -> 1.nokta
// -174,0,67 -> 2.nokta
// -182,0,56 -> 3.nokta

// FINISHED



int main() {
    // handle ctrl+c signal
    signal(SIGINT, signalCallbackHandler);
    srand(time(0));

    // std::thread t1(readConfig, std::ref(configSocket));
    // t1.detach();

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

    // std::cout << "test.." << std::endl;
    
    std::ifstream inputFile("vertices.txt"); // Replace "filename.txt" with your file's name
    if (!inputFile.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        return 1;
    }

    std::string line;
    char buffer[BUFFER_SIZE];
    
    // memset(buffer, '\0', BUFFER_SIZE);
    // sprintf(buffer, "%s", "RUNNING");
    // printf("RUNNING\n");
    // send(dataSocket, buffer, sizeof(buffer), 0);
    int counter = 0;
    while (std::getline(inputFile, line)) {
        if(counter % 455 == 0) {
            if(counter/455 == 256) {
                break;
            }
            memset(buffer, '\0', BUFFER_SIZE);
            recv(dataSocket, buffer, sizeof(buffer), 0);

            memset(buffer, '\0', BUFFER_SIZE);
            sprintf(buffer, "%d %s", counter/455+1, "256");
            printf("%s\n", buffer);
            send(dataSocket, buffer, sizeof(buffer), 0);
            memset(buffer, '\0', BUFFER_SIZE);
            sprintf(buffer, "%s", "455");
            printf("%s\n", buffer);
            send(dataSocket, buffer, sizeof(buffer), 0);
            // std::cout << "455" << std::endl;
            usleep(300000);
        }
        // // std::istringstream iss(line);
        // std::vecing temp_str;

        // std::cout << line.substr(2) << std::endl;
        memset(buffer, '\0', BUFFER_SIZE);
        strncpy(buffer, line.c_str() + 2, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0'; // Ensure null-termination
        //printf("%s\n", buffer);
        send(dataSocket, buffer, sizeof(buffer), 0);
        recv(dataSocket, buffer, sizeof(buffer), 0);
        // usleep(1000);

        counter++;
    }
    memset(buffer, '\0', BUFFER_SIZE);
    sprintf(buffer, "%s", "FINISHED");
    printf("FINISHED\n");
    send(dataSocket, buffer, sizeof(buffer), 0);
    inputFile.close();

    std::cout << "finished...\n";

    while(1){}

    // Close the client socket
    close(dataSocket);

    std::cout << "bye!!" << std::endl;

    return 0;
}
