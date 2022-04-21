#include "stdafx.h"
#include <iostream>
#include <limits.h>

struct ColoredPoint {
    cv::Vec3b color;
    int x, y;

    ColoredPoint(cv::Vec3b color = cv::Vec3b(0, 0, 0), int x = 0, int y = 0) {
        this->x = x;
        this->y = y;

        this->color = color;
    }

    bool const operator==(const ColoredPoint& p) const {
        return y == p.y && x == p.x;
    }

    bool const operator<(const ColoredPoint& p) const {
        return y < p.y || (y == p.y && x < p.x);
    }
};


double Gradient(cv::Mat img, int x, int y);

void ChooseInitialCentroids(cv::Mat& img, std::vector<ColoredPoint>& centers, const int& S);

double Distance(const ColoredPoint& from, const ColoredPoint& to, const int& S, const int& m);

void AssignToNearestCentroids(cv::Mat& img, std::vector<ColoredPoint>& centroids, std::map<ColoredPoint, std::vector<ColoredPoint>>& clusters, const int& S, const int& m);

void RecalculateCentroids(cv::Mat& img, std::vector<ColoredPoint>& tempCentroids, std::vector<ColoredPoint>& centroids, std::map<ColoredPoint, std::vector<ColoredPoint>>& clusters);

template<typename T>
bool VectorsEqual(std::vector<T>& v1, std::vector<T>& v2);

bool VectorsSimilar(std::vector<ColoredPoint>& v1, std::vector<ColoredPoint>& v2, const int threshold);


int main() {
    const cv::String fileName = "images/polar.jpg";
    cv::Mat img = cv::imread(fileName, CV_LOAD_IMAGE_COLOR);
    cv::Mat source = img.clone();

    if (img.empty()) {
        printf("Unable to read input file (%s, %d).", __FILE__, __LINE__);
        return EXIT_FAILURE;
    }

    const int m = 1;
    const int K = 15;
    const int N = img.total();
    const int threshold = 10;
    const int superpixelArea = N / K;
    const int S = sqrt(superpixelArea);

    std::vector<ColoredPoint> centroids, tempCentroids;
    std::map<ColoredPoint, std::vector<ColoredPoint>> clusters;

    ChooseInitialCentroids(img, centroids, S);
    AssignToNearestCentroids(img, centroids, clusters, S, m);

    while (true) {

        RecalculateCentroids(img, tempCentroids, centroids, clusters);

        /*if (!VectorsEqual(tempCentroids, centroids)) 
            centroids = tempCentroids;
        else break;*/
        if (!VectorsSimilar(tempCentroids, centroids, threshold))
            centroids = tempCentroids;
        else break;

        AssignToNearestCentroids(img, centroids, clusters, S, m);
    }

    for (auto& centroid : centroids) {
        #pragma region DebugOutput
        /*int x = centroid.x;
        int y = centroid.y;

        img.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 255, 0);

        img.at<cv::Vec3b>(y + 1, x) = cv::Vec3b(0, 255, 0);
        img.at<cv::Vec3b>(y, x + 1) = cv::Vec3b(0, 255, 0);
        img.at<cv::Vec3b>(y - 1, x) = cv::Vec3b(0, 255, 0);
        img.at<cv::Vec3b>(y, x - 1) = cv::Vec3b(0, 255, 0);

        img.at<cv::Vec3b>(y + 1, x + 1) = cv::Vec3b(0, 255, 0);
        img.at<cv::Vec3b>(y - 1, x + 1) = cv::Vec3b(0, 255, 0);
        img.at<cv::Vec3b>(y + 1, x - 1) = cv::Vec3b(0, 255, 0);
        img.at<cv::Vec3b>(y - 1, x - 1) = cv::Vec3b(0, 255, 0);*/
        #pragma endregion

        for (auto& assignedpixel : clusters[centroid]) {
            int x = assignedpixel.x;
            int y = assignedpixel.y;

            img.at<cv::Vec3b>(y, x) = centroid.color;
        }
    }

    cv::imshow("Source Image", source);
    cv::imshow("Processed Image", img);

    cv::waitKey(0);
    return EXIT_SUCCESS;
}


double Gradient(cv::Mat img, int x, int y) {

    auto left   = img.at<cv::Vec3b>(y, x - 1);
    auto right  = img.at<cv::Vec3b>(y, x + 1);
    auto up     = img.at<cv::Vec3b>(y + 1, x);
    auto down   = img.at<cv::Vec3b>(y - 1, x);

    double xSum = 0, ySum = 0;

    for (int i = 0; i < left.channels; i++) {
        xSum += pow(right[i] - left[i], 2);
        ySum += pow(up[i] - down[i], 2);
    }

    xSum = sqrt(xSum);
    ySum = sqrt(ySum);

    return xSum + ySum;
};

