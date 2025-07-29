#pragma once

#ifndef I_SCREEN_H_
#define I_SCREEN_H_

#include "opencv4/opencv2/opencv.hpp"

namespace psh {

class IScreen {
public:
    virtual cv::Mat Screencap() = 0;
    virtual int GetDisplayWidth() = 0;
    virtual int GetDisplayHeight() = 0;

    virtual ~IScreen() = default;
};

}  // namespace psh

#endif  // I_SCREEN_H_