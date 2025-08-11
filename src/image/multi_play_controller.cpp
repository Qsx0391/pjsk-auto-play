#include "multi_play_controller.h"

#include <spdlog/spdlog.h>
#include <climits>
#include <chrono>
#include <thread>

#include "events.h"
#include "psh_utils.hpp"

namespace psh {

MultiPlayController::MultiPlayController(IScreen& screen, ITouch& touch,
                                         AutoPlayer& auto_player)
    : screen_(screen), touch_(touch), auto_player_(auto_player) {}

MultiPlayController::~MultiPlayController() { Stop(); }

bool MultiPlayController::Start(PlayMode mode, SongDiffculty max_diff) {
    if (run_flag_.load(std::memory_order_acquire)) {
        spdlog::warn("MultiPlayController is already running");
        return false;
    }
    if (worker_.joinable()) {
        worker_.join();
    }
    run_flag_.store(true, std::memory_order_release);
    worker_ =
        std::thread(&MultiPlayController::WorkerLoop, this, mode, max_diff);
    return true;
}

void MultiPlayController::Stop() {
    run_flag_.store(false, std::memory_order_release);
    if (worker_.joinable()) {
        worker_.join();
    }
}

bool MultiPlayController::IsRunning() {
    return run_flag_.load(std::memory_order_acquire);
}

void MultiPlayController::WorkerLoop(PlayMode mode, SongDiffculty max_diff) {
    try {
        spdlog::info("MultiPlayController started");

        Finalizer finalizer([this]() {
            run_flag_.store(false, std::memory_order_release);
            stop_callback_();
            spdlog::info("Stop MultiPlayController");
		});
        start_callback_();
        int step = 0;

        while (run_flag_) {
            cv::Mat capture_img = screen_.Screencap();
            if (step == 0) {
                const auto& event = Events::MultiPlayBegin();
                if (event.Check(capture_img, 0.9)) {
                    spdlog::info(
                        "MultiPlayController detected MultiPlayBegin event");
                    event.ClickBtn(touch_, "Next");
                    step = 1;
                }
            } else if (step == 1) {
                const auto& event = Events::MultiPlaySongChoose();
                if (event.Check(capture_img, 0.9)) {
                    spdlog::info("MultiPlayController detected "
                                 "MultiPlaySongChoose event");
                    SongDiffculty diff = max_diff;
                    if (mode != PlayMode::kFixedDifficulty) {
                        int target_status = 0;
                        if (mode == PlayMode::kUnclearFirst) {
                            target_status =
                                static_cast<int>(SongStatus::kClear);
                        } else if (mode == PlayMode::kUnfullComboFirst) {
                            target_status =
                                static_cast<int>(SongStatus::kFullCombo);
                        } else if (mode == PlayMode::kUnallPerfectFirst) {
                            target_status =
                                static_cast<int>(SongStatus::kAllPerfect);
                        }

                        std::vector<SongStatus> status =
                            FindSongStatus(capture_img);
                        std::string status_str;
                        for (const auto& s : status) {
							status_str +=
								std::to_string(static_cast<int>(s)) + " ";
						}
                        spdlog::info("Song status: {}", status_str);
                        for (int i = 0; i <= static_cast<int>(max_diff); ++i) {
                            if (i < status.size() &&
                                status[i] <
                                    static_cast<SongStatus>(target_status)) {
                                diff = static_cast<SongDiffculty>(i);
                                break;
                            }
                        }
                    }
                    std::string diff_str;
                    switch (diff) {
                        case SongDiffculty::kEasy: diff_str = "Easy"; break;
                        case SongDiffculty::kNormal: diff_str = "Normal"; break;
                        case SongDiffculty::kHard: diff_str = "Hard"; break;
                        case SongDiffculty::kExpert: diff_str = "Expert"; break;
                        case SongDiffculty::kMaster: diff_str = "Master"; break;
                        case SongDiffculty::kAppend: diff_str = "Append"; break;
                    }

                    event.ClickBtn(touch_, diff_str, 0);
                    event.ClickBtn(touch_, "Choose", 1000);
                    auto_player_.AutoCheckStart(1);
                    return;
                }
            } else {
                spdlog::warn(
                    "MultiPlayController reached an unexpected step: {}", step);
                return;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    } catch (const std::exception& e) {
		spdlog::error("MultiPlayController worker loop error: {}", e.what());
	}
}

std::vector<SongStatus> MultiPlayController::FindSongStatus(
    const cv::Mat& img) {
    std::vector<SongStatus> status;
    status.reserve(kSongStatusCheckPoints.size());
    for (const auto& point : kSongStatusCheckPoints) {
        cv::Vec3b color = img.at<cv::Vec3b>(point);
        int min_diff = INT_MAX;
        int idx = -1;
        for (int i = 0; i < kSongStatusColors.size(); ++i) {
            int diff = static_cast<int>(cv::norm(color - kSongStatusColors[i]));
            if (diff < min_diff) {
                min_diff = diff;
                idx = i;
            }
        }
        status.push_back(static_cast<SongStatus>(idx));
    }
    return status;
}

} // namespace psh
