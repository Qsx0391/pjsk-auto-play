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
#include "framework.h"
#include "ring_buffer.hpp"
#include "range_list.hpp"

namespace psh {

class AutoPlayer {
public:
    using Color = cv::Vec3b;

    struct TouchConfig {
        ITouch &touch;
        std::vector<int> slot_indexs;
    };

    struct PlayConfig {
        int max_color_dist;
        int tap_delay_ms;
        int slide_delay_ms;
        int tap_duration_ms;
        int check_loop_delay_ms;
    };
    inline static const PlayConfig kDefaultPlayConfig = {
        100, 2110, 2110, 20, 10,
    };

    struct TrackConfig {
        int upper_len;
        int lower_len;
        int height;
        int hit_line_height;
        int check_line_height;
        int dx;
        int dy;
    };
    inline static const TrackConfig kDefaultTrackConfig = {
        59, 1260, 720, 153, 650, 10, 0,
    };

    AutoPlayer(const TouchConfig &tap, const TouchConfig &hold, IScreen &screen,
               const TrackConfig &track_config, const PlayConfig &play_config);
    virtual ~AutoPlayer() = default;

    void SetPlayConfig(const PlayConfig &play_config);
    void SetTrackConfig(const TrackConfig &track_config);

    bool Start();
    void Stop();

    void TestCheckLine();

private:
    struct CheckInfo {
        int slot_index;
        int64_t time_update;

        static const int kBlankSlotIndex = -1;
        static const int kNoSlotIndex = -2;

        bool operator==(const CheckInfo &rhs) const {
            return slot_index == kBlankSlotIndex &&
                   rhs.slot_index == kBlankSlotIndex;
        }

        static CheckInfo BuildBlank() { return CheckInfo{kBlankSlotIndex, -1}; }
        bool IsBlank() const { return slot_index == kBlankSlotIndex; }
    };

    // clang-format off
    inline static const Color kTapNodeColor              = {255, 243, 243};
    inline static const Color kHoldNodeColor             = {251, 255, 243};
    inline static const Color kSlideNodeColor            = {247, 239, 255};
    inline static const Color kYellowNodeColor           = {203, 251, 255};
    inline static const Color kYellowSlideNodeUpperColor = { 70, 210, 252};
    // clang-format on

    inline static const std::array<Color, 4> kNodeColors = {
        kTapNodeColor, kHoldNodeColor, kSlideNodeColor, kYellowNodeColor};

    static const int kMaxTouchCount = 10;
    static const int kTouchQueueSize = 256;
    static const int kMinNodeWidth = 10;
    static const int kCheckSlideDY = -20;
    static const int kSlideMoveDY = -100;
    static const int kSlideDurationMs = 10;
    static const int kSlideStepDelayMs = 2;

    static const int64_t kTapDurationMs = 20;
    static const int64_t kMinNodeTimeDistNs = 100 * 1000000;
    static const int64_t kMinLoopWaitTimeNs = 1 * 1000000;

    void CheckLoop();

    void PreAutoPlayStart();
    void StartHoldTouch();
    void AfterAutoPlayStop();
    AutoPlayer::CheckInfo BuildCheckInfo(int64_t cur_time, int begin, int end,
                                         const Color &color);
    void DetectNodes(int64_t cur_time);
    void ProcessCheckRanges(int64_t cur_time);

    bool AreColorsClose(const Color &color1, const Color &color2) const;
    std::optional<Color> FindClosestNode(const Color &color) const;
    static int64_t GetCurrentTimeNs();
    static constexpr int64_t MsToNs(int ms) { return ms * 1000000LL; }
    bool CheckIsSlide(const Color &color);

    std::atomic_bool run_flag_ = false;
    bool hold_touch_started_ = false;
    std::thread check_worker_;
    std::thread touch_worker_;

    IScreen &screen_;
    TouchConfig tap_;
    TouchConfig hold_;

    Line check_line_;
    Line hit_line_;

    int64_t tap_delay_ms_;
    int64_t slide_delay_ms_;
    int64_t tap_duration_ms_;
    int64_t check_loop_delay_ns_;

    cv::Mat capture_img_;
    int max_color_dist_square_;

    RangeList<CheckInfo> check_ranges_;

    std::queue<int> unused_slots_;

private:
    struct TouchGrand {
        TouchGrand(AutoPlayer &player) : player_(player) {}
        ~TouchGrand() { player_.AfterAutoPlayStop(); }
        AutoPlayer &player_;
    };
};

} // namespace psh

#endif // AUTO_PLAYER_H_