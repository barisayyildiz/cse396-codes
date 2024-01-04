#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
// #include <wiringPi.h>
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

#include <wiringPi.h>
#include <chrono>
#include "header/scanner.h"
#include "header/communication_layer.h"

using namespace cv;
using namespace std;

bool FEATURE_COMMUNICATION = false;

void signalCallbackHandler(int signum) {
    std::cout << "Caught signal " << signum << std::endl;
    close(dataSocket);
    close(configSocket);
    exit(signum);
}

int main() {
    // handle ctrl+c signal

    // std::string ipAddress = getIpAddress();

    // // Create an image with white background
    // Mat windowImage(300, 400, CV_8UC3, Scalar(0,0,0));

    // // Display the IP address on the image
    // stringstream text;
    // text << "Raspberry Pi IP Address: " << ipAddress;
    
    // Size textSize = getTextSize(text.str(), FONT_HERSHEY_SIMPLEX, 0.5, 1, 0);
    // Point textPosition((windowImage.cols - textSize.width) / 2, (windowImage.rows + textSize.height) / 2);
    // putText(windowImage, text.str(), textPosition, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255, 1), 1);

    // // Show the window with the IP address
    // namedWindow("Raspberry Pi IP Address", WINDOW_NORMAL);
    // resizeWindow("Raspberry Pi IP Address", 400, 300);
    // imshow("Raspberry Pi IP Address", windowImage);
    // cv::waitKey(0);

    if(FEATURE_COMMUNICATION) {
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

        memset(buffer, '\0', BUFFER_SIZE);
        sprintf(buffer, "RUNNING");
        send(dataSocket, buffer, sizeof(buffer), 0);
    }

    wiringPiSetupGpio();

    pinMode(STEPPER_PIN_1, OUTPUT);
    pinMode(STEPPER_PIN_2, OUTPUT);
    pinMode(STEPPER_PIN_3, OUTPUT);
    pinMode(STEPPER_PIN_4, OUTPUT);

    int i = 0;
    double theta = 0;
    int counter = 0;
    int centerC = 175;

    vector<vector<Vertex>> meshPoints;
    vector<int> lineLength;

    while(theta <= 360) {
        // cv::VideoCapture cap(0);
        // if (!cap.isOpened()) {
        //     cout << "Error: Camera not found" << endl;
        //     return 1;
        // }
        // cv::Mat img;
        // cap >> img;
        // cap.release();
        cv::Mat img;
        cv::Mat cropped;
        
        std::string save_path;
        
        save_path = "imgs_db/original/" + std::to_string(counter) + ".jpg";
        // img = cv::imread(save_path);

        char filename[] = "output.jpg";
        takePic(filename);
        img = cv::imread(filename);
        // sleep(1);

        save_path = "imgs/original/" + std::to_string(counter) + ".jpg";
        cv::imwrite(save_path, img);

        Point2f pts[4];
        pts[0] = { 340.0, 244.0 };
        pts[1] = { 611.0, 244.0 };
        pts[2] = { 611.0, 747.0 };
        pts[3] = { 340.0, 747.0 };

        cropped = fourPointTransform(img, std::vector<cv::Point2f>(pts, pts + 4));
        save_path = "imgs/four_points/" + std::to_string(counter) + ".jpg";
        cv::imwrite(save_path, cropped);

        int h = cropped.rows;
        int w = cropped.cols;
        Mat backG = Mat::zeros(h, w, CV_8U);
        int bottomR = 0;
        int topR = 0;
        vector<Vertex> tempV;

        cv::Mat modified = backG.clone();

        for(int i=0; i<h; i++) {
            int max = -1;
            int cIndex = -1;
            for(int j=0; j<w; j++) {
                int current = cropped.at<cv::Vec3b>(i, j)[2];
                if(current > max) {
                    cIndex = j;
                    max = current;
                }
            }
            if(cropped.at<cv::Vec3b>(i, cIndex)[2] > 25) {
                backG.at<uchar>(i, cIndex) = 1;
                bottomR = i;
                if(topR == 0) {
                    topR = i;
                }
            }
        }
        save_path = "imgs/red_line/" + std::to_string(counter) + ".jpg";
        cv::imwrite(save_path, backG*255);

        for (int r = 0; r < h; r++) {
            int cIndex = 0;
            for (int c = 0; c < w; c++) {
                if (backG.at<uchar>(r, c) == 1) {
                    double H = r - bottomR;
                    double dist = c - centerC;
                    double t = theta * (M_PI / 180.0); // Convert degrees to radians
                    Vertex coord(H, t, dist);
                    tempV.push_back(getVertex(coord));
                }
            }
        }

        int intv = 550; // Vertical resolution
        intv = tempV.size() / intv;

        if (!tempV.empty()) {
            vector<Vertex> V;
            V.push_back(tempV[0]);

            for (int ind = 1; ind < tempV.size() - 2; ind++) {
                if(intv == 0) {
                    V.push_back(tempV[ind]);
                } else if (ind % intv == 0) {
                    V.push_back(tempV[ind]);
                }
            }

            V.push_back(tempV[tempV.size() - 1]);
            meshPoints.push_back(V);
            lineLength.push_back(-1 * V.size());
        }

        std::cout << "theta: " << theta << std::endl;
        std::cout << meshPoints.back().size() << std::endl;

        if(FEATURE_COMMUNICATION) {
            // read the message from desktop for synchronization
            memset(buffer, '\0', BUFFER_SIZE);
            recv(dataSocket, buffer, sizeof(buffer), 0);
            
            memset(buffer, '\0', BUFFER_SIZE);
            sprintf(buffer, "%d %d", counter+1, 2048 / STEP_PER_MOVEMENT);
            send(dataSocket, buffer, sizeof(buffer), 0);

            memset(buffer, '\0', BUFFER_SIZE);
            sprintf(buffer, "%d", (int)meshPoints.back().size());
            send(dataSocket, buffer, sizeof(buffer), 0);
            printf("size: %s\n", buffer);
            usleep(300000);
        }

        vector<Vertex> vertices = meshPoints.back();
        uint numOfScannedPoints = vertices.size();

        if(FEATURE_COMMUNICATION) {
            for(int i=0; i<numOfScannedPoints; i++) {
                memset(buffer, '\0', BUFFER_SIZE);
                double x = vertices.at(i).x, y = vertices.at(i).y, z = vertices.at(i).z;

                 sprintf(buffer, "%lf %lf %lf", x, y, z);
                send(dataSocket, buffer, sizeof(buffer), 0);
                recv(dataSocket, buffer, sizeof(buffer), 0);
            }

            // send image size
            std::vector<uchar> imageBuffer;
            imencode(".jpg", img, imageBuffer);
            int imgSize = imageBuffer.size();
            send(dataSocket, &imgSize, sizeof(int), 0);
            
            // send image
            int chunkSize = 1024; // Choose an appropriate chunk size
            for (int i = 0; i <imgSize; i += chunkSize) {
                int remaining = std::min(chunkSize, imgSize - i);
                send(dataSocket, imageBuffer.data() + i, remaining, 0);
                recv(dataSocket, buffer, BUFFER_SIZE, 0);
            }

            // send image size
            imageBuffer;
            imencode(".jpg", backG*255, imageBuffer);
            imgSize = imageBuffer.size();
            send(dataSocket, &imgSize, sizeof(int), 0);
            
            // send image
            chunkSize = 1024; // Choose an appropriate chunk size
            for (int i = 0; i <imgSize; i += chunkSize) {
                int remaining = std::min(chunkSize, imgSize - i);
                send(dataSocket, imageBuffer.data() + i, remaining, 0);
                recv(dataSocket, buffer, BUFFER_SIZE, 0);
            }
        }

        move(STEP_PER_MOVEMENT);
        
        theta = theta + static_cast<double>((360.0 / (2048 / STEP_PER_MOVEMENT)));
        counter++;
    }

    memset(buffer, '\0', BUFFER_SIZE);
    sprintf(buffer, "%s", "FINISHED");
    send(dataSocket, buffer, sizeof(buffer), 0);

    int shortest = meshPoints[distance(lineLength.begin(), max_element(lineLength.begin(), lineLength.end()))].size();

    for (vector<vector<Vertex>>::iterator it = meshPoints.begin(); it != meshPoints.end(); ++it) {
        while (it->size() > shortest) {
            it->erase(it->end() - 2);
        }
    }

    vector<Vertex> points;
    vector<Face> faces;
    vector<int> firstRow;
    vector<int> prevRow;
    vector<int> lastVertices;

    for (int index = 0; index < meshPoints[0].size(); ++index) {
        points.push_back(meshPoints[0][index]);
        firstRow.push_back(index + 1);
    }

    prevRow = firstRow;

    for (int col = 0; col < meshPoints.size(); ++col) {
        if (col != 0) {
            int indexS = prevRow.back();
            vector<int> currentRow;

            for (int point = 0; point < meshPoints[col].size() - 1; ++point) {
                int tl = indexS + point + 1;
                int bl = tl + 1;
                int tr = prevRow[point];
                int br = prevRow[point + 1];

                Face f1(tl, tr, bl);
                Face f2(bl, tr, br);
                faces.push_back(f1);
                faces.push_back(f2);

                points.push_back(meshPoints[col][point]);
                currentRow.push_back(tl);

                if (point == meshPoints[col].size() - 2) {
                    points.push_back(meshPoints[col][point + 1]);
                    currentRow.push_back(bl);
                }

                if (col == meshPoints.size() - 1) {
                    tr = tl;
                    br = bl;
                    tl = firstRow[point];
                    bl = firstRow[point + 1];

                    Face f3(tl, tr, bl);
                    Face f4(bl, tr, br);
                    faces.push_back(f3);
                    faces.push_back(f4);
                }
            }
            lastVertices.push_back(prevRow.back());
            prevRow = currentRow;
        }
    }
    
    for(int i=0; i<lastVertices.size()-1; i++) {
        faces.push_back(Face(lastVertices.at(0), lastVertices.at(i), lastVertices.at(i+1)));
    }

    double minValues[3] = {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
    double maxValues[3] = {-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};

    // Calculate min and max values
    for (const Vertex& point : points) {
        double x = point.x, y = point.y, z = point.z;
        for (int i = 0; i < 3; ++i) {
            minValues[i] = std::min(minValues[i], std::min({x, y, z}));
            maxValues[i] = std::max(maxValues[i], std::max({x, y, z}));
        }
    }

    // Normalize vertices
    double ranges[3] = {maxValues[0] - minValues[0], maxValues[1] - minValues[1], maxValues[2] - minValues[2]};
    for(int i=0; i<points.size(); i++) {
        points.at(i).x = (points.at(i).x - minValues[0]) / ranges[0];
        points.at(i).y  = (points.at(i).y - minValues[1]) / ranges[1];
        points.at(i).z  = (points.at(i).z - minValues[2]) / ranges[2];
    }

    // create .obj file
    std::string fileToWrite = "3d.obj";
    std::ofstream file(fileToWrite);

    if (file.is_open()) {
        for (Vertex& point : points) {
            file << point;
        }
        for (Face& face : faces) {
            file << face;
        }
        file.close();
    } else {
        std::cerr << "Error: Unable to open file for writing." << std::endl;
    }

    // cv::waitKey(0);

    return 0;
}
