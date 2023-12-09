#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <wiringPi.h>
#include <unistd.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include "../header/scanner.h"

int stepNumber = 0;
int counter = 0;

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

void oneStep(){
    // std::cout << "counter : " << ++counter << std::endl;
    switch(stepNumber){
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
    stepNumber++;
    if(stepNumber > 3){
        stepNumber = 0;
    }
}

void move(int stepPrecision){
    int precisionCounter = 0;
    while(precisionCounter < stepPrecision){
        oneStep();
        precisionCounter++;
        delay(DELAY_ONE_STEP);
    }
}

void takePic (char* filename)
{
	int pid, status;
	if((pid = fork()) == 0)
	{
		execl("/usr/bin/raspistill", "raspistill", "-t", "200", "-w", "1024", "-h", "768", "-o", filename, (char *)NULL);
	}
	waitpid(pid, &status, 0);
}
