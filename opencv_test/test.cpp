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

using namespace std;
using namespace cv;

cv::Mat fourPointTransform(cv::Mat image, std::vector<cv::Point2f> pts) {
    cv::Point2f tl = pts[0];
    cv::Point2f tr = pts[1];
    cv::Point2f br = pts[2];
    cv::Point2f bl = pts[3];

    // Compute the width of the new image
    float widthA = std::sqrt(std::pow(br.x - bl.x, 2) + std::pow(br.y - bl.y, 2));
    float widthB = std::sqrt(std::pow(tr.x - tl.x, 2) + std::pow(tr.y - tl.y, 2));
    int maxWidth = static_cast<int>(std::max(widthA, widthB));

    std::cout << "widthA: " << widthA << std::endl;
    std::cout << "widthB: " << widthB << std::endl;
    std::cout << "maxWidth: " << maxWidth << std::endl;

    // Compute the height of the new image
    float heightA = std::sqrt(std::pow(tr.x - br.x, 2) + std::pow(tr.y - br.y, 2));
    float heightB = std::sqrt(std::pow(tl.x - bl.x, 2) + std::pow(tl.y - bl.y, 2));
    int maxHeight = static_cast<int>(std::max(heightA, heightB));

    std::cout << "heightA: " << heightA << std::endl;
    std::cout << "heightB: " << heightB << std::endl;
    std::cout << "maxHeight: " << maxHeight << std::endl;

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

int main() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cout << "Error: Camera not found" << std::endl;
        return 1;
    }
    cv::Mat img;
    cap >> img;
    cap.release();

    if(!cv::imwrite("imgs/test.jpg", img)) {
        std::cerr << "an error occured";
    }

    // std::cout << img.rows << ", " << img.cols << std::endl;

    // Point2f pts[4];
    // pts[0] = { 0.0, 0.0 };
    // pts[1] = { 800.0, 0.0 };
    // pts[2] = { 800.0, 800.0 };
    // pts[3] = { 0.0, 800.0 };

    // cv::Mat transformed = fourPointTransform(img, std::vector<cv::Point2f>(pts, pts + 4));

    // cv::imshow("test", img);
    // cv::imshow("transformed", transformed);
    cv::waitKey(0);

    return 0;
}
