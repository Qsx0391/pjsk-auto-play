#pragma once

#ifndef NOTE_TIME_ESTIMATOR_H_
#define NOTE_TIME_ESTIMATOR_H_

#include <opencv2/opencv.hpp>

#include "i_screen.h"
#include "note_finder.h"

namespace psh {

class NoteTimeEstimator {
public:
    inline static const cv::Rect kDefaultRect =
        cv::Rect(cv::Point{1046, 0}, cv::Point{1263, 720});

    NoteTimeEstimator(int check_upper_y, int check_lower_y);
    NoteTimeEstimator(const NoteTimeEstimator&) = default;
    NoteTimeEstimator(NoteTimeEstimator&&) = default;
    NoteTimeEstimator(const std::vector<int>& delay_lookup);
    NoteTimeEstimator(std::vector<int>&& delay_lookup);

    int EstimateHitTime(int pos_y);
    const std::vector<int>& GetLookupTable() const { return delay_lookup_; }
    static std::vector<int> BuildLookupTable(int check_upper_y,
                                             int check_lower_y);

private:
    static const int kMinNoteDY = 100;
    static const int kMinSampleSize = 10;

    std::vector<int> delay_lookup_;
};

} // namespace psh

#endif // !NOTE_TIME_ESTIMATOR_H_
