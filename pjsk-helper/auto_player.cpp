#include "auto_player.h"

#include "spdlog/spdlog.h"

namespace psh {

AutoPlayer::AutoPlayer(const TouchConfig& tap, const TouchConfig& hold,
                       IScreen& screen, const TrackConfig& track_config,
                       const PlayConfig& play_config)
    : tap_(tap), hold_(hold), screen_(screen) {
    SetTrackConfig(track_config);
    SetPlayConfig(play_config);
}

void AutoPlayer::SetPlayConfig(const PlayConfig& play_config) {
    const PlayConfig& pc = play_config;
    if (pc.max_color_dist < 0 || pc.tap_delay_ms < 0 || pc.slide_delay_ms < 0 ||
        pc.tap_duration_ms < 0 || pc.check_loop_delay_ms < 0) {
        throw std::invalid_argument("Invalid play config values");
    }

    tap_delay_ms_ = pc.tap_delay_ms;
    slide_delay_ms_ = pc.slide_delay_ms;
    tap_duration_ms_ = pc.tap_duration_ms;
    check_loop_delay_ns_ = MsToNs(pc.check_loop_delay_ms);

    int mcd = pc.max_color_dist;
    max_color_dist_square_ = mcd * mcd;
}

void AutoPlayer::SetTrackConfig(const TrackConfig& track_config) {
    const TrackConfig& tc = track_config;
    auto BuildLine = [&](int line_height) -> Line {
        int a = (tc.lower_len - tc.upper_len) * line_height / tc.height;
        return Line{Point{tc.dx + a / 2, tc.height - line_height},
                    tc.lower_len - a};
    };
    auto Check = [](Point p, int width, int height) -> bool {
        return p.x >= 0 && p.x < width && p.y >= 0 && p.y < height;
    };

    check_line_ = BuildLine(tc.check_line_height);
    hit_line_ = BuildLine(tc.hit_line_height);

    int height = screen_.GetDisplayHeight();
    int width = screen_.GetDisplayWidth();
    if (!Check(check_line_.Left(), width, height) ||
        !Check(check_line_.Right(), width, height)) {
        throw std::invalid_argument(
            "Invalid track config: check line out of bounds");
    }
    if (!Check(hit_line_.Left(), width, height) ||
        !Check(hit_line_.Right(), width, height)) {
        throw std::invalid_argument(
            "Invalid track config: hit line out of bounds");
    }
}

bool AutoPlayer::Start() {
    if (run_flag_.load(std::memory_order_acquire) || check_worker_.joinable()) {
        spdlog::warn("Auto play already running");
        return false;
    }
    PreAutoPlayStart();
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
    if (touch_worker_.joinable()) {
        touch_worker_.join();
    }
    AfterAutoPlayStop();
    spdlog::info("Stop auto play");
}

void AutoPlayer::TestCheckLine() {
    cv::Mat image = screen_.Screencap();
    for (int i = 0; i < check_line_.length; ++i) {
        Point p = check_line_.PosOf(i);
        cv::circle(image, cv::Point(p.x, p.y), 5, cv::Scalar(0, 255, 0), -1);
    }
    cv::line(image, cv::Point(hit_line_.pos.x, hit_line_.pos.y),
             cv::Point(hit_line_.pos.x + hit_line_.length, hit_line_.pos.y),
             cv::Scalar(255, 0, 0), 2);
    cv::imshow("TestCheckPoints", image);
    cv::waitKey(0);
}

void AutoPlayer::PreAutoPlayStart() {
    hold_touch_started_ = false;
    check_ranges_ = RangeList<CheckInfo>(0, check_line_.length - 1,
                                         CheckInfo::BuildBlank());

    unused_slots_ = {};
    for (int index : tap_.slot_indexs) {
        unused_slots_.push(index);
    }
}

void AutoPlayer::StartHoldTouch() {
    int hold_cnt = hold_.slot_indexs.size();
    auto hold_ranges = hit_line_.Split(hold_cnt);
    for (int i = 0; i < hold_cnt; ++i) {
        const Line& cur = hold_ranges[i];
        hold_.touch.TouchDown(cur.PosOf(cur.length / 2), hold_.slot_indexs[i]);
    }
}

void AutoPlayer::AfterAutoPlayStop() {
    for (int index : tap_.slot_indexs) {
        tap_.touch.TouchUp(index);
    }
    for (int index : hold_.slot_indexs) {
        hold_.touch.TouchUp(index);
    }
}

void AutoPlayer::CheckLoop() {
    int64_t next_check_time = GetCurrentTimeNs();

    while (run_flag_) {
        int64_t cur_time = GetCurrentTimeNs();

        if (cur_time >= next_check_time) {
            next_check_time += check_loop_delay_ns_;
            ProcessCheckRanges(cur_time);
            DetectNodes(cur_time);
        }

        auto wait_time =
            std::max(kMinLoopWaitTimeNs, next_check_time - cur_time);
        std::this_thread::sleep_for(std::chrono::nanoseconds(wait_time));
    }
}

AutoPlayer::CheckInfo AutoPlayer::BuildCheckInfo(int64_t cur_time, int begin,
                                                 int end, const Color& color) {
    CheckInfo ret{};
    ret.time_update = cur_time + kMinNodeTimeDistNs;
    if (unused_slots_.empty()) {
        spdlog::warn("No available slots for touch");
        ret.slot_index = CheckInfo::kNoSlotIndex;
        return ret;
    }

    ret.slot_index = unused_slots_.front();
    unused_slots_.pop();
    Point hit_pos = hit_line_.PosOf(check_line_, (begin + end - 1) / 2);
    if (CheckIsSlide(color)) {
        tap_.touch.DelayTouchSlide(
            slide_delay_ms_, hit_pos, hit_pos + Point{0, kSlideMoveDY},
            ret.slot_index, kSlideDurationMs, kSlideStepDelayMs);
    } else {
        tap_.touch.DelayTouchTap(tap_delay_ms_, hit_pos, ret.slot_index,
                                 kTapDurationMs);
    }
    return ret;
}

void AutoPlayer::DetectNodes(int64_t cur_time) {
    capture_img_ = screen_.Screencap();
    for (int i = 0; i < check_ranges_.Size(); ++i) {
        auto& cur = *check_ranges_[i].data;
        if (!cur.IsBlank()) {
            continue;
        }

        int cur_begin = check_ranges_[i].GetBegin();
        int cur_end = check_ranges_[i].GetEnd();

        for (int i = cur_begin; i <= cur_end;) {
            Point p = check_line_.PosOf(i);
            cv::Vec3b color = capture_img_.at<Color>(p.y, p.x);
            auto node_color_opt = FindClosestNode(color);
            if (!node_color_opt.has_value()) {
                ++i;
                continue;
            }

            int j = i + 1;
            for (; j <= cur_end; ++j) {
                p = check_line_.PosOf(j);
                cv::Vec3b cur_color = capture_img_.at<Color>(p.y, p.x);
                if (!AreColorsClose(*node_color_opt, cur_color)) {
                    break;
                }
            }
            if (j - i < kMinNodeWidth) {
                i = j;
                continue;
            }

            if (!hold_touch_started_) {
                StartHoldTouch();
            }

            CheckInfo info =
                BuildCheckInfo(cur_time, i, j - 1, *node_color_opt);
            check_ranges_.Insert(i, j - 1, info);
            spdlog::debug("Detected node from {} to {}: ({}, {}, {})", i, j - 1,
                          (*node_color_opt)[0], (*node_color_opt)[1],
                          (*node_color_opt)[2]);

            i = j;
        }
    }
}

void AutoPlayer::ProcessCheckRanges(int64_t cur_time) {
    bool merge_needed = false;
    for (int i = 0; i < check_ranges_.Size(); ++i) {
        auto& cur = *check_ranges_[i].data;

        if (!cur.IsBlank() && cur_time >= cur.time_update) {
            if (cur.slot_index != CheckInfo::kNoSlotIndex) {
                unused_slots_.push(cur.slot_index);
            }
            cur.slot_index = CheckInfo::kBlankSlotIndex;
            merge_needed = true;
        }
    }

    if (merge_needed) {
        check_ranges_.Merge();
    }
}

bool AutoPlayer::AreColorsClose(const Color& color1,
                                const Color& color2) const {
    int db = color1[0] - color2[0];
    int dg = color1[1] - color2[1];
    int dr = color1[2] - color2[2];
    return (db * db + dg * dg + dr * dr) <= max_color_dist_square_;
};

std::optional<AutoPlayer::Color> AutoPlayer::FindClosestNode(
    const Color& color) const {
    int index = -1;
    int min_dist = INT_MAX;
    for (int i = 0; i < kNodeColors.size(); ++i) {
        int db = color[0] - kNodeColors[i][0];
        int dg = color[1] - kNodeColors[i][1];
        int dr = color[2] - kNodeColors[i][2];
        int cur_diff = db * db + dg * dg + dr * dr;
        if (cur_diff < min_dist) {
            min_dist = cur_diff;
            index = i;
        }
    }
    if (min_dist <= max_color_dist_square_) {
        return kNodeColors[index];
    }
    return std::nullopt;
}

bool AutoPlayer::CheckIsSlide(const Color& color) {
    return color == kSlideNodeColor || color == kYellowNodeColor;
}

int64_t AutoPlayer::GetCurrentTimeNs() {
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

} // namespace psh