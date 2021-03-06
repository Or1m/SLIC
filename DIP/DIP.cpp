#include "stdafx.h"

#include <iostream>
#include <chrono>
#include <limits>

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


double Gradient(const cv::Mat img, const int x, const int y);

void ChooseInitialCentroids(const cv::Mat& img, std::vector<ColoredPoint>& centers, const int& S);

double Distance(const ColoredPoint& from, const ColoredPoint& to, const int& S, const int& m);

void AssignToNearestCentroids(cv::Mat& img, std::vector<ColoredPoint>& centroids, std::map<ColoredPoint, std::vector<ColoredPoint>>& clusters, const int& S, const int& m);

void RecalculateCentroids(std::vector<ColoredPoint>& tempCentroids, const std::vector<ColoredPoint>& centroids, std::map<ColoredPoint, std::vector<ColoredPoint>>& clusters);

void ColorateClusters(const std::vector<ColoredPoint>& centroids, std::map<ColoredPoint, std::vector<ColoredPoint>>& clusters, cv::Mat& img);

template<typename T>
bool VectorsEqual(std::vector<T>& v1, std::vector<T>& v2);

bool VectorsSimilar(std::vector<ColoredPoint>& v1, std::vector<ColoredPoint>& v2, const int threshold);


int main() {
    const cv::Mat source = cv::imread("images/bear.jpg", CV_LOAD_IMAGE_COLOR);

    if (source.empty()) {
        printf("Unable to read input file (%s, %d).", __FILE__, __LINE__);
        return EXIT_FAILURE;
    }

    const int m = 1; // balance between the color and spatial distances
    const int K = 15; // number of sectors
    const int N = (int)source.total(); // image pixel count
    const int threshold = 10; // min alowed distance between clusters to end clustering
    const int superpixelArea = N / K; // approximate size of each segment
    const int S = (int)sqrt(superpixelArea); // grid interval

    cv::Mat img = source.clone();
    std::vector<ColoredPoint> centroids, tempCentroids;
    std::map<ColoredPoint, std::vector<ColoredPoint>> clusters;

    auto start = std::chrono::high_resolution_clock::now();
    ChooseInitialCentroids(img, centroids, S);
    AssignToNearestCentroids(img, centroids, clusters, S, m);
    
    while (true) {

        RecalculateCentroids(tempCentroids, centroids, clusters);

        if (!VectorsSimilar(tempCentroids, centroids, threshold))
            centroids = tempCentroids;
        else break;

        AssignToNearestCentroids(img, centroids, clusters, S, m);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "\nElapsed time: " << elapsed.count() << " ms" << std::endl;
    
    ColorateClusters(centroids, clusters, img);
    
    cv::imshow("Source Image", source);
    cv::imshow("Processed Image", img);

    cv::waitKey(0);
    return EXIT_SUCCESS;
}


double Gradient(const cv::Mat img, const int x, const int y) {

    auto left   = img.at<cv::Vec3b>(y, x - 1);
    auto right  = img.at<cv::Vec3b>(y, x + 1);
    auto up     = img.at<cv::Vec3b>(y + 1, x);
    auto down   = img.at<cv::Vec3b>(y - 1, x);

    double xSum = 0, ySum = 0, xDiff, yDiff;
    const int cols = left.channels;
    for (int i = 0; i < cols; i++) {
        xDiff = right[i] - left[i];
        yDiff = up[i] - down[i];

        xSum += xDiff * xDiff;
        ySum += yDiff * yDiff;
    }

    xSum = sqrt(xSum);
    ySum = sqrt(ySum);

    return xSum + ySum;
};

void ChooseInitialCentroids(const cv::Mat& img, std::vector<ColoredPoint>& centers, const int& S) {
    int start = S / 2, minX, minY;
    double minGradient, grad;

    const int rows = img.rows;
    const int cols = img.cols;

    for (int y = start; y < rows; y += S) {

        for (int x = start; x < cols; x += S) {

            minGradient = std::numeric_limits<double>::max();
            minX = minY = -1;

            for (int i = x - 1; i <= x + 1; i++) {

                for (int j = y - 1; j <= y + 1; j++) {

                    grad = Gradient(img, i, j);

                    if (grad < minGradient) {
                        minGradient = grad;
                        minX = i;
                        minY = j;
                    }
                }
            }

            centers.emplace_back(img.at<cv::Vec3b>(minY, minX), minX, minY);
        }
    }
}

double Distance(const ColoredPoint& from, const ColoredPoint& to, const int& S, const int& m) {
    double drgb = 0, dxy = 0;
    int colorDiff = 0;

    const int cols = from.color.channels;
    for (int i = 0; i < cols; i++) {
        colorDiff = from.color[i] - to.color[i];
        drgb += colorDiff * (double)colorDiff;
    }

    int xDiff = from.x - to.x;
    int yDiff = from.y - to.y;

    drgb = sqrt(drgb);
    dxy = sqrt(xDiff * xDiff + yDiff * yDiff);

    return drgb + ((m / S) * dxy);
}

void AssignToNearestCentroids(cv::Mat& img, std::vector<ColoredPoint>& centroids, std::map<ColoredPoint, std::vector<ColoredPoint>>& clusters, const int& S, const int& m) {
    clusters.clear();

    const int threshold = 2 * S;
    const int rows = img.rows;
    const int cols = img.cols;

    ColoredPoint nearestCentroid, currentPoint;
    double nearestCentroidDistance, distance;

    for (int y = 0; y < rows; y++) {

        for (int x = 0; x < cols; x++) {
            nearestCentroidDistance = std::numeric_limits<double>::max();

            for (const auto& centroid : centroids) {

                if (centroid.x < x - threshold || centroid.x > x + threshold || centroid.y < y - threshold || centroid.y > y + threshold)
                    continue;

                currentPoint = ColoredPoint(img.at<cv::Vec3b>(y, x), x, y);
                distance = Distance(currentPoint, centroid, S, m);

                if (distance < nearestCentroidDistance) {
                    nearestCentroidDistance = distance;
                    nearestCentroid = centroid;
                }
            }

            clusters[nearestCentroid].push_back(currentPoint);
        }
    }
}

void RecalculateCentroids(std::vector<ColoredPoint>& tempCentroids, const std::vector<ColoredPoint>& centroids, std::map<ColoredPoint, std::vector<ColoredPoint>>& clusters) {

    tempCentroids.clear();
    
    //B, G, R
    double x = 0, y = 0, b = 0, g = 0, r = 0, coefficient;
    cv::Vec3b color;

    for (const auto& centroid : centroids) {

        for (const auto& pixel : clusters[centroid]) {
            x += pixel.x;
            y += pixel.y;

            color = pixel.color;

            b += color[0];
            g += color[1];
            r += color[2];
        }

        coefficient = 1 / (double)clusters[centroid].size();

        x *= coefficient;
        y *= coefficient;

        b *= coefficient;
        g *= coefficient;
        r *= coefficient;

        tempCentroids.emplace_back(cv::Vec3b((uchar)b, (uchar)g, (uchar)r), (int)x, (int)y);
    }
}

void ColorateClusters(const std::vector<ColoredPoint>& centroids, std::map<ColoredPoint, std::vector<ColoredPoint>>& clusters, cv::Mat& img)
{
    for (const auto& centroid : centroids)
        for (const auto& pixel : clusters[centroid])
            img.at<cv::Vec3b>(pixel.y, pixel.x) = centroid.color;
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

    int distance = 0, size = (int)v1.size();

    for (int i = 0; i < size; i++)
        distance += abs(v1[i].x - v2[i].x) + abs(v1[i].y - v2[i].y);

    std::cout << "Error: " << distance << " [" << threshold << "]" << std::endl;
    return distance < threshold;
}