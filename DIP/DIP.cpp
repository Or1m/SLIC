#include "stdafx.h"
#include <iostream>

using namespace std;
using namespace cv;


struct SegmentCenter {
    uchar R, G, B;
    int x, y;
};

int main() {
    const String fileName = "images/polar.jpg";
    Mat img = imread(fileName, CV_LOAD_IMAGE_COLOR);

    if (img.empty()) {
        printf("Unable to read input file (%s, %d).", __FILE__, __LINE__);
        return EXIT_FAILURE;
    }

    const int K = 15;
    const int N = img.total();
    const int superpixelArea = N / K;
    const int S = sqrt(superpixelArea);

    const int cols = 5;
    const int rows = 3;

    int rowIterator = 1, colIterator = 1;

    int currentX = S / 2;
    int currentY = S / 2;

    //B, R, G
    for (int y = currentY; y < img.rows; y += S) {

        for (int x = currentX; x < img.cols; x += S) {
            img.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 255);

            img.at<cv::Vec3b>(y + 1, x) = cv::Vec3b(0, 0, 255);
            img.at<cv::Vec3b>(y, x + 1) = cv::Vec3b(0, 0, 255);
            img.at<cv::Vec3b>(y - 1, x) = cv::Vec3b(0, 0, 255);
            img.at<cv::Vec3b>(y, x - 1) = cv::Vec3b(0, 0, 255);

            img.at<cv::Vec3b>(y + 1, x + 1) = cv::Vec3b(0, 0, 255);
            img.at<cv::Vec3b>(y - 1, x + 1) = cv::Vec3b(0, 0, 255);
            img.at<cv::Vec3b>(y + 1, x - 1) = cv::Vec3b(0, 0, 255);
            img.at<cv::Vec3b>(y - 1, x - 1) = cv::Vec3b(0, 0, 255);
        }
    }

    cv::imshow("Source Image", img);

    cv::waitKey(0);
    return EXIT_SUCCESS;
}
