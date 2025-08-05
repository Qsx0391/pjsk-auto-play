#include "auto_player.h"

#include <spdlog/spdlog.h>

#include "psh_utils.hpp"

namespace psh {

AutoPlayer::AutoPlayer(ITouch& tap_touch, ITouch& hold_touch,
                       ITouch& slide_touch, IScreen& screen,
                       NoteTimeEstimator& note_estimator,
                       const TrackConfig& track_config,
                       const PlayConfig& play_config)
    : tap_touch_(tap_touch),
      hold_touch_(hold_touch),
      slide_touch_(slide_touch),
      screen_(screen),
      note_estimator_(note_estimator) {
    SetTrackConfig(track_config);
    SetPlayConfig(play_config);
}

void AutoPlayer::SetTrackConfig(const TrackConfig& track_config) {
    tc_ = track_config;
}

void AutoPlayer::SetPlayConfig(const PlayConfig& play_config) {
    if (play_config.color_delta < 0 || play_config.tap_duration_ms < 0 ||
        play_config.check_loop_delay_ms < 0 || play_config.hit_delay_ms < 0) {
        throw std::invalid_argument("Invalid play config values");
    }
    pc_ = play_config;
}

bool AutoPlayer::Start() {
    if (run_flag_.load(std::memory_order_acquire) || check_worker_.joinable()) {
        spdlog::warn("Auto play already running");
        return false;
    }
    run_flag_.store(true, std::memory_order_release);
    check_worker_ = std::thread([&] { CheckLoop(); });
    spdlog::info("Start auto play");
    return true;
}

void AutoPlayer::Stop() {
    run_flag_.store(false, std::memory_order_release);
    if (check_worker_.joinable()) {
        check_worker_.join();
    }
    AfterAutoPlayStop();
    spdlog::info("Stop auto play");
}

void AutoPlayer::StartHoldTouch(const HrLine& hit_line) {
    int n = pc_.hold_cnt;
    auto hold_ranges = hit_line.Split(n);
    for (int i = 0; i < n; ++i) {
        int slot_index = hold_touch_.TouchDown(hold_ranges[i].PosOf(0.5));
        if (slot_index == -1) {
            spdlog::warn("No available slots for hold touch");
        }
    }
}

void AutoPlayer::AfterAutoPlayStop() {
    tap_touch_.TouchUpAll();
    hold_touch_.TouchUpAll();
    slide_touch_.TouchUpAll();
}

void AutoPlayer::CheckLoop() {
    NoteFinder note_finder(pc_.color_delta);
    TrackController track(tc_, note_finder, note_estimator_);

    int64_t check_time_ms = GetCurrentTimeMs();
    bool is_hold_started = false;

    while (run_flag_) {
        int64_t cur_time_ms = GetCurrentTimeMs();

        if (cur_time_ms >= check_time_ms) {
            check_time_ms += pc_.check_loop_delay_ms;

            for (int i = 0; i < notes_.size();) {
                int64_t hit_time_ms = notes_[i].HitTimeMs() + pc_.hit_delay_ms;
                if (track.IsOutSideCheckRange(cur_time_ms, hit_time_ms)) {
                    cv::Point pos{notes_[i].HitX(), tc_.hit_line_y};
                    int64_t delay_ms = hit_time_ms - cur_time_ms;
                    if (notes_[i].type == NoteType::Tap ||
                        notes_[i].type == NoteType::Hold) {
                        tap_touch_.TouchTapAsyn(pos, kTapDurationMs, delay_ms);
                    } else {
                        slide_touch_.TouchSlideAsyn(
                            pos, pos + cv::Point{0, kSlideMoveDY},
                            kSlideDurationMs, kSlideStepDelayMs, 1.0, 1.0,
                            delay_ms);
                    }
                    std::swap(notes_[i], notes_.back());
                    notes_.pop_back();
                } else {
                    ++i;
                }
            }

            if (!is_hold_started && !notes_.empty()) {
                StartHoldTouch(track.GetHitLine());
                is_hold_started = true;
            }

            cv::Mat capture_img = screen_.Screencap();
            cur_time_ms = GetCurrentTimeMs();
            auto new_notes = track.FindNotes(cur_time_ms, capture_img);
            for (const auto& new_note : new_notes) {
                bool is_new = true;
                for (const auto& note : notes_) {
                    if (note.type == new_note.type &&
                        std::abs(note.HitX() - new_note.hit_x) < kMinNoteDX &&
                        std::abs(note.HitTimeMs() - new_note.hit_time_ms) <
                            kMinNoteTimeDistMs) {
                        is_new = false;
                        break;
                    }
                }

                if (is_new) {
                    /*
                    spdlog::debug(
                        "New note found: type={}, hit_x={}, hit_time_ms={}",
                        NoteTypeToString(new_note.type).toUtf8().constData(),
                        new_note.hit_x, new_note.hit_time_ms);
                    */
                    notes_.push_back(NoteSample{new_note.type, new_note.hit_x,
                                                new_note.hit_time_ms, 1});
                }
            }
        }

        auto wait_time =
            std::max(kMinLoopWaitTimeMs, check_time_ms - cur_time_ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
    }
}

} // namespace psh