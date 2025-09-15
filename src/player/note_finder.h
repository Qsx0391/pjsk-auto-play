#pragma once

#ifndef PSH_PLAYER_NOTE_FINDER_H_
#define PSH_PLAYER_NOTE_FINDER_H_

#include <vector>
#include <qstring.h>

#include <opencv2/opencv.hpp>

#include "screen/i_screen.h"
#include "common/hr_line.h"
#include "player/auto_play_constant.h"
#include "player/note_time_estimator.h"

namespace psh {

// clang-format off
struct TrackConfig {
    int upper_len     = 59;
    int lower_len     = 1260;
    int height        = 720; 
    int hit_line_y    = 567;
    int check_upper_y = 0;
    int check_lower_y = 270;
    int dx            = 10;
    int dy            = 0;
};
// clang-format on

struct Note {
    bool is_slide;
    HoldType hold;
    NoteColor color;
    cv::Rect box;
    cv::Point hit_pos;
    int64_t hit_time_ms;

    bool IsHold() const { return hold != HoldType::None; }
    bool IsHoldBegin() const { return hold == HoldType::HoldBegin; }
    bool IsHoldEnd() const { return hold == HoldType::HoldEnd; }
};

class NoteFinder {
public:
    static constexpr int kMinNoteWidth = 5;
    static constexpr int kHoldCheckDY = 6;

    NoteFinder(NoteTimeEstimator& estimator, const TrackConfig& track_config);
    NoteFinder(const NoteFinder&) = default;
    NoteFinder(NoteFinder&&) = default;

    std::vector<std::pair<NoteColor, std::vector<Note>>> FindAllNotes(
        const Frame& frame);
    HrLine GetHitLine() const { return hit_line_; }
    cv::Mat GetTrackImg(const cv::Mat& img) const;

private:
    HoldType FindHoldType(const Frame& frame, NoteColor color, cv::Rect rect);
    HrLine TrackLineOf(int line_y) const;

    NoteTimeEstimator& estimator_;

    std::vector<std::vector<cv::Point>> note_contours_;
    TrackConfig tc_;
    cv::Rect track_area_;
    cv::Rect check_area_;
    cv::Mat track_mask_;
    cv::Mat check_mask_;
    HrLine hit_line_;
};

} // namespace psh

#endif // !PSH_PLAYER_NOTE_FINDER_H_
