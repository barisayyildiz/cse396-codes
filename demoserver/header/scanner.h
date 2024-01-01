#ifndef SCANNER_H
#define SCANNER_H

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
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <fcntl.h>

#include "communication_layer.h"

#define STEPPER_PIN_1 1
#define STEPPER_PIN_2 7
#define STEPPER_PIN_3 8
#define STEPPER_PIN_4 25

#define BUTTON_PIN 17
#define LED_PIN_1 27
#define LED_PIN_2 22

#define STEP_PER_MOVEMENT 8
#define DELAY_ONE_STEP 8

enum ScannerState {
    RUNNING,
    FINISHED,
    CANCELLED,
    IDLE
};

// hardware state
extern int prevButtonState;

// current scan values
extern int currentStepNumber;
extern int currentHorizontalPrecision;
extern int currentVerticalPrecision;
extern float currentTopLeftX;
extern float currentTopLeftY;
extern float currentBottomRightX;
extern float currentBottomRightY;
extern pthread_mutex_t currentScannerMutex;

extern ScannerState scannerState;
extern pthread_mutex_t scannerStateMutex;
extern int desktopSocket;
extern int mobileSocket;
extern int clientCounter;

struct Configuration {
    int horizontal_precision, vertical_precision;
    float top_left_x, top_left_y;
    float bottom_right_x, bottom_right_y;
};

class Vertex {
    public:
        double x, y, z;
        
        Vertex(double x, double y, double z) : x(x), y(y), z(z) {}

        // Copy constructor
        Vertex(const Vertex& other) : x(other.x), y(other.y), z(other.z) {}
        
        friend std::ostream& operator<<(std::ostream& os, const Vertex& vertex) {
            os << "v " << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
            os << "vt " << vertex.x << " " << vertex.y << "\n";
            return os;
        }
};

class Face {
public:
    int v1, v2, v3;
    Face(int v1, int v2, int v3) : v1(v1), v2(v2), v3(v3) {}

    friend std::ostream& operator<<(std::ostream& os, const Face& face) {
        os << "f " << face.v1 << "/" << face.v1 << " " << face.v2 << "/" << face.v2 << " " << face.v3 << "/" << face.v3 << "\n";
        return os;
    }
};

cv::Mat fourPointTransform(cv::Mat image, std::vector<cv::Point2f> pts);
Vertex getVertex(Vertex pCoord);
void readConfigurationsFile(const char* fileName, Configuration& config);
void writeConfigurationsFile(const char* fileName, Configuration& config);
void mainScanner();

void mainScannerSend(unsigned char buffer[BUFFER_SIZE], int size, int desktopOnly=false);
void mainScannerSend(const std::vector<char>& buffer, int desktopOnly=false);
void mainScannerSend(const char* buffer, int size, int desktopOnly=false);
void mainScannerSend(int *objSize);

void getScannerStateStr(char buffer[BUFFER_SIZE]);
void sendImageForCalibration(int calibrationImageSocket);

#endif
