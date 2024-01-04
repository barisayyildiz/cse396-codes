#include <raspicam/raspicam.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <opencv2/opencv.hpp>

int main(int argc, char **argv) {
    raspicam::RaspiCam Camera;
    if (!Camera.open()) {
        std::cerr << "Error opening camera" << std::endl;
        return -1;
    }

    // Wait a while to allow the camera to adjust to lighting conditions
    // sleep(3);

    auto start = std::chrono::steady_clock::now();

    // Capture image
    Camera.grab();
    unsigned char *data = new unsigned char[Camera.getImageTypeSize(raspicam::RASPICAM_FORMAT_RGB)];
    Camera.retrieve(data, raspicam::RASPICAM_FORMAT_RGB);

    // Save the image to a file (you can change the filename as needed)
    std::ofstream outFile("captured_image.ppm", std::ios::binary);
    outFile << "P6\n" << Camera.getWidth() << " " << Camera.getHeight() << " 255\n";
    outFile.write(reinterpret_cast<char*>(data), Camera.getImageTypeSize(raspicam::RASPICAM_FORMAT_RGB));
    delete[] data;
    outFile.close();

    cv::Mat img = cv::imread("captured_image.ppm", cv::IMREAD_UNCHANGED);
    cv::imwrite("image.jpg", img, {cv::IMWRITE_JPEG_QUALITY, 95});

    // cv::imshow("image", img);

    // Close the camera
    Camera.release();

    // cv::waitKey(0);

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    std::cout << "Time taken by function takePic: " << duration.count() << " milliseconds" << std::endl;

    return 0;
}
