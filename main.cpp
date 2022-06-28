#include <iostream>
#include <opencv2/opencv.hpp>

int main(int, char**) {
    std::cout << "Hello, world!\n";

    auto filename = "C:/Users/Conrad/source/repos/highfleet_aimer/april.png";
    auto image = cv::imread(filename);

    cv::imshow("Image1", image);
    cv::waitKey(0);
}
