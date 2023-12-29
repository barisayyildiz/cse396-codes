#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <unistd.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <sys/socket.h>
#include "../header/scanner.h"
#include "../header/communication_layer.h"

int currentStepNumber;
int currentHorizontalPrecision;
int currentVerticalPrecision;
float currentTopLeftX;
float currentTopLeftY;
float currentBottomRightX;
float currentBottomRightY;
pthread_mutex_t currentScannerMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t scannerStateMutex = PTHREAD_MUTEX_INITIALIZER;
ScannerState scannerState = IDLE;

int desktopSocket;
int mobileSocket;
int clientCounter = 0;

using namespace cv;
using namespace std;

cv::Mat fourPointTransform(cv::Mat image, std::vector<cv::Point2f> pts) {
    cv::Point2f tl = pts[0];
    cv::Point2f tr = pts[1];
    cv::Point2f br = pts[2];
    cv::Point2f bl = pts[3];

    // Compute the width of the new image
    float widthA = std::sqrt(std::pow(br.x - bl.x, 2) + std::pow(br.y - bl.y, 2));
    float widthB = std::sqrt(std::pow(tr.x - tl.x, 2) + std::pow(tr.y - tl.y, 2));
    int maxWidth = static_cast<int>(std::max(widthA, widthB));

    // Compute the height of the new image
    float heightA = std::sqrt(std::pow(tr.x - br.x, 2) + std::pow(tr.y - br.y, 2));
    float heightB = std::sqrt(std::pow(tl.x - bl.x, 2) + std::pow(tl.y - bl.y, 2));
    int maxHeight = static_cast<int>(std::max(heightA, heightB));

    // Define the destination points for the perspective transform
    std::vector<cv::Point2f> dst{
        cv::Point2f(0, 0),
        cv::Point2f(maxWidth - 1, 0),
        cv::Point2f(maxWidth - 1, maxHeight - 1),
        cv::Point2f(0, maxHeight - 1)
    };

    // Compute the perspective transform matrix and apply it
    cv::Mat M = cv::getPerspectiveTransform(pts, dst);
    cv::Mat warped;
    cv::warpPerspective(image, warped, M, cv::Size(maxWidth, maxHeight));

    // Return the warped image
    return warped;
}

Vertex getVertex(Vertex pCoord) {
    double H = pCoord.x;
    double t = pCoord.y;
    double d = pCoord.z;
    double x = d * cos(t);
    double y = d * sin(t);
    double z = H;
    return Vertex((int)x, (int)y, (int)z);
}

void setConfigValues(const char* key, const char* value, Configuration& config) {
    if (strcmp(key, "horizontal_precision") == 0) {
        sscanf(value, "%d", &config.horizontal_precision);
    } else if (strcmp(key, "vertical_precision") == 0) {
        sscanf(value, "%d", &config.vertical_precision);
    } else if (strcmp(key, "top_left") == 0) {
        sscanf(value, "%f %f", &config.top_left_x, &config.top_left_y);
    } else if (strcmp(key, "bottom_right") == 0) {
        sscanf(value, "%f %f", &config.bottom_right_x, &config.bottom_right_y);
    }
}

void readConfigurationsFile(const char* fileName, Configuration& config) {
    pthread_mutex_lock(&fileMutex);

    int fd = open(fileName, O_RDONLY);
    
    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);  
    read(fd, buffer, BUFFER_SIZE);

    char* line = strtok(buffer, "\n");
    while (line) {
        // Parse each line into key and value
        char key[30];
        char value[256];
        sscanf(line, "%s %[^\n]", key, value);

        // Set the values in the Configuration structure
        setConfigValues(key, value, config);

        // Move to the next line
        line = strtok(NULL, "\n");
    }

    close(fd);

    pthread_mutex_unlock(&fileMutex);
}

void writeConfigurationsFile(const char* fileName, Configuration& config) {
    pthread_mutex_lock(&fileMutex);
    int fd = open(fileName, O_WRONLY);

    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);

    // Format the configuration into the buffer
    snprintf(buffer, BUFFER_SIZE,
             "horizontal_precision %d\n"
             "vertical_precision %d\n"
             "top_left %f %f\n"
             "bottom_right %f %f\n",
             config.horizontal_precision,
             config.vertical_precision,
             config.top_left_x, config.top_left_y,
             config.bottom_right_x, config.bottom_right_y);

    // Write the buffer to the file
    write(fd, buffer, strlen(buffer));

    close(fd);
    pthread_mutex_unlock(&fileMutex);
}

