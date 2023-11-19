#include "mainwindow.h"

#include "scannedpoints.h"
#include "pointcloud.h"
#include "tabdialog.h"

#include <QApplication>
#include <QtCharts>
#include <QScrollArea>

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
#include <cstdlib>

#define DATA_PORT 5000
#define CONFIG_PORT 4000
#define BUFFER_SIZE 1024

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
    configSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(configSocket == -1) {
        qDebug() << "Error creating client socket";
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(CONFIG_PORT);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(configSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error connecting to the server." << std::endl;
        return 1;
    }

    // Send data to the server
    std::string dataToSend = "configurations changed...";
    if (send(configSocket, dataToSend.c_str(), dataToSend.size(), 0) == -1) {
        std::cerr << "Error sending data." << std::endl;
        return 1;
    }

    close(configSocket);
}


int readData(int& serverSocket, PointCloud* pointCloud) {
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

        char buffer[BUFFER_SIZE]; // Buffer to store received data
        memset(buffer, '\0', BUFFER_SIZE);
        sprintf(buffer, "%s", "OK");
        send(clientSocket, buffer, sizeof(buffer), 0);

        memset(buffer, '\0', BUFFER_SIZE);
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead == -1) {
            qDebug() << "Error receiving data.";
            close(clientSocket);
            break;
        }

        if(strcmp(buffer, "FINISHED") == 0) {
            close(clientSocket);
            break;
        }

        char* token;
        token = strtok(buffer, " ");

        qDebug() << "token: " << token;
        int round = atoi(token);
        qDebug() << "round number: " << round;

        memset(buffer, '\0', BUFFER_SIZE);
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead == -1) {
            qDebug() << "Error receiving data.";
            close(clientSocket);
            break;
        }

        int numOfScannedPoints = atoi(buffer);
        qDebug() << "number of scanned points : " << numOfScannedPoints;

        for(int i=0; i<numOfScannedPoints; i++) {
            memset(buffer, '\0', BUFFER_SIZE);
            recv(clientSocket, buffer, sizeof(buffer), 0);
            send(clientSocket, buffer, sizeof(buffer), 0);
            //qDebug() << "buffer: " << buffer;
            int x, y, z;
            sscanf(buffer, "%d %d %d", &x, &y, &z);
            pointCloud->addNewDataPoint((double)x, (double)y, (double)z);
        }

        if(round%5 == 0) {
            pointCloud->reRenderGraph();
        }
        qDebug() << "----------------";
        //chartView->addNewDataPoint(numOfScannedPoints);
    }
    pointCloud->reRenderGraph();
    qDebug() << "end of readdata thread" ;

    // Close the client socket
    close(clientSocket);

    // Close the server socket (usually never reached in this example)
    close(serverSocket);
}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QMainWindow mainWindow;

    QWidget widget;
    widget.setMinimumSize(1800, 1000);  // Set a fixed size for the widget

    QVBoxLayout layout;
    widget.setLayout(&layout);

    /*
    PointCloud pointCloud;

    QWidget *container = QWidget::createWindowContainer(&pointCloud);
    container->setMinimumSize(400, 400);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    for(int i=0; i<100; i++) {
        pointCloud.addNewDataPoint(rand()%100, rand()%100, rand()%100);
    }
    layout.addWidget(container);
    */


    // Create a QLabel for the header
    QLabel *headerLabel = new QLabel("Live Charts");
    headerLabel->setFont(QFont("Arial", 16, QFont::Bold));  // Set font and size for the header text
    headerLabel->setAlignment(Qt::AlignCenter);  // Center-align the text

    PointCloud pointCloud;
    //pointCloud.show();

    /*ScannedPoints chartView(&mainWindow);
    chartView.addNewDataPoint(0.4);
    chartView.addNewDataPoint(0.5);
    chartView.addNewDataPoint(20);
    chartView.setMinimumHeight(500);
    chartView.setMinimumWidth(500);
*/

    QWidget *container = QWidget::createWindowContainer(&pointCloud);
    container->setMinimumSize(400, 400);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    /*for(int i=0; i<50000; i++) {
        pointCloud.addNewDataPoint(rand()%100, rand()%100, rand()%100);
    }
    pointCloud.reRenderGraph();*/
    QPushButton *button = new QPushButton("Add new data point");

    //layout.addWidget(headerLabel);
    layout.addWidget(container);
    //layout.addWidget(&chartView);
    //layout.addWidget(button);

    mainWindow.setCentralWidget(&widget);

    //mainWindow.setCentralWidget(&widget);
    mainWindow.show();

    /*mainWindow.setCentralWidget(&widget);

    //mainWindow.setCentralWidget(&widget);
    mainWindow.show();
    */

    // new thread
    std::thread t1(readData, std::ref(dataSocket), &pointCloud);

    QObject::connect(button, &QPushButton::clicked, [&]() {
        qDebug() << "clicked...";
        //chartView.addNewDataPoint(0.7);
        sendConfig();
    });

    /*QObject::connect(chartView.series, &QLineSeries::pointAdded, [&] (int index) {
        qDebug() << "index at " << index;
    });
*/



    return a.exec();
}
