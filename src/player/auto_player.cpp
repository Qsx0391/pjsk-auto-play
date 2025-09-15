#include "player/auto_player.h"

#include <deque>

#include <spdlog/spdlog.h>

#include <QApplication>
#include <QMessageBox>
#include <QMetaObject>
#include <Qt>
#include <MikuMikuWorld/ScoreConverter.h>

#include "common/time_utils.h"
#include "common/finalizer.hpp"
#include "common/cv_utils.h"
#include "sus/score_touch.h"
#include "sus/sus_loader.h"
#include "player/display_manager.h"
#include "player/song_utils.h"

namespace MMW = MikuMikuWorld;

namespace {
using namespace psh;

void FillExecutorByScoreTouch(TouchExecutor& executor,
                              const ScoreTouch& score_touch,
                              const HrLine& hit_line) {
    constexpr double kLaneCount = 12.0;
    for (const auto& note : score_touch.notes) {
        cv::Point pos = hit_line.PosOf(note.lane / kLaneCount);
        if (note.flick == MMW::FlickType::None) {
            executor.TouchTap(note.delay_ms, pos, kTapDurationMs);
        } else {
            executor.TouchSlide(note.delay_ms + kSlideDelayMs, pos,
                                pos + cv::Point{0, kSlideMoveDY},
                                kSlideDurationMs, kSlideStepDelayMs, 1.0, 1.0);
        }
    }
    for (const auto& hold : score_touch.holds) {
        TouchTaskStream tasks;
        if (hold.start.flick == MMW::FlickType::None) {
            tasks.AddTask(hold.start.delay_ms, TouchAction::Down,
                          hit_line.PosOf(hold.start.lane / kLaneCount));
        } else {
            tasks.AddSlide(hold.start.delay_ms + kSlideDelayMs,
                           hit_line.PosOf(hold.start.lane / kLaneCount) -
                               cv::Point{0, kSlideMoveDY},
                           hit_line.PosOf(hold.start.lane / kLaneCount),
                           kSlideDurationMs, kSlideStepDelayMs, 1.0, 1.0, true,
                           false);
        }
        for (const auto& step : hold.steps) {
            tasks.AddTask(step.delay_ms, TouchAction::Move,
                          hit_line.PosOf(step.lane / kLaneCount));
        }
        if (hold.end.flick == MMW::FlickType::None) {
            tasks.AddTask(hold.end.delay_ms, TouchAction::Up, {});
        } else {
            tasks.AddSlide(hold.end.delay_ms + kHoldEndSlideDelayMs,
                           hit_line.PosOf(hold.end.lane / kLaneCount),
                           hit_line.PosOf(hold.end.lane / kLaneCount) +
                               cv::Point{0, kSlideMoveDY},
                           kSlideDurationMs, kSlideStepDelayMs, 1.0, 1.0, false,
                           true);
        }
        executor.Execute(std::move(tasks));
    }
}

} // namespace

