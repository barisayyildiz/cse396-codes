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
#include <wiringPi.h>

int prevButtonState = LOW;
int stepNumber = 0;

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

pthread_mutex_t cameraMutex = PTHREAD_MUTEX_INITIALIZER;

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

void oneStep(){
    // std::cout << "counter : " << ++counter << std::endl;
    switch(stepNumber){
      case 0:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, HIGH);
      break;
      case 1:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, HIGH);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
      case 2:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, HIGH);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
      case 3:
      digitalWrite(STEPPER_PIN_1, HIGH);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
    }
    stepNumber++;
    if(stepNumber > 3){
        stepNumber = 0;
    }
}

void move(int stepPrecision){
    int precisionCounter = 0;
    while(precisionCounter < stepPrecision){
        oneStep();
        precisionCounter++;
        delay(DELAY_ONE_STEP);
    }
}

void takePic (char* filename)
{
    int pid, status;
    if((pid = fork()) == 0)
    {
        execl("/usr/bin/raspistill", "raspistill", "-t", "200", "-w", "1024", "-h", "768", "-o", filename, (char *)NULL);
    }
    waitpid(pid, &status, 0);
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
    char filename[] = "calibration.jpg";
    cv::Mat img;

    pthread_mutex_lock(&cameraMutex);
    takePic(filename);
    img = cv::imread(filename);
    pthread_mutex_unlock(&cameraMutex);

    // img = cv::imread("imgs_db/original/0.jpg");

    // std::string save_path = "imgs_db/original/0.jpg";
    // cv::Mat img = cv::imread(save_path);
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
            // std::cout << buffer << std::endl;
            // std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", send" << std::endl;
            send(clients.at(i).liveSocket, buffer, size, 0);
            // std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", recv" << std::endl;
            recv(clients.at(i).liveSocket, tmp, 3, 0);
            // std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", recv2" << std::endl;
            // std::cout << buffer << std::endl;
        }
    }
}

void mainScannerSend(int *objSize) {
    char tmp[BUFFER_SIZE];
    for(int i=0; i<clients.size(); i++) {
        if(clients.at(i).type == DESKTOP) {
            // std::cout << clients.at(i).liveSocket << ", " << buffer << std::endl;
            // std::cout << buffer << std::endl;
            // std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", send" << std::endl;7
            send(clients.at(i).liveSocket, objSize, sizeof(int), 0);
            // send(clients.at(i).liveSocket, buffer, size, 0);
            // std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", recv" << std::endl;
            recv(clients.at(i).liveSocket, tmp, 3, 0);
            // std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", recv2" << std::endl;
            // std::cout << buffer << std::endl;
        }
    }
}

void mainScannerSend(const std::vector<char>& buffer, int desktopOnly) {
    for (const auto& client : clients) {
        if (client.type == DESKTOP || (client.type == MOBILE && !desktopOnly)) {
            // std::cout << "Buffer: ";
            for (char ch : buffer) {
                // std::cout << ch;
            }
            // std::cout << std::endl;

            if (client.type == MOBILE) {
                const char* charArray = buffer.data();
                send(client.liveSocket, charArray, strlen(charArray), 0);
            } else {
                ssize_t sentBytes = send(client.liveSocket, buffer.data(), buffer.size(), 0);
                // std::cout << "sent_bytes: " << sentBytes << std::endl;
            }
            // std::cout << "sent" << std::endl;

            // Receive acknowledgment
            std::vector<char> acknowledgment(3, '\0');
            ssize_t receivedBytes = recv(client.liveSocket, acknowledgment.data(), 3, 0);
            // std::cerr << "receivedbytes: " << receivedBytes << std::endl;

            // while (receivedBytes != 3) {
            //     receivedBytes = recv(client.liveSocket, acknowledgment.data(), 3, 0);
            //     std::cerr << "receivedbytes: " << receivedBytes << std::endl;
            // }

            // Process acknowledgment if needed
        }
    }
}

