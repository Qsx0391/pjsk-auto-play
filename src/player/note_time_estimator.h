#pragma once

#ifndef PSH_PLAYER_NOTE_TIME_ESTIMATOR_H_
#define PSH_PLAYER_NOTE_TIME_ESTIMATOR_H_

#include <array>

#include <opencv2/opencv.hpp>

#include "screen/i_screen.h"

namespace psh {

enum class SpeedFactor {
    kSpeed6x = 6,
    kSpeed8x = 8,
    kSpeed9x = 9,
    kSpeed10x = 10,
    kSpeed11x = 11,
};

class NoteTimeEstimator {
public:
    static const int kDelayLoopupSize = 720;

    NoteTimeEstimator(SpeedFactor speed_factor);
    NoteTimeEstimator(const NoteTimeEstimator&) = default;
    NoteTimeEstimator(NoteTimeEstimator&&) = default;

    int EstimateHitTime(int pos_y);
    void SetSpeedFactor(SpeedFactor speed_factor);

private:
    const std::array<int, kDelayLoopupSize>* delay_lookup_;
};

} // namespace psh

#endif // !PSH_PLAYER_NOTE_TIME_ESTIMATOR_H_
