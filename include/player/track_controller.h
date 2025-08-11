#pragma once

#ifndef TRACK_CONTROLLER_H_
#define TRACK_CONTROLLER_H_

#include <opencv2/opencv.hpp>

#include "auto_play_config.h"
#include "hr_line.h"
#include "i_screen.h"
#include "note_finder.h"
#include "note_time_estimator.h"

namespace psh {

struct NoteInfo {
    NoteType type;
    int hit_x;
    int64_t hit_time_ms;
};

class TrackController {
public:
    TrackController(const TrackConfig& track_config, NoteFinder& finder,
                    NoteTimeEstimator& estimator);
    std::vector<NoteInfo> FindNotes(int64_t cur_time_ms, cv::Mat& img);
    HrLine GetHitLine() const { return hit_line_; }
    bool IsOutSideCheckRange(int64_t cur_time_ms, int64_t hit_time_ms) const;

private:
    HrLine LineOf(int line_y) const;

    NoteFinder& finder_;
    NoteTimeEstimator& estimator_;

    TrackConfig tc_;
    cv::Rect check_rect_;
    HrLine hit_line_;
    int64_t min_hit_delay_ms_;
};

} // namespace psh

#endif // !TRACK_CONTROLLER_H_
