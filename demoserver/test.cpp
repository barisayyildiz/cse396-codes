#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <unistd.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <arpa/inet.h>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
// for random number generation
#include <cstdlib>  
#include <ctime> 
// for signal handling
#include <signal.h>

#include <chrono>
#include "header/scanner.h"
#include "header/communication_layer.h"

using namespace cv;
using namespace std;

bool FEATURE_COMMUNICATION = false;

void signalCallbackHandler(int signum) {
    std::cout << "Caught signal " << signum << std::endl;
    for(int i=0; i<clients.size(); i++) {
        close(clients.at(i).serverSocket);
        close(clients.at(i).configSocket);
        close(clients.at(i).broadcastSocket);
        close(clients.at(i).scannerSocket);
        close(clients.at(i).calibrationImageSocket);
        close(clients.at(i).liveSocket);
    }
    close(serverSocketId);
    close(configSocketId);
    close(broadcastSocketId);
    close(scannerSocketId);
    close(calibrationImageId);
    close(liveSocketId);
}

int main() {
    // signal(SIGTERM, signalCallbackHandler);
    // signal(SIGINT, signalCallbackHandler);

    // initialize scanner state
    Configuration config;
    readConfigurationsFile("configurations.txt", config);
    
    pthread_mutex_lock(&currentScannerMutex);
    currentStepNumber = 0;
    currentHorizontalPrecision = config.horizontal_precision;
    currentVerticalPrecision = config.vertical_precision;
    scannerState = IDLE;
    currentTopLeftX = config.top_left_x;
    currentTopLeftY = config.top_left_y;
    currentBottomRightX = config.bottom_right_x;
    currentBottomRightY = config.bottom_right_y;
    pthread_mutex_unlock(&currentScannerMutex);

    int serverSocketId = socket(AF_INET, SOCK_STREAM, 0);
    int configSocketId = socket(AF_INET, SOCK_STREAM, 0);
    int broadcastSocketId = socket(AF_INET, SOCK_STREAM, 0);
    int scannerSocketId = socket(AF_INET, SOCK_STREAM, 0);
    int calibrationImageId = socket(AF_INET, SOCK_STREAM, 0);
    int liveSocketId = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketId == -1 || configSocketId == -1 || broadcastSocketId == -1 || scannerSocketId == -1 || calibrationImageId == -1 || liveSocketId == -1) {
        std::cerr << "Error creating server socket." << std::endl;
        return 1;
    }

    // Define the server address and port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    sockaddr_in configAddress;
    configAddress.sin_family = AF_INET;
    configAddress.sin_port = htons(CONFIG_PORT);
    configAddress.sin_addr.s_addr = INADDR_ANY;

    sockaddr_in broadcastAddress;
    broadcastAddress.sin_family = AF_INET;
    broadcastAddress.sin_port = htons(BROADCAST_PORT);
    broadcastAddress.sin_addr.s_addr = INADDR_ANY;

    sockaddr_in calibrationImageAddress;
    calibrationImageAddress.sin_family = AF_INET;
    calibrationImageAddress.sin_port = htons(IMAGE_PORT);
    calibrationImageAddress.sin_addr.s_addr = INADDR_ANY;

    sockaddr_in liveAddress;
    liveAddress.sin_family = AF_INET;
    liveAddress.sin_port = htons(LIVE_PORT);
    liveAddress.sin_addr.s_addr = INADDR_ANY;


    // sockaddr_in scannerAddress;
    // scannerAddress.sin_family = AF_INET;
    // scannerAddress.sin_port = htons(SCANNER_PORT);
    // scannerAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the server socket to the address and port
    if (bind(serverSocketId, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error binding server socket." << std::endl;
        return 1;
    }
    if (bind(configSocketId, (struct sockaddr*)&configAddress, sizeof(configAddress)) == -1) {
        std::cerr << "Error binding config socket." << std::endl;
        return 1;
    }
    if (bind(broadcastSocketId, (struct sockaddr*)&broadcastAddress, sizeof(broadcastAddress)) == -1) {
        std::cerr << "Error binding broadcast socket." << std::endl;
        return 1;
    }
    if (bind(calibrationImageId, (struct sockaddr*)&calibrationImageAddress, sizeof(calibrationImageAddress)) == -1) {
        std::cerr << "Error binding calibration image socket." << std::endl;
        return 1;
    }
    if (bind(liveSocketId, (struct sockaddr*)&liveAddress, sizeof(liveAddress)) == -1) {
        std::cerr << "Error binding live socket." << std::endl;
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocketId, SOMAXCONN) == -1) {
        std::cerr << "Error listening for incoming connections." << std::endl;
        return 1;
    }
    if (listen(configSocketId, SOMAXCONN) == -1) {
        std::cerr << "Error listening for incoming connections." << std::endl;
        return 1;
    }
    if (listen(broadcastSocketId, SOMAXCONN) == -1) {
        std::cerr << "Error listening for incoming connections." << std::endl;
        return 1;
    }
    if (listen(calibrationImageId, SOMAXCONN) == -1) {
        std::cerr << "Error listening for incoming connections." << std::endl;
        return 1;
    }
    if (listen(liveSocketId, SOMAXCONN) == -1) {
        std::cerr << "Error listening for incoming connections." << std::endl;
        return 1;
    }

    char buffer[BUFFER_SIZE];

    std::cout << "Server is listening for incoming connections..." << std::endl;

    // std::thread tClient(readFromAllClients);
    // tClient.detach();

    ClientType type;

    while (true) {
        // Accept incoming connections
        int serverClientSocket;
        sockaddr_in serverClientAddress;
        socklen_t serverClientAddressSize = sizeof(serverClientAddress);

        int configClientSocket;
        sockaddr_in configClientAddress;
        socklen_t configClientAddressSize = sizeof(configClientAddress);

        int broadcastClientSocket;
        sockaddr_in broadcastClientAddress;
        socklen_t broadcastClientAddressSize = sizeof(broadcastClientAddress);

        int calibrationImageClientSocket;
        sockaddr_in calibrationImageClientAddress;
        socklen_t calibrationImageClientAddressSize = sizeof(calibrationImageClientAddress);

        int liveClientSocket;
        sockaddr_in liveClientAddress;
        socklen_t liveClientAddressSize = sizeof(liveClientAddress);

        serverClientSocket = accept(serverSocketId, (struct sockaddr*)&serverClientAddress, &serverClientAddressSize);
        if (serverClientSocket == -1) {
            std::cerr << "Error accepting the connection." << std::endl;
            return 1;
        }

        configClientSocket = accept(configSocketId, (struct sockaddr*)&configClientAddress, &configClientAddressSize);
        if (configClientSocket == -1) {
            std::cerr << "Error accepting the connection." << std::endl;
            return 1;
        }

        broadcastClientSocket = accept(broadcastSocketId, (struct sockaddr*)&broadcastClientAddress, &broadcastClientAddressSize);
        if (broadcastClientSocket == -1) {
            std::cerr << "Error accepting the connection." << std::endl;
            return 1;
        }

        calibrationImageClientSocket = accept(calibrationImageId, (struct sockaddr*)&calibrationImageClientAddress, &calibrationImageClientAddressSize);
        if (calibrationImageClientSocket == -1) {
            std::cerr << "Error accepting the connection." << std::endl;
            return 1;
        }

        liveClientSocket = accept(liveSocketId, (struct sockaddr*)&liveAddress, &liveClientAddressSize);
        if (liveClientSocket == -1) {
            std::cerr << "Error accepting the connection." << std::endl;
            return 1;
        }

        memset(buffer, '\0', BUFFER_SIZE);
        recv(serverClientSocket, buffer, BUFFER_SIZE, 0);
        if(strcmp(buffer, "desktop") == 0) {
            type = DESKTOP;
            std::cerr << "desktop" << std::endl;
        }else if(strcmp(buffer, "mobile") == 0) {
            type = MOBILE;
            std::cerr << "mobile" << std::endl;
        }
        ClientNode client;
        client.type = type;
        client.serverSocket = serverClientSocket;
        client.configSocket = configClientSocket;
        client.broadcastSocket = broadcastClientSocket;
        client.calibrationImageSocket = calibrationImageClientSocket;
        client.liveSocket = liveClientSocket;
        clients.push_back(client);

        // TODO: send scanner info to newly connected client
        memset(buffer, '\0', BUFFER_SIZE);
        getScannerStateStr(buffer);
        send(serverClientSocket, buffer, BUFFER_SIZE, 0);
        std::cout << buffer << std::endl;

        std::thread t2(handleClientConfigSocket, std::ref(client.serverSocket), std::ref(client.configSocket), std::ref(calibrationImageClientSocket));
        t2.detach();
    }
    
    close(serverSocketId);
    close(configSocketId);
    close(broadcastSocketId);
}


