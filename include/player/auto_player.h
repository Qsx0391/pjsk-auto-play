#pragma once

#ifndef AUTO_PLAYER_H_
#define AUTO_PLAYER_H_

#include <array>
#include <thread>
#include <vector>
#include <optional>
#include <functional>

#include <opencv2/opencv.hpp>

#include "i_touch.h"
#include "i_screen.h"
#include "hr_line.h"
#include "ring_buffer.hpp"
#include "range_list.hpp"
#include "auto_play_config.h"
#include "track_controller.h"

namespace psh {

class AutoPlayer {
public:
    AutoPlayer(ITouch &tap_touch, ITouch &hold_touch, ITouch &slide_touch,
               IScreen &screen, NoteTimeEstimator &note_estimator,
               const TrackConfig &track_config, const PlayConfig &play_config);
    virtual ~AutoPlayer();

    void SetTrackConfig(const TrackConfig &track_config);
    void SetPlayConfig(const PlayConfig &play_config);

    void SetPlayStartCallback(std::function<void()> callback) {
        if (callback) {
            play_start_callback_ = std::move(callback);
        }
    }

    void SetPlayStopCallback(std::function<void()> callback) {
        if (callback) {
            play_stop_callback_ = std::move(callback);
        }
    }

    void SetCheckStartCallback(std::function<void()> callback) {
        if (callback) {
            check_start_callback_ = std::move(callback);
        }
    }

    void SetCheckStopCallback(std::function<void()> callback) {
        if (callback) {
            check_stop_callback_ = std::move(callback);
        }
    }

    void SetImgShowOffCallback(std::function<void()> callback) {
		if (callback) {
			img_show_off_callback_ = std::move(callback);
		}
	}

    void SetImgShowFlag(bool flag) {
        img_show_flag_.store(flag, std::memory_order_release);
    }

    bool Start();
    void Stop();

    bool AutoCheckStart(int loop_cnt = -1);
    void AutoCheckStop();

private:
    struct NoteSample {
        NoteType type;
        int hit_x;
        int64_t hit_time_ms;
    };

    static const int kSlideMoveDY = -200;
    static const int kSlideDurationMs = 20;
    static const int kSlideStepDelayMs = 1;

    static const int kMinNoteDX = 50;

    static const int64_t kTapDurationMs = 20;
    static const int64_t kMinNoteTimeDistMs = 100;
    static const int64_t kMinLoopWaitTimeMs = 1;

    static const int64_t kPlayingCheckLoopDelayMs = 5000;
    static const int64_t kCheckLoopDelayMs = 1000;

    void PlayLoop();
    void CheckLoop(int loop_cnt);

    void StartHoldTouch(const HrLine &hit_line);
    void AfterAutoPlayStop();

    std::atomic_bool img_show_flag_ = false;
    std::atomic_bool play_run_flag_ = false;
    std::atomic_bool check_run_flag_ = false;
    bool hold_touch_started_ = false;
    std::thread play_worker_;
    std::thread check_worker_;

    IScreen &screen_;
    ITouch &tap_touch_;
    ITouch &hold_touch_;
    ITouch &slide_touch_;
    NoteTimeEstimator &note_estimator_;

    TrackConfig tc_;
    PlayConfig pc_;

    std::vector<NoteSample> notes_;

    std::function<void()> play_start_callback_ = [] {};
    std::function<void()> play_stop_callback_ = [] {};
    std::function<void()> check_start_callback_ = [] {};
    std::function<void()> check_stop_callback_ = [] {};
    std::function<void()> img_show_off_callback_ = [] {};
};

} // namespace psh

#endif // AUTO_PLAYER_H_