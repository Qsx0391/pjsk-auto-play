#pragma once

#ifndef PSH_STORY_STORY_AUTO_READER_H_
#define PSH_STORY_STORY_AUTO_READER_H_

#include <atomic>
#include <thread>

#include <opencv2/opencv.hpp>
#include <QObject>

#include "touch/i_touch.h"

namespace psh {

class StoryAutoReader : public QObject {
public:
    Q_OBJECT

public:
    StoryAutoReader(TouchController &touch);
    virtual ~StoryAutoReader();

    bool Start();
    void Stop();

signals:
    void stopped();

private:
    void MainLoop();

    inline static const cv::Point kAutoClickPoint{400, 400};
    static constexpr int click_delay_ms = 100;

    std::atomic_bool run_flag_{false};
    std::thread worker_;

    TouchController& touch_;
};

} // namespace psh

#endif // !PSH_STORY_STORY_AUTO_READER_H_
