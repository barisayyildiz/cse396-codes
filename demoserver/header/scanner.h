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

#define MOBILE_VERTEX_LIMIT 60000

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

extern pthread_mutex_t cameraMutex;

extern int stepNumber;

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

class Mesh {
    public:
        Mesh(){};
        void build(std::vector<std::vector<Vertex>> meshPoints) {
            std::vector<int> firstRow;
            std::vector<int> prevRow;
            std::vector<int> lastVertices;

            for (int index = 0; index < meshPoints[0].size(); ++index) {
                points.push_back(meshPoints[0][index]);
                firstRow.push_back(index + 1);
            }

            prevRow = firstRow;

            for (int col = 0; col < meshPoints.size(); ++col) {
                if (col != 0) {
                    int indexS = prevRow.back();
                    std::vector<int> currentRow;

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
        }
        void normalize() {
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
        }
        std::vector<Vertex> getPoints() {
            return points;
        }
        std::vector<Face> getFaces() {
            return faces;
        }
    private:
        std::vector<Vertex> points;
        std::vector<Face> faces;
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

// from converter
void oneStep(bool direction);
void move(int stepPrecision);
void takePic (char* filename);

#endif
