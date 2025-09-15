#include "common/hr_line.h"

namespace psh {

bool HrLine::operator==(const HrLine &other) const {
    return pos == other.pos && length == other.length;
}

bool HrLine::operator!=(const HrLine &other) const { return !(*this == other); }

std::vector<HrLine> HrLine::Split(int n) const {
    std::vector<HrLine> lines(n);
    int len = length / n;
    for (int i = 0; i < n; ++i) {
        lines[i].length = len;
        lines[i].pos.x = pos.x + length * i / n;
        lines[i].pos.y = pos.y;
    }
    return lines;
}

cv::Point HrLine::PosOf(double t) const {
    return pos + cv::Point{static_cast<int>(length * t), 0};
}

cv::Point HrLine::PosOf(const HrLine &other, cv::Point p) const {
    return pos + cv::Point{(length * (p.x - other.pos.x) / other.length), 0};
}

} // namespace psh