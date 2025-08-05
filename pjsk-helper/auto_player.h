#pragma once

#ifndef AUTO_PLAYER_H_
#define AUTO_PLAYER_H_

#include <array>
#include <thread>
#include <vector>
#include <optional>

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
    virtual ~AutoPlayer() = default;

    void SetTrackConfig(const TrackConfig &track_config);
    void SetPlayConfig(const PlayConfig &play_config);

    bool Start();
    void Stop();

private:
    struct NoteSample {
        NoteType type;
        int sum_hit_x;
        int64_t sum_hit_time_ms;
        int count;

        int HitX() const { return sum_hit_x / count; };
        int64_t HitTimeMs() const { return sum_hit_time_ms / count; };

        void AddSmaple(int hit_x, int64_t hit_time_ms) {
            sum_hit_x += hit_x;
            sum_hit_time_ms += hit_time_ms;
            ++count;
        }
    };

    static const int kSlideMoveDY = -200;
    static const int kSlideDurationMs = 20;
    static const int kSlideStepDelayMs = 2;

    static const int kMinNoteDX = 50;

    static const int64_t kTapDurationMs = 20;
    static const int64_t kMinNoteTimeDistMs = 100;
    static const int64_t kMinLoopWaitTimeMs = 1;

    void CheckLoop();

    void StartHoldTouch(const HrLine &hit_line);
    void AfterAutoPlayStop();

    std::atomic_bool run_flag_ = false;
    bool hold_touch_started_ = false;
    std::thread check_worker_;

    IScreen &screen_;
    ITouch &tap_touch_;
    ITouch &hold_touch_;
    ITouch &slide_touch_;
    NoteTimeEstimator &note_estimator_;

    TrackConfig tc_;
    PlayConfig pc_;

    std::vector<NoteSample> notes_;
};

} // namespace psh

#endif // AUTO_PLAYER_H_