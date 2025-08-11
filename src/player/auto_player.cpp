#include "auto_player.h"

#include "spdlog/spdlog.h"

#include "psh_utils.hpp"
#include "events.h"

namespace psh {

AutoPlayer::AutoPlayer(ITouch& tap_touch, ITouch& hold_touch,
                       ITouch& slide_touch, IScreen& screen,
                       NoteTimeEstimator& note_estimator,
                       const TrackConfig& track_config,
                       const PlayConfig& play_config)
    : screen_(screen),
      tap_touch_(tap_touch),
      hold_touch_(hold_touch),
      slide_touch_(slide_touch),
      note_estimator_(note_estimator) {
    SetTrackConfig(track_config);
    SetPlayConfig(play_config);
}

AutoPlayer::~AutoPlayer() {
    Stop();
    AutoCheckStop();
}

void AutoPlayer::SetTrackConfig(const TrackConfig& track_config) {
    tc_ = track_config;
}

void AutoPlayer::SetPlayConfig(const PlayConfig& play_config) {
    if (play_config.color_delta < 0 || play_config.tap_duration_ms < 0 ||
        play_config.check_loop_delay_ms < 0) {
        throw std::invalid_argument("Invalid play config values");
    }
    pc_ = play_config;
}

bool AutoPlayer::Start() {
    if (play_run_flag_.load(std::memory_order_acquire) ||
        play_worker_.joinable()) {
        spdlog::warn("Auto play already running");
        return false;
    }
    play_run_flag_.store(true, std::memory_order_release);
    play_worker_ = std::thread(&AutoPlayer::PlayLoop, this);
    return true;
}

void AutoPlayer::Stop() {
    play_run_flag_.store(false, std::memory_order_release);
    if (play_worker_.joinable()) {
        play_worker_.join();
    }
}

bool AutoPlayer::AutoCheckStart(int loop_cnt) {
    if (check_run_flag_.load(std::memory_order_acquire)) {
        spdlog::warn("Auto check already running");
        return false;
    }
    if (check_worker_.joinable()) {
        check_worker_.join();
    }
    check_run_flag_.store(true, std::memory_order_release);
    check_worker_ = std::thread(&AutoPlayer::CheckLoop, this, loop_cnt);
    return true;
}

void AutoPlayer::AutoCheckStop() {
    check_run_flag_.store(false, std::memory_order_release);
    if (check_worker_.joinable()) {
        check_worker_.join();
    }
}

void AutoPlayer::StartHoldTouch(const HrLine& hit_line) {
    int n = pc_.hold_cnt;
    auto hold_ranges = hit_line.Split(n);
    for (int i = 0; i < n; ++i) {
        cv::Point pos = hold_ranges[i].PosOf(0.5);
        int slot_index = hold_touch_.TouchDown(pos);
        if (slot_index == -1) {
            spdlog::warn("No available slots for hold touch");
        } else {
            hold_touch_.TouchDown(slot_index, pos);
        }
    }
}

void AutoPlayer::AfterAutoPlayStop() {
    tap_touch_.TouchUpAll();
    hold_touch_.TouchUpAll();
    slide_touch_.TouchUpAll();
}

void AutoPlayer::PlayLoop() {
    try {
        spdlog::info("Start auto play");

        play_start_callback_();
        Finalizer finalizer([this]() {
            AfterAutoPlayStop();
            play_stop_callback_();
            try {
                cv::destroyWindow("Img Show");
            } catch (...) {
            }
            spdlog::info("Stop auto play");
        });
        NoteFinder note_finder(pc_.color_delta);
        TrackController track(tc_, note_finder, note_estimator_);

        int64_t check_time_ms = GetCurrentTimeMs();
        bool is_hold_started = false;
        bool img_show_window_created = false;

        while (play_run_flag_) {
            int64_t cur_time_ms = GetCurrentTimeMs();

            if (cur_time_ms >= check_time_ms) {
                check_time_ms += pc_.check_loop_delay_ms;

                for (int i = 0; i < notes_.size();) {
                    const auto& note = notes_[i];
                    if (track.IsOutSideCheckRange(cur_time_ms,
                                                  note.hit_time_ms)) {
                        int64_t hit_time_ms =
                            note.hit_time_ms + pc_.hit_delay_ms;
                        cv::Point pos{note.hit_x, tc_.hit_line_y};
                        int64_t delay_ms = hit_time_ms - cur_time_ms;
                        if (note.type == NoteType::Tap ||
                            note.type == NoteType::Hold) {
                            tap_touch_.TouchTapAsyn(pos, kTapDurationMs,
                                                    delay_ms);
                        } else {
                            slide_touch_.TouchSlideAsyn(
                                pos, pos + cv::Point{0, kSlideMoveDY},
                                kSlideDurationMs, kSlideStepDelayMs, 1.0, 1.0,
                                delay_ms);
                        }
                        notes_[i] = notes_.back();
                        notes_.pop_back();
                    } else {
                        ++i;
                    }
                }

                cv::Mat capture_img = screen_.Screencap();
                cur_time_ms = GetCurrentTimeMs();
                auto new_notes = track.FindNotes(cur_time_ms, capture_img);
                for (const auto& new_note : new_notes) {
                    bool is_new = true;
                    for (auto& note : notes_) {
                        if (note.type == new_note.type &&
                            std::abs(note.hit_x - new_note.hit_x) <
                                kMinNoteDX &&
                            std::abs(note.hit_time_ms - new_note.hit_time_ms) <
                                kMinNoteTimeDistMs) {
                            is_new = false;
                            note.hit_x = new_note.hit_x;
                            note.hit_time_ms = new_note.hit_time_ms;
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
                        notes_.push_back(NoteSample{new_note.type,
                                                    new_note.hit_x,
                                                    new_note.hit_time_ms});
                    }
                }

                if (!is_hold_started && !notes_.empty()) {
                    StartHoldTouch(track.GetHitLine());
                    is_hold_started = true;
                }

                if (img_show_flag_) {
                    if (!img_show_window_created) {
                        cv::namedWindow("Img Show", cv::WINDOW_AUTOSIZE);
                        img_show_window_created = true;
                    }
                    cv::imshow("Img Show", capture_img);
                    cv::waitKey(1);
                    if (cv::getWindowProperty("Img Show",
                                              cv::WND_PROP_VISIBLE) < 1) {
                        img_show_flag_ = false;
                        img_show_window_created = false;
                        try {
                            cv::destroyWindow("Img Show");
                        } catch (...) {
                        }
                        img_show_off_callback_();
                    }
                }
            }

            cur_time_ms = GetCurrentTimeMs();
            auto wait_time =
                std::max(kMinLoopWaitTimeMs, check_time_ms - cur_time_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
        }
    } catch (const std::exception& e) {
        spdlog::error("PlayLoop error: {}", e.what());
    }
}

void AutoPlayer::CheckLoop(int loop_cnt) {
    try {
        spdlog::info("Start auto check");

        check_start_callback_();
        Finalizer finalizer([this]() {
            Stop();
            check_run_flag_.store(false, std::memory_order_release);
            check_stop_callback_();
            spdlog::info("Stop auto check");
        });
        while (check_run_flag_ && loop_cnt != 0) {
            if (play_run_flag_) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(kPlayingCheckLoopDelayMs));
                int cnt = 0;
                for (int i = 0; i < 3; ++i) {
                    if (Events::SongPlaying().Check(screen_.Screencap(), 0.9)) {
                        break;
                    }
                    cnt++;
                }
                if (cnt >= 3) {
                    --loop_cnt;
                    Stop();
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(10000));
                }
            } else {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(kCheckLoopDelayMs));
                if (Events::SongStart().Check(screen_.Screencap(), 0.9) ||
                    Events::SongPlaying().Check(screen_.Screencap(), 0.9)) {
                    Start();
                }
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("CheckLoop error: {}", e.what());
    }
}

} // namespace psh