void ChooseInitialCentroids(cv::Mat& img, std::vector<ColoredPoint>& centers, const int& S) {
    int currentX = S / 2;
    int currentY = S / 2;

    //B, G, R
    for (int y = currentY; y < img.rows; y += S) {

        for (int x = currentX; x < img.cols; x += S) {

            double minGradient = std::numeric_limits<double>::max();
            int minX = -1, minY = -1;

            for (int i = x - 1; i <= x + 1; i++) {
                for (int j = y - 1; j <= y + 1; j++) {

                    double grad = Gradient(img, i, j);
                    if (grad < minGradient) {
                        minGradient = grad;
                        minX = i;
                        minY = j;
                    }
                }
            }

            #pragma region DebugOutput
            /*img.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 255);

            img.at<cv::Vec3b>(y + 1, x) = cv::Vec3b(0, 0, 255);
            img.at<cv::Vec3b>(y, x + 1) = cv::Vec3b(0, 0, 255);
            img.at<cv::Vec3b>(y - 1, x) = cv::Vec3b(0, 0, 255);
            img.at<cv::Vec3b>(y, x - 1) = cv::Vec3b(0, 0, 255);

            img.at<cv::Vec3b>(y + 1, x + 1) = cv::Vec3b(0, 0, 255);
            img.at<cv::Vec3b>(y - 1, x + 1) = cv::Vec3b(0, 0, 255);
            img.at<cv::Vec3b>(y + 1, x - 1) = cv::Vec3b(0, 0, 255);
            img.at<cv::Vec3b>(y - 1, x - 1) = cv::Vec3b(0, 0, 255);


            img.at<cv::Vec3b>(minY, minX) = cv::Vec3b(0, 255, 0);

            img.at<cv::Vec3b>(minY + 1, minX) = cv::Vec3b(0, 255, 0);
            img.at<cv::Vec3b>(minY, minX + 1) = cv::Vec3b(0, 255, 0);
            img.at<cv::Vec3b>(minY - 1, minX) = cv::Vec3b(0, 255, 0);
            img.at<cv::Vec3b>(minY, minX - 1) = cv::Vec3b(0, 255, 0);

            img.at<cv::Vec3b>(minY + 1, minX + 1) = cv::Vec3b(0, 255, 0);
            img.at<cv::Vec3b>(minY - 1, minX + 1) = cv::Vec3b(0, 255, 0);
            img.at<cv::Vec3b>(minY + 1, minX - 1) = cv::Vec3b(0, 255, 0);
            img.at<cv::Vec3b>(minY - 1, minX - 1) = cv::Vec3b(0, 255, 0);*/
            #pragma endregion

            centers.push_back({ img.at<cv::Vec3b>(minY, minX), minX, minY });
        }
    }
}

double Distance(const ColoredPoint& from, const ColoredPoint& to, const int& S, const int& m) {
    double drgb = 0, dxy;

    for (int i = 0; i < from.color.channels; i++) {
        drgb += pow(from.color[i] - to.color[i], 2);
    }

    drgb = sqrt(drgb);
    dxy = sqrt(pow(from.x - to.x, 2) + pow(from.y - to.y, 2));

    return drgb + ((m / S) * dxy);
}

void AssignToNearestCentroids(cv::Mat& img, std::vector<ColoredPoint>& centroids, std::map<ColoredPoint, std::vector<ColoredPoint>>& clusters, const int& S, const int& m) {
    clusters.clear();
    int threshold = 2 * S;

    ColoredPoint nearestCentroid;
    double nearestCentroidDistance;

    for (int y = 0; y < img.rows; y++) {

        for (int x = 0; x < img.cols; x++) {
            nearestCentroidDistance = std::numeric_limits<double>::max();

            for (auto& centroid : centroids) {

                if (centroid.x < x - threshold || centroid.x > x + threshold || centroid.y < y - threshold || centroid.y > y + threshold)
                    continue;

                double distance = Distance({ img.at<cv::Vec3b>(y, x), x, y }, centroid, S, m);

                if (distance < nearestCentroidDistance) {
                    nearestCentroidDistance = distance;
                    nearestCentroid = centroid;
                }
            }

            clusters[nearestCentroid].push_back({ img.at<cv::Vec3b>(y, x), x, y });
        }
    }
}

void RecalculateCentroids(cv::Mat& img, std::vector<ColoredPoint>& tempCentroids, std::vector<ColoredPoint>& centroids, std::map<ColoredPoint, std::vector<ColoredPoint>>& clusters) {

    tempCentroids.clear();

    for (auto& centroid : centroids) {
        double x = 0, y = 0, b = 0, g = 0, r = 0;

        for (auto& assignedpixels : clusters[centroid]) {
            x += assignedpixels.x;
            y += assignedpixels.y;

            b += assignedpixels.color[0];
            g += assignedpixels.color[1];
            r += assignedpixels.color[2];
        }

        x /= clusters[centroid].size();
        y /= clusters[centroid].size();

        b /= clusters[centroid].size();
        g /= clusters[centroid].size();
        r /= clusters[centroid].size();

        tempCentroids.push_back({ cv::Vec3b(b, g, r), (int)x, (int)y });
    }
}

template<typename T>
bool VectorsEqual(std::vector<T>& v1, std::vector<T>& v2) {

    std::sort(v1.begin(), v1.end());
    std::sort(v2.begin(), v2.end());

    return v1 == v2;
}
bool VectorsSimilar(std::vector<ColoredPoint>& v1, std::vector<ColoredPoint>& v2, const int threshold) {

    std::sort(v1.begin(), v1.end());
    std::sort(v2.begin(), v2.end());

    int distance = 0;

    for (int i = 0; i < v1.size(); i++)
        distance += abs(v1[i].x - v2[i].x) + abs(v1[i].y - v2[i].y);

    std::cout << "Error: " << distance << " [" << threshold << "]" << std::endl;
    return distance < threshold;
}