// int main() {
//     // handle ctrl+c signal

//     // std::string ipAddress = getIpAddress();

//     // // Create an image with white background
//     // Mat windowImage(300, 400, CV_8UC3, Scalar(0,0,0));

//     // // Display the IP address on the image
//     // stringstream text;
//     // text << "Raspberry Pi IP Address: " << ipAddress;
    
//     // Size textSize = getTextSize(text.str(), FONT_HERSHEY_SIMPLEX, 0.5, 1, 0);
//     // Point textPosition((windowImage.cols - textSize.width) / 2, (windowImage.rows + textSize.height) / 2);
//     // putText(windowImage, text.str(), textPosition, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255, 1), 1);

//     // // Show the window with the IP address
//     // namedWindow("Raspberry Pi IP Address", WINDOW_NORMAL);
//     // resizeWindow("Raspberry Pi IP Address", 400, 300);
//     // imshow("Raspberry Pi IP Address", windowImage);
//     // cv::waitKey(0);

//     int i = 0;
//     double theta = 0;
//     int counter = 0;
//     int centerC = 185;

//     vector<vector<Vertex>> meshPoints;
//     vector<int> lineLength;

//     while(theta < 360) {
//         cv::Mat img;
//         cv::Mat cropped;
        
//         std::string save_path;
        
