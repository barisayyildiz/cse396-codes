#include "mainwindow.h"

#include "scannedpoints.h"

#include <QApplication>
#include <QtCharts>

#include <iostream>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
// for signal handling
#include <signal.h>

#define DATA_PORT 5000
#define CONFIG_PORT 4000

int dataSocket;
int configSocket;

void signalCallbackHandler(int signum) {
    std::cout << "Caught signal " << signum << std::endl;
    close(dataSocket);
    close(configSocket);
    exit(signum);
}

int sendConfig() {
    qDebug() << "Gonna send some configurations";
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSocket == -1) {
        qDebug() << "Error creating client socket";
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(CONFIG_PORT);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error connecting to the server." << std::endl;
        return 1;
    }

    // Send data to the server
    std::string dataToSend = "configurations changed...";
    if (send(clientSocket, dataToSend.c_str(), dataToSend.size(), 0) == -1) {
        std::cerr << "Error sending data." << std::endl;
        return 1;
    }

    close(clientSocket);
}


int readData(int& serverSocket, ScannedPoints* chartView) {
    qDebug() << "Start of the socket thread";
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        qDebug() << "Error creating server socket.";
        return 1;
    }
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(DATA_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the server socket to the address and port
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        qDebug() << "Error binding server socket.";
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == -1) {
        qDebug() << "Error listening for incoming connections.";
        return 1;
    }

    qDebug() << "Server is listening for incoming connections...";

    int clientSocket;
    sockaddr_in clientAddress;
    socklen_t clientAddressSize = sizeof(clientAddress);
    clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
    if (clientSocket == -1) {
        qDebug() << "Error accepting the connection.";
        return 1;
    }

    while (true) {
        // Accept incoming connections
        qDebug() << "accepting...";

        char buffer[1024]; // Buffer to store received data
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead == -1) {
            qDebug() << "Error receiving data.";
            close(clientSocket);
            break;
        }

        int numOfScannedPoints = atoi(buffer);
        qDebug() << "number of scanned points : " << numOfScannedPoints;
        chartView->addNewDataPoint(numOfScannedPoints);

        // Process the received data (echo back to the client)
        std::string receivedData(buffer, bytesRead);

        qDebug() << "Received: " << receivedData;
    }

    // Close the client socket
    close(clientSocket);

    // Close the server socket (usually never reached in this example)
    close(serverSocket);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QMainWindow mainWindow;
    mainWindow.resize(800, 800);

    ScannedPoints chartView(&mainWindow);
    QPushButton *button = new QPushButton("Add new data point");

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(&chartView);
    layout->addWidget(button);

    QWidget *widget = new QWidget();
    widget->setLayout(layout);
    mainWindow.setCentralWidget(widget);

    mainWindow.show();

    // new thread
    std::thread t1(readData, std::ref(dataSocket), &chartView);

    // Add new data points using the custom method.
    chartView.addNewDataPoint(0.4);
    chartView.addNewDataPoint(0.5);
    chartView.addNewDataPoint(0.6);

    QObject::connect(button, &QPushButton::clicked, [&]() {
        qDebug() << "clicked...";
        //chartView.addNewDataPoint(0.7);
        sendConfig();
    });

    QObject::connect(chartView.series, &QLineSeries::pointAdded, [&] (int index) {
        qDebug() << "index at " << index;
    });

    return a.exec();
}
