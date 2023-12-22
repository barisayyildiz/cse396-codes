#ifndef COMMUNICATION_LAYER
#define COMMUNICATION_LAYER

#define DATA_PORT 5000
#define CONFIG_PORT 4000
#define SERVER_PORT 3000

#define BUFFER_SIZE 1024

#include <string>

extern int dataSocket;
extern int configSocket;

int readConfig();
std::string getIpAddress();
void handleClient(int& clientSocket);

#endif
