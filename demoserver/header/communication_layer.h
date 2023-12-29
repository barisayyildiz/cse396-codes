#ifndef COMMUNICATION_LAYER
#define COMMUNICATION_LAYER

#define SERVER_PORT 3000
#define CONFIG_PORT 3001
#define BROADCAST_PORT 3002
#define IMAGE_PORT 3003
#define LIVE_PORT 3004
// #define SCANNER_PORT 3004

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
    int scannerSocket;
    int calibrationImageSocket;
    int liveSocket;
} ClientNode;

extern int serverSocketId;
extern int configSocketId;
extern int broadcastSocketId;
extern int scannerSocketId;
extern int calibrationImageId;
extern int liveSocketId;

extern std::vector<ClientNode> clients;

int readFromClient();
std::string getIpAddress();
void handleClient(int& clientSocket);
void broadcastMessage(const char* message);

void handleClientConfigSocket(int serverSocket, int configSocket, int calibrationImageSocket);
void readFromAllClients();

#endif
