#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
// #include <wiringPi.h>
#include <unistd.h>
#include <vector>
#include <cmath>
#include <iostream>

int main() {
    int width = 10, height = 10;
    cv::Mat backG = cv::Mat::zeros(width, height, CV_32F);

    // cv::imshow("image", backG);
    // for(int i=0; i<height; i++) {
    //     for(int j=0; j<width; j++) {
    //         backG.at<float>(i,j) = 1.0f;
    //     }
    // }

    backG.at<int>(4, 4) = 1;

    for(int i=0; i<height; i++) {
        for(int j=0; j<width; j++) {
            std::cout << backG.at<int>(i, j) << " ";
        }
        std::cout << std::endl;
    }

    // cv::waitKey(0);

    // cv::Mat backG = cv::Mat::zeros(544, 456, cv::IMREAD_GRAYSCALE);
    // cv::Mat img = cv::imread("imgs/red_line/0.jpg", cv::IMREAD_GRAYSCALE);

    // cv::imshow("image", img);

    // int h, w;
    // h = img.rows;
    // w = img.cols;

    // std::cout << h << " " << w;

    // for(int r = 0; r < h; ++r) {
    //     for(int c = 0; c < w; ++c) {
    //         // Check if the pixel is white
    //         // std::cout << img.at<int>(r,c) << " ";
    //         if (img.at<int>(r, c) == 256 || img.at<int>(r, c) == 255) {
    //             std::cout << "White pixel at (" << r << ", " << c << ")" << std::endl;
    //         }
    //     }
    //     std::cout << std::endl;
    // }

    // cv::waitKey(0);

    return 0;
}
