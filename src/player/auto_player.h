#pragma once

#ifndef PSH_PLAYER_AUTO_PLAYER_H_
#define PSH_PLAYER_AUTO_PLAYER_H_

#include <atomic>
#include <array>
#include <thread>
#include <vector>
#include <optional>
#include <functional>
#include <future>

#include <opencv2/opencv.hpp>
#include <MikuMikuWorld/SUS.h>
#include <QObject>

#include "common/hr_line.h"
#include "touch/i_touch.h"
#include "screen/i_screen.h"
#include "screen/events.h"
#include "player/auto_play_constant.h"
#include "player/note_time_estimator.h"
#include "player/note_sample.h"

namespace psh {

// clang-format off
struct PlayConfig {
    int hold_cnt             = 6;
    int check_loop_delay_ms  = 50;
    int cv_hit_delay_ms      = -40;
    int sus_hit_delay_ms     = -30;
    SpeedFactor speed_factor = SpeedFactor::kSpeed10x;
    SongDifficulty max_diff  = SongDifficulty::kHard;
    bool sus_mode            = false;
    bool auto_select         = false;
};
// clang-format on

enum class PlayMode { kOnce, kSolo, kMulti };

class AutoPlayer : public QObject {
    Q_OBJECT
public:
    AutoPlayer(TouchController &touch, IScreen &screen,
               const TrackConfig &track_config, const PlayConfig &play_config,
               QObject *parent = nullptr);
    virtual ~AutoPlayer();

    void SetTrackConfig(const TrackConfig &tc) { tc_ = tc; }
    void SetPlayConfig(const PlayConfig &pc) { pc_ = pc; }

    bool Start();
    void Stop();

    void SetPlayMode(PlayMode mode) {
        play_mode_.store(mode, std::memory_order_release);
    }

signals:
    void playStopped();

private:
    void MainLoop();
    void SimpleCvPlayLoop(const Event &event);
    void SusPlayLoop(const MikuMikuWorld::SUS &sus, const Event &event);

    void StartHoldTouch(TouchExecutor &executor, HrLine hit_line) const;
    void ExecuteTouch(TouchExecutor &executor, std::deque<NoteSample> &s) const;

    void UpdateFrame();

    int CalcMinSampleCount(NoteTimeEstimator &estimator, double factor) const;

    std::thread play_worker_;
    std::atomic_bool run_flag_{false};
    std::atomic<PlayMode> play_mode_{PlayMode::kOnce};

    Frame frame_;
    Frame prev_frame_;

    IScreen &screen_;
    TouchController &touch_;

    TrackConfig tc_;
    PlayConfig pc_;
};

} // namespace psh

#endif // !PSH_PLAYER_AUTO_PLAYER_H_