void getScannerStateStr(char buffer[BUFFER_SIZE]) {
    memset(buffer, '\0', BUFFER_SIZE);
    pthread_mutex_lock(&scannerStateMutex); 
    if(scannerState == IDLE) {
        strcat(buffer, "scanner_state IDLE");
    } else if(scannerState == RUNNING) {
        strcat(buffer, "scanner_state RUNNING");
    } else if(scannerState == CANCELLED) {
        strcat(buffer, "scanner_state CANCELLED");
    } else if(scannerState == FINISHED) {
        strcat(buffer, "scanner_state FINISHED");
    }
    // Concatenate additional information
    char tempBuffer[BUFFER_SIZE];
    snprintf(tempBuffer, BUFFER_SIZE, " current_step %d", currentStepNumber);
    strcat(buffer, tempBuffer);
    snprintf(tempBuffer, BUFFER_SIZE, " current_horizontal_precision %d", currentHorizontalPrecision);
    strcat(buffer, tempBuffer);
    snprintf(tempBuffer, BUFFER_SIZE, " current_vertical_precision %d", currentVerticalPrecision);
    strcat(buffer, tempBuffer);
    snprintf(tempBuffer, BUFFER_SIZE, " four_points %f %f %f %f", currentTopLeftX, currentTopLeftY, currentBottomRightX, currentBottomRightY);
    strcat(buffer, tempBuffer);
    pthread_mutex_unlock(&scannerStateMutex);
}

void sendImageForCalibration(int calibrationImageSocket) {
    char buffer[BUFFER_SIZE];
    
    std::string save_path = "imgs_db/original/0.jpg";
    cv::Mat img = cv::imread(save_path);
    std::vector<uchar> imageBuffer;
    imencode(".jpg", img, imageBuffer);
    int imgSize = imageBuffer.size();
    
    send(calibrationImageSocket, &imgSize, sizeof(int), 0);
    
    // send image
    int chunkSize = BUFFER_SIZE; // Choose an appropriate chunk size
    for (int i = 0; i <imgSize; i += chunkSize) {
        int remaining = std::min(chunkSize, imgSize - i);
        send(calibrationImageSocket, imageBuffer.data() + i, remaining, 0);
        recv(calibrationImageSocket, buffer, BUFFER_SIZE, 0);
    }
}

void mainScannerSend(uchar buffer[BUFFER_SIZE], int size, int desktopOnly) {
    char tmp[BUFFER_SIZE];
    for(int i=0; i<clients.size(); i++) {
        if(clients.at(i).type == DESKTOP || (clients.at(i).type == MOBILE && !desktopOnly)) {
            // std::cout << clients.at(i).liveSocket << ", " << buffer << std::endl;
            std::cout << buffer << std::endl;
            std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", send" << std::endl;
            send(clients.at(i).liveSocket, buffer, size, 0);
            std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", recv" << std::endl;
            recv(clients.at(i).liveSocket, tmp, BUFFER_SIZE, 0);
            std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", recv2" << std::endl;
            // std::cout << buffer << std::endl;
        }
    }
}

void mainScannerSend(char buffer[BUFFER_SIZE], int size, int desktopOnly) {
    char tmp[BUFFER_SIZE];
    for(int i=0; i<clients.size(); i++) {
        if(clients.at(i).type == DESKTOP || (clients.at(i).type == MOBILE && !desktopOnly)) {
            // std::cout << clients.at(i).liveSocket << ", " << buffer << std::endl;
            // std::cout << buffer << std::endl;
            std::cout << buffer << std::endl;
            std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", send" << std::endl;
            send(clients.at(i).liveSocket, buffer, strlen(buffer), 0);
            std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", recv" << std::endl;
            recv(clients.at(i).liveSocket, tmp, BUFFER_SIZE, 0);
            std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", recv2" << std::endl;
            // std::cout << buffer << std::endl;
        }
    }
}


void mainScannerSend(const char* buffer, int size, int desktopOnly) {
    for(int i=0; i<clients.size(); i++) {
        if(clients.at(i).type == DESKTOP || (clients.at(i).type == MOBILE && !desktopOnly)) {
            // Create a local non-const buffer
            char localBuffer[BUFFER_SIZE];
            std::memcpy(localBuffer, buffer, size);

            // std::cout << clients.at(i).liveSocket << ", " << localBuffer << std::endl;
            // std::cout << buffer << std::endl;
            std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", send" << std::endl;
            send(clients.at(i).liveSocket, localBuffer, size, 0);
            // TODO: burası mobilde çalışmıyor!!!!!
            std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", recv" << std::endl;
            recv(clients.at(i).liveSocket, localBuffer, BUFFER_SIZE, 0);
            std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", recv2" << std::endl;
            // std::cout << buffer << std::endl;
        }
    }
}

