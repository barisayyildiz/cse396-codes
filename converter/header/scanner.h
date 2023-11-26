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
#include <chrono>

#define STEPPER_PIN_1 1
#define STEPPER_PIN_2 7
#define STEPPER_PIN_3 8
#define STEPPER_PIN_4 25

#define STEP_PER_MOVEMENT 8
#define DELAY_ONE_STEP 0.003

extern int stepNumber;
extern int counter;

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
// void oneStep(bool direction);
// void move(int stepPrecision);
