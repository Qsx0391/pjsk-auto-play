#pragma once

#ifndef HR_LINE_H_
#define HR_LINE_H_

#include <opencv2/opencv.hpp>

namespace psh {

struct HrLine {
    cv::Point pos;
    int length;

    HrLine() = default;
    HrLine(const cv::Point &pos, int length) : pos(pos), length(length){};
    HrLine(const HrLine &) = default;
    HrLine &operator=(const HrLine &) = default;
    bool operator==(const HrLine &other) const {
        return pos == other.pos && length == other.length;
    }
    bool operator!=(const HrLine &other) const { return !(*this == other); }

    std::vector<HrLine> Split(int n) const;
    cv::Point Left() const { return pos; };
    cv::Point Right() const { return pos + cv::Point{length, 0}; }
    cv::Point PosOf(double t) const {
        return pos + cv::Point{static_cast<int>(length * t), 0};
    }
    cv::Point PosOf(const HrLine &other, cv::Point p) const {
        return pos +
               cv::Point{(length * (p.x - other.pos.x) / other.length), 0};
    }
};

}  // namespace psh

#endif // !HR_LINE_H_
