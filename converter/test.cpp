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

#include <wiringPi.h>
#include <chrono>

using namespace cv;
using namespace std;

const int out1 = 13;
const int out2 = 16;
const int out3 = 5;
const int out4 = 12;
const int buttonPin = 23;
const int ledPin = 18;


#define STEPPER_PIN_1 1
#define STEPPER_PIN_2 7
#define STEPPER_PIN_3 8
#define STEPPER_PIN_4 25

#define DELAY_ONE_STEP 30
#define DELAY_MOVE 150

#define STEP_SIZE 32

#define DEGREE_PER_ROTATION 5.625

int step_number = 0;
int counter = 0;

class Vertex {
    public:
        int x, y, z;
        stringstream ss;
        
        Vertex(int x, int y, int z) : x(x), y(y), z(z) {
            ss << "v " << x << " " << y << " " << z;
        }

        // Copy constructor
        Vertex(const Vertex& other) : x(other.x), y(other.y), z(other.z), ss(other.ss.str()) {
        }

        // Copy assignment operator
        Vertex& operator=(const Vertex& other) {
            if (this != &other) {
                x = other.x;
                y = other.y;
                z = other.z;
                ss.str(other.ss.str());
            }
            return *this;
        }

        string write() {
            return ss.str();
        }
        
        friend std::ostream& operator<<(std::ostream& os, const Vertex& vertex) {
            os << vertex.ss.str();
            return os;
        }
};

class Face {
public:
    int v1, v2, v3;
    Face(int v1, int v2, int v3) : v1(v1), v2(v2), v3(v3) {}
    string write() {
        stringstream ss;
        ss << "f " << v1 << " " << v2 << " " << v3;
        return ss.str();
    }
};

Vertex getVertex(Vertex pCoord) {
    int H = pCoord.x;
    int t = pCoord.y;
    int d = pCoord.z;
    int x = d * cos(t);
    int y = d * sin(t);
    int z = H;
    return Vertex(x, y, z);
}

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

void oneStep(bool direction){
    // std::cout << "counter : " << ++counter << std::endl;
    if(direction){
    switch(step_number){
      case 0:
      digitalWrite(STEPPER_PIN_1, HIGH);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
      case 1:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, HIGH);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
      case 2:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, HIGH);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
      case 3:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, HIGH);
      break;
    }
  }
  else{
    switch(step_number){
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
  }
  step_number++;
  if(step_number > 3){
    step_number = 0;
  }
}

void move(bool direction, int stepPrecision){
    int precisionCounter = 0;
    
    while(precisionCounter < stepPrecision){
        
        oneStep(direction);
        precisionCounter++;

        delay(DELAY_ONE_STEP);
    }

    // delay(DELAY_MOVE);
}

