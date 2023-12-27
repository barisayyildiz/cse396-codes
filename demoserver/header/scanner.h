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

#define STEPPER_PIN_1 1
#define STEPPER_PIN_2 7
#define STEPPER_PIN_3 8
#define STEPPER_PIN_4 25

#define STEP_PER_MOVEMENT 8
#define DELAY_ONE_STEP 8

enum ScannerState {
    RUNNING,
    FINISHED,
    CANCELLED,
    IDLE
};

extern int stepNumber;
extern int counter;
extern pthread_mutex_t scannerStateMutex;
extern ScannerState scannerState;
extern int desktopSocket;
extern int mobileSocket;
extern int clientCounter;

struct Configuration {
    int horizontal_precision, vertical_precision;
    float top_left_x, top_left_y;
    float top_right_x, top_right_y;
    float bottom_right_x, bottom_right_y;
    float bottom_left_x, bottom_left_y;
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
void mainScanner(int& clientSocket);

#endif
