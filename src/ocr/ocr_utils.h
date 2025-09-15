#pragma once

#ifndef PSH_OCR_UTILS_H_
#define PSH_OCR_UTILS_H_

#include <opencv2/opencv.hpp>
#include <QString>

namespace psh {

cv::Mat PreprocessWhiteTextForOCR(const cv::Mat& image);

QString ExtractSongNameFromImage(const cv::Mat& image);

} // namespace psh

#endif // !PSH_OCR_UTILS_H_
