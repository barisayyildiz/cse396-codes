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

using namespace cv;
using namespace std;

const int out1 = 13;
const int out2 = 16;
const int out3 = 5;
const int out4 = 12;
const int buttonPin = 23;
const int ledPin = 18;

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


// int step(int x, int i) {
//     int positive = 0;
//     int negative = 0;
//     int y = 0;
//     digitalWrite(out1, LOW);
//     digitalWrite(out2, LOW);
//     digitalWrite(out3, LOW);
//     digitalWrite(out4, LOW);

//     if (x > 0 && x <= 400) {
//         for (y = x; y > 0; y--) {
//             if (negative == 1) {
//                 if (i == 7) {
//                     i = 0;
//                 } else {
//                     i = i + 1;
//                 }
//                 y = y + 2;
//                 negative = 0;
//             }
//             positive = 1;
//             if (i == 0) {
//                 digitalWrite(out1, HIGH);
//                 digitalWrite(out2, LOW);
//                 digitalWrite(out3, LOW);
//                 digitalWrite(out4, LOW);
//                 usleep(30000);
//             } else if (i == 1) {
//                 digitalWrite(out1, HIGH);
//                 digitalWrite(out2, HIGH);
//                 digitalWrite(out3, LOW);
//                 digitalWrite(out4, LOW);
//                 usleep(30000);
//             } else if (i == 2) {
//                 digitalWrite(out1, LOW);
//                 digitalWrite(out2, HIGH);
//                 digitalWrite(out3, LOW);
//                 digitalWrite(out4, LOW);
//                 usleep(30000);
//             } else if (i == 3) {
//                 digitalWrite(out1, LOW);
//                 digitalWrite(out2, HIGH);
//                 digitalWrite(out3, HIGH);
//                 digitalWrite(out4, LOW);
//                 usleep(30000);
//             } else if (i == 4) {
//                 digitalWrite(out1, LOW);
//                 digitalWrite(out2, LOW);
//                 digitalWrite(out3, HIGH);
//                 digitalWrite(out4, LOW);
//                 usleep(30000);
//             } else if (i == 5) {
//                 digitalWrite(out1, LOW);
//                 digitalWrite(out2, LOW);
//                 digitalWrite(out3, HIGH);
//                 digitalWrite(out4, HIGH);
//                 usleep(30000);
//             } else if (i == 6) {
//                 digitalWrite(out1, LOW);
//                 digitalWrite(out2, LOW);
//                 digitalWrite(out3, LOW);
//                 digitalWrite(out4, HIGH);
//                 usleep(30000);
//             } else if (i == 7) {
//                 digitalWrite(out1, HIGH);
//                 digitalWrite(out2, LOW);
//                 digitalWrite(out3, LOW);
//                 digitalWrite(out4, HIGH);
//                 usleep(30000);
//             }
//             if (i == 7) {
//                 i = 0;
//                 continue;
//             }
//             i = i + 1;
//         }
//     } else if (x < 0 && x >= -400) {
//         x = x * -1;
//         for (y = x; y > 0; y--) {
//             if (positive == 1) {
//                 if (i == 0) {
//                     i = 7;
//                 } else {
//                     i = i - 1;
//                 }
//                 y = y + 3;
//                 positive = 0;
//             }
//             negative = 1;
//             if (i == 0) {
//                 digitalWrite(out1, HIGH);
//                 digitalWrite(out2, LOW);
//                 digitalWrite(out3, LOW);
//                 digitalWrite(out4, LOW);
//                 usleep(30000);
//             } else if (i == 1) {
//                 digitalWrite(out1, HIGH);
//                 digitalWrite(out2, HIGH);
//                 digitalWrite(out3, LOW);
//                 digitalWrite(out4, LOW);
//                 usleep(30000);
//             } else if (i == 2) {
//                 digitalWrite(out1, LOW);
//                 digitalWrite(out2, HIGH);
//                 digitalWrite(out3, LOW);
//                 digitalWrite(out4, LOW);
//                 usleep(30000);
//             } else if (i == 3) {
//                 digitalWrite(out1, LOW);
//                 digitalWrite(out2, HIGH);
//                 digitalWrite(out3, HIGH);
//                 digitalWrite(out4, LOW);
//                 usleep(30000);
//             } else if (i == 4) {
//                 digitalWrite(out1, LOW);
//                 digitalWrite(out2, LOW);
//                 digitalWrite(out3, HIGH);
//                 digitalWrite(out4, LOW);
//                 usleep(30000);
//             } else if (i == 5) {
//                 digitalWrite(out1, LOW);
//                 digitalWrite(out2, LOW);
//                 digitalWrite(out3, HIGH);
//                 digitalWrite(out4, HIGH);
//                 usleep(30000);
//             } else if (i == 6) {
//                 digitalWrite(out1, LOW);
//                 digitalWrite(out2, LOW);
//                 digitalWrite(out3, LOW);
//                 digitalWrite(out4, HIGH);
//                 usleep(30000);
//             } else if (i == 7) {
//                 digitalWrite(out1, HIGH);
//                 digitalWrite(out2, LOW);
//                 digitalWrite(out3, LOW);
//                 digitalWrite(out4, HIGH);
//                 usleep(30000);
//             }
//             if (i == 0) {
//                 i = 7;
//                 continue;
//             }
//             i = i - 1;
//         }
//     }
//     return i;
// }