namespace psh {

AutoPlayer::AutoPlayer(TouchController& touch, IScreen& screen,
                       const TrackConfig& track_config,
                       const PlayConfig& play_config, QObject* parent)
    : QObject(parent), screen_(screen), touch_(touch) {
    SetTrackConfig(track_config);
    SetPlayConfig(play_config);
}

AutoPlayer::~AutoPlayer() { Stop(); }

bool AutoPlayer::Start() {
    if (run_flag_.exchange(true, std::memory_order_acq_rel)) {
        spdlog::warn("Auto play already running");
        return false;
    }
    if (play_worker_.joinable()) {
        play_worker_.join();
    }
    play_worker_ = std::thread(&AutoPlayer::MainLoop, this);
    return true;
}

void AutoPlayer::Stop() {
    run_flag_.store(false, std::memory_order_release);
    if (play_worker_.joinable()) {
        play_worker_.join();
    }
}

void AutoPlayer::StartHoldTouch(TouchExecutor& executor,
                                HrLine hit_line) const {
    int n = pc_.hold_cnt;
    auto hold_ranges = hit_line.Split(n);
    for (int i = 0; i < n; ++i) {
        cv::Point pos = hold_ranges[i].PosOf(0.5);
        TouchTaskStream task_stream;
        task_stream.AddTask(0, TouchAction::Down, pos);
        executor.Execute(std::move(task_stream));
    }
}

void AutoPlayer::UpdateFrame() {
    prev_frame_ = frame_;
    frame_ = screen_.GetFrame();
}

void AutoPlayer::ExecuteTouch(TouchExecutor& executor,
                              std::deque<NoteSample>& s) const {
    const auto& front = s.front();
    if (front.touched) {
        return;
    }
    const auto& note = front.note;
    int64_t real_hit_time = note.hit_time_ms + pc_.cv_hit_delay_ms;
    if (note.IsHoldEnd() && note.is_slide) {
        TouchTaskStream task_stream;
        task_stream.AddTask(real_hit_time + kHoldDelayMs, TouchAction::Down,
                            note.hit_pos);
        task_stream.AddSlide(real_hit_time + kHoldEndSlideDelayMs, note.hit_pos,
                             note.hit_pos + cv::Point{0, kSlideMoveDY},
                             kSlideDurationMs, kSlideStepDelayMs, 1.0, 1.0,
                             false, true);
        executor.Execute(task_stream);
    } else if (note.is_slide) {
        executor.TouchSlide(real_hit_time, note.hit_pos,
                            note.hit_pos + cv::Point{0, kSlideMoveDY},
                            kSlideDurationMs, kSlideStepDelayMs, 1.0, 1.0);
    } else if (note.IsHoldEnd()) {
        executor.TouchTap(real_hit_time + kHoldDelayMs, note.hit_pos,
                          -kHoldDelayMs);
    } else {
        executor.TouchTap(real_hit_time, note.hit_pos, kTapDurationMs);
    }
}

int AutoPlayer::CalcMinSampleCount(NoteTimeEstimator& estimator,
                                   double factor) const {
    int low = estimator.EstimateHitTime(tc_.check_lower_y);
    int high = estimator.EstimateHitTime(tc_.check_upper_y);
    return factor * (high - low) / pc_.check_loop_delay_ms;
}

void AutoPlayer::MainLoop() {
    try {
        spdlog::info("Start auto play");
        Finalizer finalizer([this]() {
            run_flag_.store(false, std::memory_order_release);
            emit playStopped();
            spdlog::info("Stop auto play");
        });

        const Event* event = nullptr;
        const Event* prev_event = nullptr;
        int count = 0;
        while (run_flag_.load(std::memory_order_acquire)) {
            UpdateFrame();

            const Event* cur = Events::MatchEventByName(
                {"solo_song_playing", "multi_song_playing"}, frame_.img, 40.0);
            if (cur != nullptr) {
                auto sus_ptr = pc_.sus_mode ? SusLoader::GetSus() : nullptr;
                if (sus_ptr) {
                    SusPlayLoop(sus_ptr->sus, *cur);
                } else {
                    if (pc_.sus_mode) {
                        spdlog::info("SUS not ready yet, use cv play");
                    }
                    SimpleCvPlayLoop(*cur);
                }
                if (play_mode_.load(std::memory_order_acquire) ==
                    PlayMode::kOnce) {
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5000));
                event = nullptr;
                prev_event = nullptr;
                count = 0;
                const Event* e = &Events::GetEvent("main_menu");
                while (!e->Check(frame_.img)) {
                    if (!run_flag_.load(std::memory_order_acquire)) {
                        return;
                    }
                    touch_.TouchTap(e->GetButton(QString("next1")));
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(1000));
                    UpdateFrame();
                }
                continue;
            }

            auto mode = play_mode_.load(std::memory_order_acquire);
            std::vector<QString> event_names = {
                "multi_play_start", "solo_play_start", "challenge_play_start"};
            if (mode != PlayMode::kOnce) {
                event_names.push_back("main_menu");
                event_names.push_back("live_menu");
            }
            if (mode == PlayMode::kSolo) {
                event_names.push_back("solo_song_choosing");
            } else if (mode == PlayMode::kMulti) {
                event_names.push_back("multi_mode_choosing");
                event_names.push_back("multi_song_choosing");
            }

            cur = Events::MatchEventByName(event_names, frame_.img);
            count = cur == prev_event ? count + 1 : 1;
            prev_event = cur;

            if (event != cur && ((cur == nullptr && count == 20) ||
                                 (cur != nullptr && count == 5))) {
                event = cur;
                if (cur != nullptr) {
                    spdlog::info("Detected event: {}",
                                 cur->GetName().toUtf8().constData());

                    if (cur->GetName() == "solo_play_start" ||
                        cur->GetName() == "challenge_play_start") {
                        if (pc_.sus_mode) {
                            auto diff_opt = FindSoloDifficulty(frame_.img);
                            if (!diff_opt.has_value()) {
                                --count; // try again later
                                spdlog::info(
                                    "Cannot determine song difficulty");
                            } else {
                                auto song_name_img = GetSongNameImg(
                                    frame_.img, cur->GetArea("song_name"));
                                const QString& diff_str =
                                    kDifficultyStrs[static_cast<int>(
                                        *diff_opt)];
                                SusLoader::LoadSusByImg(song_name_img,
                                                        diff_str);
                            }
                        }
                        if (count == 5 && mode == PlayMode::kSolo) {
                            touch_.TouchTap(cur->GetButton("confirm"));
                        }

                    } else if (cur->GetName() == "multi_play_start") {
                        auto diff = pc_.auto_select
                                        ? SelectMultiDifficulty(frame_.img,
                                                                pc_.max_diff)
                                        : pc_.max_diff;
                        const QString& diff_str =
                            kDifficultyStrs[static_cast<int>(diff)];

                        int64_t cur_time_ms = GetCurrentTimeMs();
                        touch_.TouchTap(cur->GetButton(diff_str));
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(1000));
                        touch_.TouchTap(cur->GetButton("confirm"));

                        if (pc_.sus_mode) {
                            auto song_name_img = GetSongNameImg(
                                frame_.img, cur->GetArea("song_name"));
                            SusLoader::LoadSusByImg(song_name_img, diff_str);
                        }

                    } else if (cur->GetName() == "main_menu") {
                        touch_.TouchTap(cur->GetButton("live"));

                    } else if (cur->GetName() == "live_menu") {
                        auto mode =
                            play_mode_.load(std::memory_order_acquire);
                        if (mode == PlayMode::kSolo) {
                            touch_.TouchTap(cur->GetButton("solo_live"));
                        } else if (mode == PlayMode::kMulti) {
                            touch_.TouchTap(cur->GetButton("multi_live"));
                        }

                    } else if (cur->GetName() == "solo_song_choosing") {
						touch_.TouchTap(cur->GetButton("confirm"));

					} else if (cur->GetName() == "multi_mode_choosing") {
                        touch_.TouchTap(cur->GetButton("veteran"));

                    } else if (cur->GetName() == "multi_song_choosing") {
                        touch_.TouchTap(cur->GetButton("random"));
                    } 
                }
            }
            std::this_thread::sleep_for(
                std::chrono::milliseconds(kMainLoopDelayMs));
        }
    } catch (const std::exception& e) {
        spdlog::error("Auto play error: {}", e.what());
    }
}
class StopChecker {
public:
    StopChecker(cv::Point pos) : pos_(pos) {}

