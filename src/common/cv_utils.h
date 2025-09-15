#pragma once

#ifndef PSH_COMMON_CV_UTILS_H_
#define PSH_COMMON_CV_UTILS_H_

#include <opencv2/opencv.hpp>

namespace psh {

bool ColorSimilar(const cv::Vec3b& c1, const cv::Vec3b& c2, int delta = 10);

cv::Mat CreateMask(const cv::Mat& img, const cv::Scalar& color, int delta);

double CalcSimilarity(const cv::Mat& img1, const cv::Mat& img2);

} // namespace psh

#endif // !PSH_COMMON_CV_UTILS_H_