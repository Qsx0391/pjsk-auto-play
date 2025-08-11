#include "track_controller.h"

#include <spdlog/spdlog.h>

#include "hr_line.h"

namespace psh {

TrackController::TrackController(const TrackConfig &track_config,
                                 NoteFinder &finder,
                                 NoteTimeEstimator &estimator)
    : finder_(finder), estimator_(estimator), tc_(track_config) {
    HrLine check_upper = LineOf(tc_.check_upper_y);
    HrLine check_lower = LineOf(tc_.check_lower_y);
    hit_line_ = LineOf(tc_.hit_line_y);
    check_rect_ = cv::Rect(check_lower.pos.x, 0, check_lower.length,
                           check_lower.pos.y - check_upper.pos.y);
    min_hit_delay_ms_ = estimator_.EstimateHitTime(tc_.check_lower_y);
}

std::vector<NoteInfo> TrackController::FindNotes(int64_t cur_time_ms,
                                                 cv::Mat &img) {
    auto notes = finder_.FindAllNotes(img(check_rect_));
    std::vector<NoteInfo> ret(notes.size());
    for (int i = 0; i < notes.size(); ++i) {
        HrLine line = LineOf(notes[i].second.y);
        cv::Point hit_pos = hit_line_.PosOf(line, notes[i].second + check_rect_.tl());
        ret[i] = NoteInfo {
			notes[i].first,
			hit_pos.x,
			cur_time_ms + estimator_.EstimateHitTime(notes[i].second.y)
		};
    }
    return ret;
}

bool TrackController::IsOutSideCheckRange(int64_t cur_time_ms,
                                          int64_t hit_time_ms) const {
    return cur_time_ms + min_hit_delay_ms_ - 100 > hit_time_ms;
}

HrLine TrackController::LineOf(int line_y) const {
    int a = (tc_.lower_len - tc_.upper_len) * (tc_.height - line_y) / tc_.height;
    return HrLine{cv::Point{tc_.dx + a / 2, line_y}, tc_.lower_len - a};
}

} // namespace psh