//         save_path = "imgs_db/original/" + std::to_string(counter) + ".jpg";
//         img = cv::imread(save_path);

//         save_path = "imgs_db/original/" + std::to_string(counter) + ".jpg";
//         cv::imwrite(save_path, img);

//         Point2f pts[4];
//         pts[0] = { 364.0, 140.0 };
//         pts[1] = { 704.0, 140.0 };
//         pts[2] = { 704.0, 659.0 };
//         pts[3] = { 364.0, 659.0 };

//         cropped = fourPointTransform(img, std::vector<cv::Point2f>(pts, pts + 4));
//         save_path = "imgs_db/four_points/" + std::to_string(counter) + ".jpg";
//         cv::imwrite(save_path, cropped);

//         int h = cropped.rows;
//         int w = cropped.cols;
//         Mat backG = Mat::zeros(h, w, CV_8U);
//         int bottomR = 0;
//         int topR = 0;
//         vector<Vertex> tempV;

//         cv::Mat modified = backG.clone();

//         for(int i=0; i<h; i++) {
//             int max = -1;
//             int cIndex = -1;
//             for(int j=0; j<centerC; j++) {
//                 int current = cropped.at<cv::Vec3b>(i, j)[2];
//                 if(current > max) {
//                     cIndex = j;
//                     max = current;
//                 }
//             }
//             if(cropped.at<cv::Vec3b>(i, cIndex)[2] > 25) {
//                 backG.at<uchar>(i, cIndex) = 1;
//                 bottomR = i;
//                 if(topR == 0) {
//                     topR = i;
//                 }
//             }
//         }
//         save_path = "imgs_db/red_line/" + std::to_string(counter) + ".jpg";
//         cv::imwrite(save_path, backG*255);

//         for (int r = 0; r < h; r++) {
//             int cIndex = 0;
//             for (int c = 0; c < w; c++) {
//                 if (backG.at<uchar>(r, c) == 1) {
//                     double H = r - bottomR;
//                     double dist = c - centerC;
//                     double t = theta * (M_PI / 180.0); // Convert degrees to radians
//                     Vertex coord(H, t, dist);
//                     tempV.push_back(getVertex(coord));
//                 }
//             }
//         }

//         int percentage = 100;
//         int itemsToKeep = static_cast<int>(tempV.size() * (percentage / 100.0));
//         std::cout << "items to keep: " << itemsToKeep << std::endl;
//         itemsToKeep = std::max(itemsToKeep, 1);

//         double stepSize = static_cast<double>(tempV.size()) / itemsToKeep;

        
//         std::cout << "stepSize: " << stepSize << std::endl;
//         vector<Vertex> V;

