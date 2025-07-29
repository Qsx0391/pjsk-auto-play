#pragma once

#ifndef FRAMEWORK_H_
#define FRAMEWORK_H_

#include <vector>
#include <memory>

namespace psh {

struct Point {
    int x, y;

    Point() = default;
    Point(int x, int y) : x(x), y(y) {}
    Point(const Point&) = default;
    Point& operator=(const Point&) = default;
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const Point& other) const { return !(*this == other); }
    Point operator+(const Point& other) const {
        return {x + other.x, y + other.y};
    }
    Point operator-(const Point& other) const {
        return {x - other.x, y - other.y};
    }
    Point& operator+=(const Point& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    Point& operator-=(const Point& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
};

struct Line {
    Point pos;
    int length;

    Line() = default;
    Line(const Point& pos, int length) : pos(pos), length(length){};
    Line(const Line&) = default;
    Line& operator=(const Line&) = default;
    bool operator==(const Line& other) const {
        return pos == other.pos && length == other.length;
    }
    bool operator!=(const Line& other) const { return !(*this == other); }

    std::vector<Line> Split(int n) const;
    Point PosOf(int dx) const;
    Point PosOf(const Line& other, int other_dx) const;
    Point Left() const;
    Point Right() const;
};

} // namespace psh

#endif // FRAMEWORK_H_