void mainScannerSend(const std::vector<char>& buffer, ClientType clientType) {
    for (const auto& client : clients) {
        if (client.type == clientType) {
            // std::cout << "Buffer: ";
            for (char ch : buffer) {
                // std::cout << ch;
            }
            // std::cout << std::endl;

            if (client.type == MOBILE) {
                const char* charArray = buffer.data();
                send(client.liveSocket, charArray, strlen(charArray), 0);
            } else {
                ssize_t sentBytes = send(client.liveSocket, buffer.data(), buffer.size(), 0);
                // std::cout << "sent_bytes: " << sentBytes << std::endl;
            }
            // std::cout << "sent" << std::endl;

            // Receive acknowledgment
            std::vector<char> acknowledgment(3, '\0');
            ssize_t receivedBytes = recv(client.liveSocket, acknowledgment.data(), 3, 0);
            // std::cerr << "receivedbytes: " << receivedBytes << std::endl;

            // while (receivedBytes != 3) {
            //     receivedBytes = recv(client.liveSocket, acknowledgment.data(), 3, 0);
            //     std::cerr << "receivedbytes: " << receivedBytes << std::endl;
            // }

            // Process acknowledgment if needed
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
            // std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", send" << std::endl;
            send(clients.at(i).liveSocket, localBuffer, size, 0);
            // TODO: burası mobilde çalışmıyor!!!!!
            // std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", recv" << std::endl;
            recv(clients.at(i).liveSocket, localBuffer, 3, 0);
            // std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", recv2" << std::endl;
            // std::cout << buffer << std::endl;
        }
    }
}

void mainScannerSend(const char* buffer, int size, ClientType clientType) {
    // std::cout << "mainScannersend mobile\n";
    for(int i=0; i<clients.size(); i++) {
        if(clients.at(i).type == clientType) {
            // Create a local non-const buffer
            char localBuffer[BUFFER_SIZE];
            std::memcpy(localBuffer, buffer, size);

            // std::cout << clients.at(i).liveSocket << ", " << localBuffer << std::endl;
            // std::cout << buffer << std::endl;
            // std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", send" << std::endl;
            send(clients.at(i).liveSocket, localBuffer, size, 0);
            // TODO: burası mobilde çalışmıyor!!!!!
            // std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", recv" << std::endl;
            recv(clients.at(i).liveSocket, localBuffer, 3, 0);
            // std::cerr << clients.at(i).type << ", " << clients.at(i).liveSocket << ", recv2" << std::endl;
            // std::cout << buffer << std::endl;
        }
    }
}

