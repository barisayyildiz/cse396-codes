#ifndef COMMUNICATION_LAYER
#define COMMUNICATION_LAYER

#define SERVER_PORT 3000
#define CONFIG_PORT 3001
#define BROADCAST_PORT 3002

#define BUFFER_SIZE 1024

#include <string>
#include <vector>

extern int dataSocket;
extern int configSocket;

enum ClientType {
    DESKTOP,
    MOBILE
};

typedef struct ClientNode{
    ClientType type;
    int serverSocket;
    int configSocket;
    int broadcastSocket;
} ClientNode;

extern std::vector<ClientNode> clients;

int readFromClient();
std::string getIpAddress();
void handleClient(int& clientSocket);
void broadcastMessage(const char* message);

void handleClientConfigSocket(int serverSocket, int configSocket);
void readFromAllClients();

#endif
