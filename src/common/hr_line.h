#pragma once

#ifndef PSH_COMMON_HR_LINE_H_
#define PSH_COMMON_HR_LINE_H_

#include <opencv2/opencv.hpp>

namespace psh {

struct HrLine {
    HrLine() = default;
    HrLine(const cv::Point &pos, int length) : pos(pos), length(length){};
    HrLine(const HrLine &) = default;

    HrLine &operator=(const HrLine &) = default;
    bool operator==(const HrLine &other) const;
    bool operator!=(const HrLine &other) const;

    std::vector<HrLine> Split(int n) const;
    cv::Point Left() const { return pos; };
    cv::Point Right() const { return pos + cv::Point{length, 0}; }
    cv::Point PosOf(double t) const;
    cv::Point PosOf(const HrLine &other, cv::Point p) const;

    cv::Point pos;
    int length = 0;
};

} // namespace psh

#endif // !PSH_COMMON_HR_LINE_H_