int main() {

    wiringPiSetupGpio();

    pinMode(STEPPER_PIN_1, OUTPUT);
    pinMode(STEPPER_PIN_2, OUTPUT);
    pinMode(STEPPER_PIN_3, OUTPUT);
    pinMode(STEPPER_PIN_4, OUTPUT);

    int i = 0;
    int numItt = 20;
    double theta = 0;
    double thetaInc = 360.0 / numItt;
    double motorPos = 0;
    double motorPosI = 400.0 / numItt;

    vector<vector<Vertex>> meshPoints;
    vector<int> lineLength;
    // double theta = 0.0;

    for(; theta<360; ) {
        cv::VideoCapture cap(0);
        if (!cap.isOpened()) {
            cout << "Error: Camera not found" << endl;
            return 1;
        }
        cv::Mat img;
        cap >> img;
        cap.release();

        delay(1000);

        // img = cv::imread("test_image.jpg");

        // std::cout << img.rows << "," << img.cols << std::endl;

        Point2f pts[4];
        pts[0] = { 0.0, 0.0 };
        pts[1] = { 200.0, 0.0 };
        pts[2] = { 200.0, 200.0 };
        pts[3] = { 0.0, 200.0 };

        // img = fourPointTransform(img, std::vector<cv::Point2f>(pts, pts + 4));

        Mat red_line;
        // B, G, R
        Scalar lowerb(220, 220, 220);
        Scalar upperb(255, 255, 255);
        cv::inRange(img, lowerb, upperb, red_line);
        
        // Create windows and display images
        // cv::namedWindow("Original Image", cv::WINDOW_NORMAL);
        // cv::imshow("Original Image", img);

        // cv::namedWindow("Red Line Image", cv::WINDOW_NORMAL);
        // cv::imshow("Red Line Image", red_line);

        Mat backG;
        int bottomR = 0;
        vector<Vertex> tempV;

        // // ---------- Preview the filtered picture (if needed) ----------
        // // imshow("perspective", red_line);
        // // waitKey(0);

        int h = red_line.rows;
        int w = red_line.cols;

        backG = Mat::zeros(h, w, CV_32F);

        // cv::imshow("background", backG);

        cv::Mat modified = backG.clone();

        for(int r=0; r<h; r++) {
            for(int c=0; c<w; c++) {
                if(red_line.at<uchar>(r,c) != 0) {
                    // std::cout << "(" << r << "," << c << ")" << std::endl;
                    backG.at<float>(r, c) = 1.0;
                    bottomR = r;
                    break;
                }
            }
        }

        // cv::imshow("background2", backG);
        
        int centerC = 420; // center column

        for (int r = 0; r < h; r++) {
            int cIndex = 0;
            for (int c = 0; c < w; c++) {
                if (backG.at<float>(r, c) == 1) {
                    int H = r - bottomR;
                    int dist = c - centerC;
                    double t = theta * (M_PI / 180.0); // Convert degrees to radians
                    tempV.push_back(Vertex(H, t, dist));
                }
            }
        }

        std::cout << tempV.size() << std::endl;

        int intv = 20; // Vertical resolution
        intv = tempV.size() / intv;

        std::cout << "intv : " << intv << std::endl;

        if (!tempV.empty() && intv != 0) {
            vector<Vertex> V;
            V.push_back(tempV[0]);

            for (int ind = 1; ind < tempV.size() - 2; ind++) {
                if (ind % intv == 0) {
                    V.push_back(tempV[ind]);
                }
            }

            V.push_back(tempV[tempV.size() - 1]);
            std::cout << "v size : " << V.size() << std::endl;
            meshPoints.push_back(V);
            lineLength.push_back(-1 * V.size());
        }

        std::cout << "theta: " << theta << std::endl;
        std::cout << "meshPoints length: " << meshPoints.size() << std::endl << std::endl;

        delay(150);
        move(true, STEP_SIZE);

        theta = theta+DEGREE_PER_ROTATION;

        // // theta += thetaInc;
        // // // Step the motor
        // // i = step(static_cast<int>(motorPosI), i);
        // // usleep(300000);
        // // theta += thetaInc;
        
    }

    std::cout << meshPoints.at(0).size() << std::endl;

    int shortest = meshPoints[distance(lineLength.begin(), max_element(lineLength.begin(), lineLength.end()))].size();

    std::cout << "shortest : " << shortest << std::endl;

    for (vector<vector<Vertex>>::iterator it = meshPoints.begin(); it != meshPoints.end(); ++it) {
        while (it->size() > shortest) {
            it->erase(it->end() - 2);
        }
    }

    std::cout << "mesh point first item size : " << meshPoints.at(0).size() << std::endl;

    vector<Vertex> points;
    vector<Face> faces;
    vector<int> firstRow;
    vector<int> prevRow;

    for (int index = 0; index < meshPoints[0].size(); ++index) {
        points.push_back(getVertex(meshPoints[0][index]));
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

                points.push_back(getVertex(meshPoints[col][point]));
                currentRow.push_back(tl);

                if (point == meshPoints[col].size() - 2) {
                    points.push_back(getVertex(meshPoints[col][point + 1]));
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

            prevRow = currentRow;
        }
    }

    // create .obj file
    std::string fileToWrite = "3d.obj";
    std::ofstream file(fileToWrite);

    if (file.is_open()) {
        for (Vertex& point : points) {
            file << point.write() << "\n";
        }

        for (Face& f : faces) {
            file << f.write() << "\n";
        }

        file.close();
    } else {
        std::cerr << "Error: Unable to open file for writing." << std::endl;
    }

    // cv::waitKey(0);
    

    return 0;
}


