#pragma once

#ifndef MULTI_PLAY_CONTROLLER_H_
#define MULTI_PLAY_CONTROLLER_H_

#include <atomic>
#include <thread>
#include <functional>

#include <opencv2/opencv.hpp>

#include "i_screen.h"
#include "i_touch.h"
#include "auto_player.h"

namespace psh {

enum class SongStatus {
    kUnplayed = 0,
    kClear = 1,
    kFullCombo = 2,
    kAllPerfect = 3
};

enum class SongDiffculty {
    kEasy = 0,
    kNormal = 1,
    kHard = 2,
    kExpert = 3,
    kMaster = 4,
    kAppend = 5
};

class MultiPlayController {
public:
    enum class PlayMode {
        kFixedDifficulty,
        kUnclearFirst,
        kUnfullComboFirst,
        kUnallPerfectFirst
    };

    MultiPlayController(IScreen& screen, ITouch& touch,
                        AutoPlayer& auto_player);
    virtual ~MultiPlayController();

    void SetStartCallback(std::function<void()> callback) {
        if (callback) {
            start_callback_ = std::move(callback);
        }
    }

    void SetStopCallback(std::function<void()> callback) {
        if (callback) {
            stop_callback_ = std::move(callback);
        }
    }

    bool Start(PlayMode mode, SongDiffculty max_diff);
    void Stop();
    bool IsRunning();

private:
    inline static std::vector<cv::Point> kSongStatusCheckPoints = {
        cv::Point{920, 334}, cv::Point{946, 335},  cv::Point{972, 336},
        cv::Point{998, 337}, cv::Point{1024, 338}, cv::Point{1059, 338},
    };

    inline static std::vector<cv::Vec3b> kSongStatusColors = {
        {102, 68, 68},   // Unplayed
        {113, 226, 254}, // Clear
        {251, 170, 255}, // Full Combo
        {206, 204, 19}   // All Perfect
    };

    void WorkerLoop(PlayMode mode, SongDiffculty max_diff);
    std::vector<SongStatus> FindSongStatus(const cv::Mat& img);

    IScreen& screen_;
    ITouch& touch_;
    AutoPlayer& auto_player_;

    std::atomic_bool run_flag_ = false;
    std::thread worker_;

    std::function<void()> start_callback_ = [] {};
    std::function<void()> stop_callback_ = [] {};
};

} // namespace psh

#endif // !MULTI_PLAY_CONTROLLER_H_
