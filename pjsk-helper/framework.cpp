#include "framework.h"

namespace psh {

std::vector<Line> Line::Split(int n) const {
    std::vector<Line> lines(n);
    int len = length / n;
    for (int i = 0; i < n; ++i) {
        lines[i].length = len;
        lines[i].pos.x = pos.x + length * i / n;
        lines[i].pos.y = pos.y;
    }
    return lines;
}

Point Line::PosOf(int dx) const { return {pos.x + dx, pos.y}; }

Point Line::PosOf(const Line& other, int other_dx) const {
    return {pos.x + other_dx * length / other.length, pos.y};
}

Point Line::Right() const { return PosOf(length); }

Point Line::Left() const { return PosOf(0); }

} // namespace psh