//         for (double i = 0; i < tempV.size(); i += stepSize) {
//             V.push_back(tempV[static_cast<int>(i)]);
//         }
//         meshPoints.push_back(V);
//         lineLength.push_back(-1 * V.size());

//         std::cout << "theta: " << theta << std::endl;
//         std::cout << meshPoints.back().size() << std::endl;

//         vector<Vertex> vertices = meshPoints.back();
//         uint numOfScannedPoints = vertices.size();
        
//         theta = theta + static_cast<double>((360.0 / (2048 / STEP_PER_MOVEMENT)));
//         counter++;
//     }

//     int shortest = meshPoints[distance(lineLength.begin(), max_element(lineLength.begin(), lineLength.end()))].size();

//     for (vector<vector<Vertex>>::iterator it = meshPoints.begin(); it != meshPoints.end(); ++it) {
//         while (it->size() > shortest) {
//             it->erase(it->end() - 2);
//         }
//     }

//     vector<Vertex> points;
//     vector<Face> faces;
//     vector<int> firstRow;
//     vector<int> prevRow;
//     vector<int> lastVertices;

//     for (int index = 0; index < meshPoints[0].size(); ++index) {
//         points.push_back(meshPoints[0][index]);
//         firstRow.push_back(index + 1);
//     }

//     prevRow = firstRow;

//     for (int col = 0; col < meshPoints.size(); ++col) {
//         if (col != 0) {
//             int indexS = prevRow.back();
//             vector<int> currentRow;

//             for (int point = 0; point < meshPoints[col].size() - 1; ++point) {
//                 int tl = indexS + point + 1;
//                 int bl = tl + 1;
//                 int tr = prevRow[point];
//                 int br = prevRow[point + 1];

//                 Face f1(tl, tr, bl);
//                 Face f2(bl, tr, br);
//                 faces.push_back(f1);
//                 faces.push_back(f2);

//                 points.push_back(meshPoints[col][point]);
//                 currentRow.push_back(tl);

//                 if (point == meshPoints[col].size() - 2) {
//                     points.push_back(meshPoints[col][point + 1]);
//                     currentRow.push_back(bl);
//                 }

//                 if (col == meshPoints.size() - 1) {
//                     tr = tl;
//                     br = bl;
//                     tl = firstRow[point];
//                     bl = firstRow[point + 1];

//                     Face f3(tl, tr, bl);
//                     Face f4(bl, tr, br);
//                     faces.push_back(f3);
//                     faces.push_back(f4);
//                 }
//             }
//             lastVertices.push_back(prevRow.back());
//             prevRow = currentRow;
//         }
//     }
    
//     for(int i=0; i<lastVertices.size()-1; i++) {
//         faces.push_back(Face(lastVertices.at(0), lastVertices.at(i), lastVertices.at(i+1)));
//     }

//     double minValues[3] = {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
//     double maxValues[3] = {-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};

//     // Calculate min and max values
//     for (const Vertex& point : points) {
//         double x = point.x, y = point.y, z = point.z;
//         for (int i = 0; i < 3; ++i) {
//             minValues[i] = std::min(minValues[i], std::min({x, y, z}));
//             maxValues[i] = std::max(maxValues[i], std::max({x, y, z}));
//         }
//     }

//     // Normalize vertices
//     double ranges[3] = {maxValues[0] - minValues[0], maxValues[1] - minValues[1], maxValues[2] - minValues[2]};
//     for(int i=0; i<points.size(); i++) {
//         points.at(i).x = (points.at(i).x - minValues[0]) / ranges[0];
//         points.at(i).y  = (points.at(i).y - minValues[1]) / ranges[1];
//         points.at(i).z  = (points.at(i).z - minValues[2]) / ranges[2];
//     }

//     // create .obj file
//     std::string fileToWrite = "3d.obj";
//     std::ofstream file(fileToWrite);

//     if (file.is_open()) {
//         for (Vertex& point : points) {
//             file << point;
//         }
//         for (Face& face : faces) {
//             file << face;
//         }
//         file.close();
//     } else {
//         std::cerr << "Error: Unable to open file for writing." << std::endl;
//     }

//     // cv::waitKey(0);

//     return 0;
// }