void createObjFile(Mesh mesh, std::string filename) {
    // // create .obj file
    // std::string fileToWrite = "3d.obj";
    std::ofstream file(filename);

    if (file.is_open()) {
        for (Vertex& point : mesh.getPoints()) {
            file << point;
        }
        for (Face& face : mesh.getFaces()) {
            file << face;
        }
        file.close();
    } else {
        std::cerr << "Error: Unable to open file for writing." << std::endl;
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
    // char buffer[BUFFER_SIZE];
    std::vector<char> buffer(BUFFER_SIZE, '\0');
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


    std::fill(buffer.begin(), buffer.end(), '\0');
    std::ostringstream oss;
    oss << "START_SCANNING";
    std::string message = oss.str();
    std::copy(message.begin(), message.end(), buffer.begin());
    mainScannerSend(buffer);

    // memset(buffer, '\0', BUFFER_SIZE);
    // sprintf(buffer, "START_SCANNING");
    // mainScannerSend(buffer, BUFFER_SIZE);
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
        
        // save_path = "imgs_db/original/" + std::to_string(counter) + ".jpg";
        // img = cv::imread(save_path);

        // take actual picture
        char filename[] = "output.jpg";
        pthread_mutex_lock(&cameraMutex);
        takePic(filename);
        img = cv::imread(filename);
        pthread_mutex_unlock(&cameraMutex);

        save_path = "imgs/original/" + std::to_string(counter) + ".jpg";
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

        if(counter == 0) {
            int cIndex = 0;
            for (int r = 0; r < h; r++) {
                for (int c = 0; c < w; c++) {
                    if (backG.at<uchar>(r, c) == 1 && c > cIndex) {
                        cIndex = c;
                    }
                }
            }
            centerC = cIndex + 40;
            std::cout << "centerC: " << centerC << std::endl;
        }

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
        // std::cout << "tempV size: " << tempV.size() << std::endl;
        
        int percentage = 100;
        int itemsToKeep = static_cast<int>(tempV.size() * (config.vertical_precision / 100.0));
        // std::cout << "items to keep: " << itemsToKeep << std::endl;
        itemsToKeep = std::max(itemsToKeep, 1);

        double stepSize = static_cast<double>(tempV.size()) / itemsToKeep;

        
        // std::cout << "stepSize: " << stepSize << std::endl;
        vector<Vertex> V;

        for (double i = 0; i < tempV.size(); i += stepSize) {
            V.push_back(tempV[static_cast<int>(i)]);
        }
        meshPoints.push_back(V);
        lineLength.push_back(-1 * V.size());        
        
        std::cout << "theta: " << theta << std::endl;
        std::cout << meshPoints.back().size() << std::endl;

        if(true) {
            std::fill(buffer.begin(), buffer.end(), '\0');
            oss.str("");
            oss << "ROUND";
            oss << " " << counter+1 << " " << config.horizontal_precision;
            message = oss.str();
            std::copy(message.begin(), message.end(), buffer.begin());
            mainScannerSend(buffer);

            // memset(buffer, '\0', BUFFER_SIZE);
            // sprintf(buffer, "ROUND %d %d", counter+1, config.horizontal_precision);
            // std::cout << "buffer: " << buffer << std::endl;
            // // send(clientSocket, buffer, sizeof(buffer), 0);
            // mainScannerSend(buffer, BUFFER_SIZE);

            std::fill(buffer.begin(), buffer.end(), '\0');
            oss.str("");
            oss << "NUMBER_OF_VERTICES";
            oss << " " << (int)meshPoints.back().size();
            message = oss.str();
            std::copy(message.begin(), message.end(), buffer.begin());
            mainScannerSend(buffer, true);

            // memset(buffer, '\0', BUFFER_SIZE);
            // sprintf(buffer, "NUMBER_OF_VERTICES %d", (int)meshPoints.back().size());
            // // send(clientSocket, buffer, sizeof(buffer), 0);
            // mainScannerSend(buffer, BUFFER_SIZE, true);
            // std::cout << "buffer: " << buffer << std::endl;
            // printf("size: %s\n", buffer);
            // // usleep(300000);
        }

        vector<Vertex> vertices = meshPoints.back();
        uint numOfScannedPoints = vertices.size();

        if(true) {
            for(int i=0; i<numOfScannedPoints; i++) {
                std::fill(buffer.begin(), buffer.end(), '\0');
                double x = vertices.at(i).x, y = vertices.at(i).y, z = vertices.at(i).z;
                oss.str("");
                oss << x << " " << y << " " << z;
                message = oss.str();
                std::copy(message.begin(), message.end(), buffer.begin());
                mainScannerSend(buffer, true);

                // memset(buffer, '\0', BUFFER_SIZE);
                // double x = vertices.at(i).x, y = vertices.at(i).y, z = vertices.at(i).z;

                // sprintf(buffer, "%lf %lf %lf", x, y, z);
                // mainScannerSend(buffer, BUFFER_SIZE, true);
                // send(clientSocket, buffer, sizeof(buffer), 0);
                // usleep(3);
                // recv(clientSocket, buffer, sizeof(buffer), 0);
            }
            // memset(buffer, '\0', BUFFER_SIZE);
            // sprintf(buffer, "IMAGES");
            // // send(clientSocket, buffer, sizeof(buffer), 0);
            // mainScannerSend(buffer, BUFFER_SIZE, true);
            std::fill(buffer.begin(), buffer.end(), '\0');
            oss.str("");
            oss << "IMAGES";
            message = oss.str();
            std::copy(message.begin(), message.end(), buffer.begin());
            mainScannerSend(buffer, true);

            // send image size
            std::vector<uchar> imageBuffer;
            imencode(".jpg", img, imageBuffer);
            int imgSize = imageBuffer.size();
            // std::cout << "livesocket for image: " << clients.at(0).liveSocket << std::endl;
            // std::cout << "first image size: " << imgSize << std::endl;
            mainScannerSend(&imgSize);
            // for(int i=0; i<clients.size(); i++) {
            //     if(clients.at(i).type == DESKTOP) {
            //         send(clients.at(i).liveSocket, &imgSize, sizeof(int), 0);
            //         recv(clients.at(i).liveSocket, buffer, 3, 0);
            //     }
            // }
            
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
            // std::cout << "imageBuffer size: " << imageBuffer.size() << std::endl;
            imgSize = imageBuffer.size();
            // std::cout << "livesocket for image: " << clients.at(0).liveSocket << std::endl;
            // std::cout << "second image size: " << imgSize << std::endl;
            mainScannerSend(&imgSize);
            
            // send image
            chunkSize = BUFFER_SIZE; // Choose an appropriate chunk size
            for (int i = 0; i <imgSize; i += chunkSize) {
                int remaining = std::min(chunkSize, imgSize - i);
                // std::cout << "remaining: " << remaining << std::endl;
                mainScannerSend(imageBuffer.data() + i, remaining, true);
                // send(clients.at(0).liveSocket, imageBuffer.data() + i, remaining, 0);
                // recv(clients.at(0).liveSocket, buffer, BUFFER_SIZE, 0);
            }
        }
        
        theta = theta + static_cast<double>((360.0 / config.horizontal_precision));
        // std::cout << "counter: " << counter << std::endl;
        counter++;
        currentStepNumber = counter;

        move(2048 / (int)config.horizontal_precision);
    }

    // std::cout << "outside" << std::endl;

    std::fill(buffer.begin(), buffer.end(), '\0');
    // std::cout << "outside2" << std::endl;
    oss.str("");
    // std::cout << "outside3" << std::endl;
    oss << "FINISH_SCANNING";
    // std::cout << "outside4" << std::endl;
    message = oss.str();
    // std::cout << "outside5" << std::endl;
    std::copy(message.begin(), message.end(), buffer.begin());
    // std::cout << "outside6" << std::endl;
    mainScannerSend(buffer);
    // std::cout << "outside7" << std::endl;

    if(!isCancelled) {

        // std::cout << "debug1";

        int shortest = meshPoints[distance(lineLength.begin(), max_element(lineLength.begin(), lineLength.end()))].size();

        // std::cout << "debug2";

        for (vector<vector<Vertex>>::iterator it = meshPoints.begin(); it != meshPoints.end(); ++it) {
            while (it->size() > shortest) {
                it->erase(it->end() - 2);
            }
        }

        // std::cout << "debug3";

        Mesh mesh;
        Mesh meshMobile;
        int numOfVertices = meshPoints.back().size() * meshPoints.size();

        // std::cout << "debug4";
        
        if(numOfVertices > MOBILE_VERTEX_LIMIT) {
            // std::cout << "debug" << std::endl;
            std::vector<std::vector<Vertex>> meshPointsMobile;
            int itemsToKeep = MOBILE_VERTEX_LIMIT;

            double stepSize = static_cast<double>(numOfVertices) / itemsToKeep;

            for(int i=0; i<meshPoints.size(); i++) {
                vector<Vertex> V;
                for (double j = 0; j < meshPoints.at(i).size(); j += stepSize) {
                    V.push_back(meshPoints.at(i).at(static_cast<int>(j)));
                }
                meshPointsMobile.push_back(V);
            }
            meshMobile.build(meshPointsMobile);
            meshMobile.normalize();
        }
        mesh.build(meshPoints);
        mesh.normalize();

        createObjFile(mesh, "3d.obj");
        if(numOfVertices >= MOBILE_VERTEX_LIMIT) {
            std::cout << "3dmobile.obj created" << std::endl;
            createObjFile(meshMobile, "3dmobile.obj");
        }

        // Read the contents of the .obj file
        std::ifstream objFile("3d.obj");
        std::stringstream objBuffer;
        objBuffer << objFile.rdbuf();
        objFile.close();

        // Get the content as a string
        std::string objContent = objBuffer.str();

        std::ifstream objFileMobile("3dmobile.obj");
        std::stringstream objBufferMobile;
        objBufferMobile << objFileMobile.rdbuf();
        objFileMobile.close();

        std::string objContentMobile = objBufferMobile.str();

        // Send the size of the .obj content to the client
        int objSize = objContent.size();
        int objSizeMobile = objContentMobile.size();

        if(numOfVertices < MOBILE_VERTEX_LIMIT) {
            std::fill(buffer.begin(), buffer.end(), '\0');
            oss.str("");
            oss << "FILE";
            oss << " " << objSize;
            message = oss.str();
            std::copy(message.begin(), message.end(), buffer.begin());
            mainScannerSend(buffer);
        } else {
            // std::cout << "test...." << std::endl;
            std::fill(buffer.begin(), buffer.end(), '\0');
            oss.str("");
            oss << "FILE";
            oss << " " << objSize;
            message = oss.str();
            std::copy(message.begin(), message.end(), buffer.begin());
            mainScannerSend(buffer, DESKTOP);

            std::fill(buffer.begin(), buffer.end(), '\0');
            oss.str("");
            oss << "FILE";
            oss << " " << objSizeMobile;
            message = oss.str();
            std::copy(message.begin(), message.end(), buffer.begin());
            mainScannerSend(buffer, MOBILE);
        }

        if(numOfVertices < MOBILE_VERTEX_LIMIT) {
            // Send the .obj content to the client in packages of size 1024
            int chunkSize = BUFFER_SIZE;
            for (int i = 0; i < objSize; i += chunkSize) {
                int remaining = std::min(chunkSize, objSize - i);
                mainScannerSend(objContent.c_str() + i, remaining);
                // send(clients.at(0).liveSocket, objContent.c_str() + i, remaining, 0);
                // recv(clients.at(0).liveSocket, buffer, BUFFER_SIZE, 0);
            }
        } else {
            // Send the .obj content to the client in packages of size 1024
            int chunkSize = BUFFER_SIZE;
            for (int i = 0; i < objSize; i += chunkSize) {
                int remaining = std::min(chunkSize, objSize - i);
                mainScannerSend(objContent.c_str() + i, remaining, DESKTOP);
                // send(clients.at(0).liveSocket, objContent.c_str() + i, remaining, 0);
                // recv(clients.at(0).liveSocket, buffer, BUFFER_SIZE, 0);
            }
            for (int i = 0; i < objSizeMobile; i += chunkSize) {
                int remaining = std::min(chunkSize, objSizeMobile - i);
                mainScannerSend(objContentMobile.c_str() + i, remaining, MOBILE);
                // send(clients.at(0).liveSocket, objContent.c_str() + i, remaining, 0);
                // recv(clients.at(0).liveSocket, buffer, BUFFER_SIZE, 0);
            }
        }

        if(numOfVertices < MOBILE_VERTEX_LIMIT) {
            std::fill(buffer.begin(), buffer.end(), '\0');
            oss.str("");
            oss << "FILE_END";
            oss << " " << objSize;
            message = oss.str();
            std::copy(message.begin(), message.end(), buffer.begin());
            mainScannerSend(buffer);
        } else {
            std::fill(buffer.begin(), buffer.end(), '\0');
            oss.str("");
            oss << "FILE_END";
            oss << " " << objSize;
            message = oss.str();
            std::copy(message.begin(), message.end(), buffer.begin());
            mainScannerSend(buffer, DESKTOP);

            std::fill(buffer.begin(), buffer.end(), '\0');
            oss.str("");
            oss << "FILE_END";
            oss << " " << objSizeMobile;
            message = oss.str();
            std::copy(message.begin(), message.end(), buffer.begin());
            mainScannerSend(buffer, MOBILE);
        }


        pthread_mutex_lock(&scannerStateMutex);
        scannerState = FINISHED;
        pthread_mutex_unlock(&scannerStateMutex);

        broadcastMessage("scanner_state FINISHED");

        std::cout << "scanning finished inside scanner successfully\n";

        std::cout << "number of vertices: " << numOfVertices << std::endl;

    } else {
        std::cout << "scanning has canncelled\n";
    }

    // cv::waitKey(0);
}