std::ostream& operator<<(std::ostream& os, const std::vector<int>& vec) {
    os << "[";
    for (const int& element : vec) {
        os << element << ",";
    }
    os << "\b]";
    return os;
}

void test() {

    
}

int main() {
    // if (wiringPiSetup() == -1) {
    //     cout << "WiringPi setup failed." << endl;
    //     return 1;
    // }

    // pinMode(out1, OUTPUT);
    // pinMode(out2, OUTPUT);
    // pinMode(out3, OUTPUT);
    // pinMode(out4, OUTPUT);
    // pinMode(buttonPin, INPUT);
    // pullUpDnControl(buttonPin, PUD_UP);
    // pinMode(ledPin, PWM_OUTPUT);

    int i = 0;
    int numItt = 20;
    double theta = 0;
    double thetaInc = 360.0 / numItt;
    double motorPos = 0;
    double motorPosI = 400.0 / numItt;

    vector<vector<Vertex>> meshPoints;
    vector<int> lineLength;

    while (true) {
        // while (digitalRead(buttonPin) != LOW) {
        //     usleep(100000);
        // }

        // pwmWrite(ledPin, 512);

        while (theta < 360) {
            cv::VideoCapture cap(0);
            if (!cap.isOpened()) {
                cout << "Error: Camera not found" << endl;
                return 1;
            }

            cv::Mat img;
            cap >> img;
            cap.release();

            // std::cout << img.rows << "," << img.cols << std::endl;

            // std::cout << "image captured..." << std::endl;

            cv::imshow("original image", img);

            Point2f pts[4];
            pts[0] = { 375.0, 275.0 };
            pts[1] = { 1090.0, 420.0 };
            pts[2] = { 1090.0, 915.0 };
            pts[3] = { 375.0, 1060.0 };

            img = fourPointTransform(img, std::vector<cv::Point2f>(pts, pts + 4));
            cv::imshow("transformed", img);

            Mat red_line;
            Scalar lowerb(50, 0, 0);
            Scalar upperb(255, 255, 255);
            cv::inRange(img, lowerb, upperb, red_line);

            Mat backG;
            int bottomR = 0;
            vector<Vertex> tempV;

            // // ---------- Preview the filtered picture (if needed) ----------
            // // imshow("perspective", red_line);
            // // waitKey(0);

            int h = red_line.rows;
            int w = red_line.cols;

            backG = Mat::zeros(h, w, CV_8UC1);

            cv::imshow("background", backG);


            for (int r = 0; r < h; r++) {
                int cIndex = 0;
                for (int c = 0; c < w; c++) {
                    if (red_line.at<uchar>(r, c) != 0) {
                        backG.at<uchar>(r, c) = 1;
                        bottomR = r;
                        cIndex = c;
                    }
                }
            }

            // // ---------- Preview the processed picture (if needed) ----------
            // // imshow("perspective", backG);
            // // waitKey(0);

            int centerC = 420; // center column

            for (int r = 0; r < h; r++) {
                int cIndex = 0;
                for (int c = 0; c < w; c++) {
                    if (backG.at<uchar>(r, c) == 1) {
                        int H = r - bottomR;
                        int dist = c - centerC;
                        double t = theta * (M_PI / 180.0); // Convert degrees to radians
                        tempV.push_back(Vertex(H, t, dist));
                    }
                }
            }

            int intv = 20; // Vertical resolution
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

            // theta += thetaInc;
            // // Step the motor
            // i = step(static_cast<int>(motorPosI), i);
            // usleep(300000);
            sleep(30);

            // theta += thetaInc;
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

        // TODO: create and send .obj file
    }

    return 0;
}