    bool Check(const Frame& frame) {
        cv::Vec3b color = frame.img.at<cv::Vec3b>(pos_);
        if (counter_ < 0) {
            return false;
        }
        if (frame.capture_time_ms - last_check_time_ < 200) {
            return true;
        }
        last_check_time_ = frame.capture_time_ms;

        if (ColorSimilar(color, kDeadColor, 10)) {
            QMetaObject::invokeMethod(
                qApp,
                []() {
                    auto* box = new QMessageBox(
                        QMessageBox::Information, QString::fromUtf8("提示"),
                        QString::fromUtf8("检测到歌曲已完成或hp为零，请注意。"),
                        QMessageBox::Ok);
                    box->setWindowModality(Qt::NonModal);
                    box->setAttribute(Qt::WA_DeleteOnClose);
                    box->show();
                },
                Qt::QueuedConnection);
            counter_ = -1;
            return false;
        }

        if (!ColorSimilar(color, kAliveColor, 10)) {
            ++counter_;
            if (counter_ == 5) {
                counter_ = -1;
                return false;
            }
        } else {
            counter_ = 0;
        }
        return true;
    }

private:
    cv::Point pos_;
    int64_t last_check_time_ = 0;
    int counter_ = 0;
};

void AutoPlayer::SimpleCvPlayLoop(const Event& event) {
    spdlog::info("Start simple cv auto play");
    Finalizer finalizer([this]() { spdlog::info("Stop simple cv auto play"); });

    try {
        TouchExecutor executor = touch_.CreateExecutor();
        executor.Start();
        NoteTimeEstimator estimator(pc_.speed_factor);
        NoteFinder finder(estimator, tc_);
        StopChecker stop_checker(event.GetPoint("hp"));
        StartHoldTouch(executor, finder.GetHitLine());

        std::vector<std::deque<NoteSample>> samples(4);
        int min_sample_count = CalcMinSampleCount(estimator, 0.3);

        int64_t check_time_ms = GetCurrentTimeMs();
        while (run_flag_.load(std::memory_order_acquire)) {
            int64_t cur_time_ms = GetCurrentTimeMs();
            if (cur_time_ms >= check_time_ms) {
                UpdateFrame();

                if (!prev_frame_.img.empty() &&
                    cv::norm(prev_frame_.img, frame_.img, cv::NORM_L2) == 0) {
                    spdlog::debug("Frame is the same as previous, skipping");
                    continue;
                }

                if (!stop_checker.Check(frame_)) {
                    return;
                }

                auto found = finder.FindAllNotes(frame_);
                for (auto& each : found) {
                    auto& pre = samples[static_cast<int>(each.first)];
                    auto& cur = each.second;
                    std::vector<int8_t> matched(cur.size(), 0);

                    for (int i = 0; i < pre.size();) {
                        bool found = false;
                        for (int j = 0; j < cur.size(); ++j) {
                            if (!matched[j] &&
                                std::abs(pre[i].note.hit_time_ms -
                                         cur[j].hit_time_ms) < kMinNoteDT &&
                                std::abs(pre[i].note.hit_pos.x -
                                         cur[j].hit_pos.x) < kMinNoteDX) {
                                pre[i].AddSample(cur[j]);
                                matched[j] = true;
                                found = true;
                                break;
                            }
                        }
                        if (!found && i == 0) {
                            if (pre[i].count > min_sample_count) {
                                ExecuteTouch(executor, pre);
                            } else {
                                spdlog::info("Note has too few samples: {}",
                                             pre[i].count);
                            }
                            pre.pop_front();
                        } else {
                            ++i;
                        }
                    }

                    for (int i = 0; i < cur.size(); ++i) {
                        if (!matched[i]) {
                            pre.emplace_back(cur[i]);
                        }
                    }
                }

                check_time_ms +=
                    DisplayManager::UpdateDisplay(frame_.img, touch_, found)
                        ? kDisplayDelayMs
                        : pc_.check_loop_delay_ms;
            }

            cur_time_ms = GetCurrentTimeMs();
            auto wait_time =
                std::max(kMinLoopWaitTimeMs, check_time_ms - cur_time_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
        }
    } catch (const std::exception& e) {
        spdlog::error("Simple cv play error: {}", e.what());
    }
}

void AutoPlayer::SusPlayLoop(const MikuMikuWorld::SUS& sus,
                             const Event& event) {
    spdlog::info("Start SUS play");
    Finalizer finalizer([this]() { spdlog::info("Stop SUS play"); });

    try {
        MMW::ScoreConverter converter;
        MMW::Score score = converter.susToScore(sus);
        ScoreTouch score_touch = ScoreToTouch(score);

        NoteTimeEstimator estimator(pc_.speed_factor);
        NoteFinder finder(estimator, tc_);
        HrLine hit_line = finder.GetHitLine();
        TouchExecutor executor = touch_.CreateExecutor();

        FillExecutorByScoreTouch(executor, score_touch, hit_line);

        int first_note_ms = INT_MAX;
        for (const auto& note : score_touch.notes) {
            first_note_ms = std::min(first_note_ms, note.delay_ms);
        }
        for (const auto& hold : score_touch.holds) {
            first_note_ms = std::min(first_note_ms, hold.start.delay_ms);
        }
        if (first_note_ms == INT_MAX) {
            spdlog::warn("No valid tap notes in the score");
            return;
        }

        UpdateFrame();
        cv::Mat track_img = finder.GetTrackImg(frame_.img);

        int start_time_ms = -1;
        int max_delay_ms =
            std::max(0, -pc_.sus_hit_delay_ms) + pc_.check_loop_delay_ms;
        while (true) {
            if (!run_flag_.load(std::memory_order_acquire)) {
                return;
            }

            UpdateFrame();
            cv::Mat cur_track = finder.GetTrackImg(frame_.img);
            cv::Mat diff, gray, mask;
            cv::absdiff(track_img, cur_track, diff);
            cv::cvtColor(diff, gray, cv::COLOR_BGR2GRAY);
            cv::threshold(gray, mask, 20, 255, cv::THRESH_BINARY);

            int max_y = -1;
            for (int y = mask.rows - 1; y >= 0; --y) {
                const uchar* row = mask.ptr<uchar>(y);
                if (cv::countNonZero(
                        cv::Mat(1, mask.cols, CV_8UC1, (void*)row))) {
                    max_y = y;
                    break;
                }
            }

            if (max_y != -1) {
                int delay_ms = estimator.EstimateHitTime(max_y);
                if (delay_ms <= max_delay_ms) {
                    start_time_ms = frame_.capture_time_ms + delay_ms;
                    break;
                }
            }

            DisplayManager::UpdateDisplay(frame_.img, touch_, {});
            std::this_thread::sleep_for(
                std::chrono::milliseconds(pc_.check_loop_delay_ms));
        }

        executor.SetBaseTime(start_time_ms - first_note_ms +
                             pc_.sus_hit_delay_ms);
        executor.Start();

        StopChecker stop_checker(event.GetPoint("hp"));
        while (run_flag_.load(std::memory_order_acquire)) {
            UpdateFrame();
            if (!stop_checker.Check(frame_)) {
                return;
            }
            int64_t wait_ms =
                DisplayManager::UpdateDisplay(frame_.img, touch_, {})
                    ? kDisplayDelayMs
                    : 200;
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
        }
    } catch (const std::exception& e) {
        spdlog::error("SusPlay error: {}", e.what());
    }
}

} // namespace psh