#include <opencv2/opencv.hpp>

cv::Point clickedPoint(-1, -1);

void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        clickedPoint = cv::Point(x, y);
        std::cout << "Clicked at: (" << x << ", " << y << ")" << std::endl;
    }
}

int main() {
    // Create a VideoCapture object to access the webcam
    cv::VideoCapture cap(0); // 0 corresponds to the default camera (you can change it to 1, 2, etc. for additional cameras)

    // Check if the camera opened successfully
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open the webcam." << std::endl;
        return -1;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 960);

    // Create a window to display the video feed
    cv::namedWindow("Webcam", cv::WINDOW_NORMAL);

    // Set the mouse callback function to capture mouse events
    cv::setMouseCallback("Webcam", onMouse, nullptr);

    while (true) {
        cv::Mat frame;
        
        // Capture a frame from the webcam
        cap >> frame;

        // Check if the frame was captured successfully
        if (frame.empty()) {
            std::cerr << "Error: Frame is empty." << std::endl;
            break;
        }

        // Display the captured frame in the window
        cv::imshow("Webcam", frame);

        // Press the 'q' key to exit the loop
        if (cv::waitKey(1) == 'q') {
            break;
        }
    }

    // Release the VideoCapture and destroy the window
    cap.release();
    cv::destroyWindow("Webcam");

    return 0;
}
