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

// #include <wiringPi.h>
#include <chrono>

using namespace cv;
using namespace std;

#define STEPPER_PIN_1 1
#define STEPPER_PIN_2 7
#define STEPPER_PIN_3 8
#define STEPPER_PIN_4 25

#define STEP_PER_MOVEMENT 8
#define DELAY_ONE_STEP 0.003

int stepNumber = 0;
int counter = 0;

class Vertex {
    public:
        double x, y, z;
        stringstream ss;
        
        Vertex(double x, double y, double z) : x(x), y(y), z(z) {
            ss << "v " << x << " " << y << " " << z << "\n";
            ss << "vt " << x << " " << y << "\n";
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
            os << "v " << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
            os << "vt " << vertex.x << " " << vertex.y << "\n";
            return os;
        }
};

class Face {
public:
    int v1, v2, v3;
    Face(int v1, int v2, int v3) : v1(v1), v2(v2), v3(v3) {}
    string write() {
        stringstream ss;
        ss << "f " << v1 << " " << v2 << " " << v3 << "\n";
        return ss.str();
    }
    friend std::ostream& operator<<(std::ostream& os, const Face& face) {
        os << "f " << face.v1 << " " << face.v2 << " " << face.v3 << "\n";
        return os;
    }
};

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

// void oneStep(bool direction){
//     // std::cout << "counter : " << ++counter << std::endl;
//     switch(stepNumber){
//       case 0:
//       digitalWrite(STEPPER_PIN_1, LOW);
//       digitalWrite(STEPPER_PIN_2, LOW);
//       digitalWrite(STEPPER_PIN_3, LOW);
//       digitalWrite(STEPPER_PIN_4, HIGH);
//       break;
//       case 1:
//       digitalWrite(STEPPER_PIN_1, LOW);
//       digitalWrite(STEPPER_PIN_2, LOW);
//       digitalWrite(STEPPER_PIN_3, HIGH);
//       digitalWrite(STEPPER_PIN_4, LOW);
//       break;
//       case 2:
//       digitalWrite(STEPPER_PIN_1, LOW);
//       digitalWrite(STEPPER_PIN_2, HIGH);
//       digitalWrite(STEPPER_PIN_3, LOW);
//       digitalWrite(STEPPER_PIN_4, LOW);
//       break;
//       case 3:
//       digitalWrite(STEPPER_PIN_1, HIGH);
//       digitalWrite(STEPPER_PIN_2, LOW);
//       digitalWrite(STEPPER_PIN_3, LOW);
//       digitalWrite(STEPPER_PIN_4, LOW);
//       break;
//     }
//     stepNumber++;
//     if(stepNumber > 3){
//         stepNumber = 0;
//     }
// }

// void move(int stepPrecision){
//     int precisionCounter = 0;
//     while(precisionCounter < stepPrecision){
//         oneStep();
//         precisionCounter++;
//         delay(DELAY_ONE_STEP);
//     }
// }

int main() {
    // wiringPiSetupGpio();

    // pinMode(STEPPER_PIN_1, OUTPUT);
    // pinMode(STEPPER_PIN_2, OUTPUT);
    // pinMode(STEPPER_PIN_3, OUTPUT);
    // pinMode(STEPPER_PIN_4, OUTPUT);

    int i = 0;
    double theta = 0;
    int counter = 0;

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

        std::string save_path = "/home/barisayyildiz/cse396-codes/converter/imgs_db/original/" + std::to_string(counter) + ".jpg";
        img = cv::imread(save_path);

        // save_path = "imgs/original/" + std::to_string(counter) + ".jpg";
        // cv::imwrite(save_path, img);

        Point2f pts[4];
        pts[0] = { 277.0, 90.0 };
        pts[1] = { 733.0, 90.0 };
        pts[2] = { 733.0, 634.0 };
        pts[3] = { 277.0, 634.0 };

        img = fourPointTransform(img, std::vector<cv::Point2f>(pts, pts + 4));
        save_path = "imgs_db/four_points/" + std::to_string(counter) + ".jpg";
        cv::imwrite(save_path, img);

        int h = img.rows;
        int w = img.cols;
        Mat backG = Mat::zeros(h, w, CV_32F);
        int bottomR = 0;
        int topR = 0;
        vector<Vertex> tempV;

        cv::Mat modified = backG.clone();

        for(int i=0; i<h; i++) {
            int max = -1;
            int cIndex = -1;
            for(int j=0; j<w; j++) {
                int current = img.at<cv::Vec3b>(i, j)[2];
                if(current > max) {
                    cIndex = j;
                    max = current;
                }
            }
            if(img.at<cv::Vec3b>(i, cIndex)[2] > 25) {
                backG.at<uchar>(i, cIndex) = 1;
                bottomR = i;
                if(topR == 0) {
                    topR = i;
                }
            }
        }
        save_path = "imgs_db/red_line/" + std::to_string(counter) + ".jpg";
        cv::imwrite(save_path, backG);

        int centerC = 275;
        for (int r = 0; r < h; r++) {
            int cIndex = 0;
            for (int c = 0; c < w; c++) {
                if (backG.at<uchar>(r, c) == 1) {
                    double H = r - bottomR;
                    double dist = c - centerC;
                    double t = theta * (M_PI / 180.0); // Convert degrees to radians
                    Vertex coord(H, t, dist);
                    tempV.push_back(Vertex(H, t, dist));
                }
            }
        }

        int intv = 350; // Vertical resolution
        intv = tempV.size() / intv;

        if (!tempV.empty() && intv != 0) {
            vector<Vertex> V;
            V.push_back(tempV[0]);

            for (int ind = 1; ind < tempV.size() - 2; ind++) {
                if (ind % intv == 0) {
                    V.push_back(tempV[ind]);
                }
            }

            V.push_back(tempV[tempV.size() - 1]);
            meshPoints.push_back(V);
            lineLength.push_back(-1 * V.size());
        }

        std::cout << "theta: " << theta << std::endl;

        // move(true, STEP_PER_MOVEMENT);
        theta = theta + static_cast<double>((360.0 / (2048 / STEP_PER_MOVEMENT)));
        counter++;
    }

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