void mainScanner() {
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

    std::cout << "main scanner...." << std::endl;
    
    srand(time(0));
    char buffer[BUFFER_SIZE];
    bool isCancelled = false;

    // recv(clientSocket, buffer, BUFFER_SIZE, 0);

    int i = 0;
    double theta = 0;
    int counter = 0;
    int centerC = 185;

    currentStepNumber = 0;

    std::vector<std::vector<Vertex>> meshPoints;
    std::vector<int> lineLength;
    Configuration config;
    readConfigurationsFile("configurations.txt", config);
    currentHorizontalPrecision = config.horizontal_precision;
    currentVerticalPrecision = config.vertical_precision;
    currentTopLeftX = config.top_left_x;
    currentTopLeftY = config.top_left_y;
    currentBottomRightX = config.bottom_right_x;
    currentBottomRightY = config.bottom_right_y;

    memset(buffer, '\0', BUFFER_SIZE);
    sprintf(buffer, "START_SCANNING");
    mainScannerSend(buffer, BUFFER_SIZE);
    // send(clientSocket, buffer, sizeof(buffer), 0);

    while(theta < 360) {
        // check if the scanning has been cancelled
        pthread_mutex_lock(&scannerStateMutex);
        if(scannerState == CANCELLED) {
            isCancelled = true;
            pthread_mutex_unlock(&scannerStateMutex);
            break;
        }
        pthread_mutex_unlock(&scannerStateMutex);

        cv::Mat img;
        cv::Mat cropped;
        
        std::string save_path;
        
        save_path = "imgs_db/original/" + std::to_string(counter) + ".jpg";
        img = cv::imread(save_path);

        save_path = "imgs_db/original/" + std::to_string(counter) + ".jpg";
        cv::imwrite(save_path, img);
        
        cv::Point2f pts[4];
        pts[0] = {config.top_left_x, config.top_left_y};
        pts[1] = {config.bottom_right_x, config.top_left_y};
        pts[2] = {config.bottom_right_x, config.bottom_right_y};
        pts[3] = {config.top_left_x, config.bottom_right_y};

        cropped = fourPointTransform(img, std::vector<cv::Point2f>(pts, pts + 4));
        save_path = "imgs_db/four_points/" + std::to_string(counter) + ".jpg";
        cv::imwrite(save_path, cropped);

        int h = cropped.rows;
        int w = cropped.cols;
        Mat backG = Mat::zeros(h, w, CV_8U);
        int bottomR = 0;
        int topR = 0;
        std::vector<Vertex> tempV;

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
        save_path = "imgs_db/red_line/" + std::to_string(counter) + ".jpg";
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
        std::cout << "tempV size: " << tempV.size() << std::endl;
        
        int percentage = 100;
        int itemsToKeep = static_cast<int>(tempV.size() * (config.vertical_precision / 100.0));
        std::cout << "items to keep: " << itemsToKeep << std::endl;
        itemsToKeep = std::max(itemsToKeep, 1);

        double stepSize = static_cast<double>(tempV.size()) / itemsToKeep;

        
        std::cout << "stepSize: " << stepSize << std::endl;
        vector<Vertex> V;

        for (double i = 0; i < tempV.size(); i += stepSize) {
            V.push_back(tempV[static_cast<int>(i)]);
        }
        meshPoints.push_back(V);
        lineLength.push_back(-1 * V.size());
        
        
        /*
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
        }*/


        /*
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
        }*/

        std::cout << "theta: " << theta << std::endl;
        std::cout << meshPoints.back().size() << std::endl;

        if(true) {            
            memset(buffer, '\0', BUFFER_SIZE);
            sprintf(buffer, "ROUND %d %d", counter+1, config.horizontal_precision);
            std::cout << "buffer: " << buffer << std::endl;
            // send(clientSocket, buffer, sizeof(buffer), 0);
            mainScannerSend(buffer, BUFFER_SIZE);

            memset(buffer, '\0', BUFFER_SIZE);
            sprintf(buffer, "NUMBER_OF_VERTICES %d", (int)meshPoints.back().size());
            // send(clientSocket, buffer, sizeof(buffer), 0);
            mainScannerSend(buffer, BUFFER_SIZE, true);
            std::cout << "buffer: " << buffer << std::endl;
            printf("size: %s\n", buffer);
            // usleep(300000);
        }

        vector<Vertex> vertices = meshPoints.back();
        uint numOfScannedPoints = vertices.size();

        if(true) {
            for(int i=0; i<numOfScannedPoints; i++) {
                memset(buffer, '\0', BUFFER_SIZE);
                double x = vertices.at(i).x, y = vertices.at(i).y, z = vertices.at(i).z;

                sprintf(buffer, "%lf %lf %lf", x, y, z);
                mainScannerSend(buffer, BUFFER_SIZE, true);
                // send(clientSocket, buffer, sizeof(buffer), 0);
                // usleep(3);
                // recv(clientSocket, buffer, sizeof(buffer), 0);
            }
            memset(buffer, '\0', BUFFER_SIZE);
            sprintf(buffer, "IMAGES");
            // send(clientSocket, buffer, sizeof(buffer), 0);
            mainScannerSend(buffer, BUFFER_SIZE, true);

            // send image size
            std::vector<uchar> imageBuffer;
            imencode(".jpg", img, imageBuffer);
            int imgSize = imageBuffer.size();
            std::cout << "livesocket for image: " << clients.at(0).liveSocket << std::endl;
            std::cout << "first image size: " << imgSize << std::endl;
            for(int i=0; i<clients.size(); i++) {
                if(clients.at(i).type == DESKTOP) {
                    send(clients.at(i).liveSocket, &imgSize, sizeof(int), 0);
                }
            }
            
            // send image
            int chunkSize = BUFFER_SIZE; // Choose an appropriate chunk size
            for (int i = 0; i <imgSize; i += chunkSize) {
                int remaining = std::min(chunkSize, imgSize - i);
                mainScannerSend(imageBuffer.data() + i, remaining, true);
                // send(clients.at(0).liveSocket, imageBuffer.data() + i, remaining, 0);
                // recv(clients.at(0).liveSocket, buffer, BUFFER_SIZE, 0);
            }

            // send image size
            imageBuffer;
            imencode(".jpg", backG*255, imageBuffer);
            std::cout << "imageBuffer size: " << imageBuffer.size() << std::endl;
            imgSize = imageBuffer.size();
            std::cout << "livesocket for image: " << clients.at(0).liveSocket << std::endl;
            std::cout << "second image size: " << imgSize << std::endl;
            // send(clientSocket, &imgSize, sizeof(int), 0);
            for(int i=0; i<clients.size(); i++) {
                if(clients.at(i).type == DESKTOP) {
                    send(clients.at(i).liveSocket, &imgSize, sizeof(int), 0);
                }
            }
            
            // send image
            chunkSize = BUFFER_SIZE; // Choose an appropriate chunk size
            for (int i = 0; i <imgSize; i += chunkSize) {
                int remaining = std::min(chunkSize, imgSize - i);
                std::cout << "remaining: " << remaining << std::endl;
                mainScannerSend(imageBuffer.data() + i, remaining, true);
                // send(clients.at(0).liveSocket, imageBuffer.data() + i, remaining, 0);
                // recv(clients.at(0).liveSocket, buffer, BUFFER_SIZE, 0);
            }
        }
        
        theta = theta + static_cast<double>((360.0 / config.horizontal_precision));
        std::cout << "counter: " << counter << std::endl;
        counter++;
        currentStepNumber = counter;
    }

    std::cout << "outside" << std::endl;

    memset(buffer, '\0', BUFFER_SIZE);
    sprintf(buffer, "FINISH_SCANNING");
    // send(clientSocket, buffer, sizeof(buffer), 0);
    mainScannerSend(buffer, BUFFER_SIZE);

    if(!isCancelled) {
        // memset(buffer, '\0', BUFFER_SIZE);
        // sprintf(buffer, "%s", "FINISHED");
        // send(clientSocket, buffer, sizeof(buffer), 0);

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

        // Read the contents of the .obj file
        std::ifstream objFile("3d.obj");
        std::stringstream objBuffer;
        objBuffer << objFile.rdbuf();
        objFile.close();

        // Get the content as a string
        std::string objContent = objBuffer.str();

        // Send the size of the .obj content to the client
        int objSize = objContent.size();
        
        memset(buffer, '\0', BUFFER_SIZE);
        sprintf(buffer, "FILE %d", objSize);
        // send(clientSocket, buffer, sizeof(buffer), 0);
        mainScannerSend(buffer, BUFFER_SIZE);

        // Send the .obj content to the client in packages of size 1024
        int chunkSize = BUFFER_SIZE;
        for (int i = 0; i < objSize; i += chunkSize) {
            int remaining = std::min(chunkSize, objSize - i);
            mainScannerSend(objContent.c_str() + i, remaining);
            // send(clients.at(0).liveSocket, objContent.c_str() + i, remaining, 0);
            // recv(clients.at(0).liveSocket, buffer, BUFFER_SIZE, 0);
        }

        for(int i=0; i<clients.size(); i++) {
            memset(buffer, '\0', BUFFER_SIZE);
            sprintf(buffer, "FILE_END");
            send(clients.at(i).liveSocket, buffer, strlen(buffer), 0);
            recv(clients.at(i).liveSocket, buffer, BUFFER_SIZE, 0);
            // send(clients.at(i).liveSocket, &objSize, sizeof(int), 0);
        }

        pthread_mutex_lock(&scannerStateMutex);
        scannerState = FINISHED;
        pthread_mutex_unlock(&scannerStateMutex);

        broadcastMessage("scanner_state FINISHED");

        std::cout << "scanning finished inside scanner successfully\n";

    } else {
        std::cout << "scanning has canncelled\n";
    }

    // cv::waitKey(0);
}
