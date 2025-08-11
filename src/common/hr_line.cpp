#include "hr_line.h"

namespace psh {

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

} // namespace psh

