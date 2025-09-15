#pragma once

#ifndef PSH_SCREEN_I_SCREEN_H_
#define PSH_SCREEN_I_SCREEN_H_

#include <atomic>
#include <mutex>

#include <opencv2/opencv.hpp>

namespace psh {

struct Frame {
    cv::Mat img;
    int64_t capture_time_ms;

    Frame() : capture_time_ms(0) {}
    Frame(cv::Mat image, int64_t time_ms)
        : img(std::move(image)), capture_time_ms(time_ms) {}
    Frame(const Frame&) = default;
    Frame(Frame&&) = default;
    Frame& operator=(const Frame&) = default;
    Frame& operator=(Frame&&) = default;

    ~Frame() = default;
};

class IScreen {
public:
    virtual ~IScreen() = default;

    Frame GetFrame();
    Frame GetFrame(int max_interval_ms);

    virtual cv::Mat Capture() = 0;
    virtual int GetDisplayWidth() = 0;
    virtual int GetDisplayHeight() = 0;

private:
    Frame frame_;

    mutable std::mutex frame_mutex_;
};

} // namespace psh

#endif // !PSH_SCREEN_I_SCREEN_H_