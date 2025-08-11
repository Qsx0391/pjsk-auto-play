#pragma once

#ifndef ENENTS_H_
#define ENENTS_H_

#include <vector>
#include <unordered_map>

#include <opencv2/opencv.hpp>

#include "images.h"
#include "i_touch.h"

namespace psh {

class Event {
public:
    Event(const std::vector<std::vector<Image*>>& check_imgs,
          const std::unordered_map<std::string, cv::Point>& btn_set)
        : check_imgs_(check_imgs), btn_set_(btn_set) {}

    bool Check(const cv::Mat& frame, double threshold) const;
    void ClickBtn(ITouch& touch, const std::string& btn_name,
                  int delay_ms = 0) const;

private:
    const std::vector<std::vector<Image*>> check_imgs_;
    const std::unordered_map<std::string, cv::Point> btn_set_;
};

class Events {
public:
    static const Event& SongStart() {
        static const Event event(
            {{&Images::SongStart1(), &Images::SongStart2()}}, {});
        return event;
    }

    static const Event& SongPlaying() {
        static const Event event({{&Images::SongPlaying1()}}, {});
        return event;
    }

    static const Event& MultiPlayBegin() {
        static const Event event({{&Images::MultiPlayBegin1()}},
                                 {{"Next", cv::Point{907, 536}}});
        return event;
    }

    static const Event& MultiPlaySongChoose() {
        static const Event event(
            std::vector<std::vector<Image*>>{{&Images::MultiPlaySongChoose1()},
                                             {&Images::MultiPlaySongChoose2()}},
            std::unordered_map<std::string, cv::Point>{
                {"Easy", cv::Point{866, 468}},
                {"Normal", cv::Point{947, 474}},
                {"Hard", cv::Point{1028, 480}},
                {"Expert", cv::Point{1109, 486}},
                {"Master", cv::Point{1191, 492}},
                {"Choose", cv::Point{1026, 586}}});
        return event;
    }

private:
    Events() = delete;
};

} // namespace psh

#endif // !ENENTS_H_
