#include <opencv2/opencv.hpp>

int main() {
    // Load an image from file
    cv::Mat image = cv::imread("image.jpg");

    // Check if the image was loaded successfully
    if (image.empty()) {
        std::cerr << "Error: Could not open or find the image." << std::endl;
        return -1;
    }

    // Create a window to display the image
    cv::namedWindow("Original Image", cv::WINDOW_AUTOSIZE);
    cv::imshow("Original Image", image);

    // Perform some image processing (e.g., convert to grayscale)
    cv::Mat grayImage;
    cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);

    // Create a window to display the processed image
    cv::namedWindow("Grayscale Image", cv::WINDOW_AUTOSIZE);
    cv::imshow("Grayscale Image", grayImage);

    // Save the processed image
    cv::imwrite("grayscale_image.jpg", grayImage);

    // Wait for a key press and close the windows when any key is pressed
    cv::waitKey(0);

    // Close all OpenCV windows
    cv::destroyAllWindows();

    return 0;
}
