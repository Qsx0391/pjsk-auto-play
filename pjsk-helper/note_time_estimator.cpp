#include "note_time_estimator.h"

#include <spdlog/spdlog.h>

#include "psh_utils.hpp"

namespace psh {

NoteTimeEstimator::NoteTimeEstimator(int check_upper_y, int check_lower_y)
    : delay_lookup_(BuildLookupTable(check_upper_y, check_lower_y)) {}

NoteTimeEstimator::NoteTimeEstimator(const std::vector<int> &delay_lookup)
    : delay_lookup_(delay_lookup) {}

NoteTimeEstimator::NoteTimeEstimator(std::vector<int> &&delay_lookup)
    : delay_lookup_(std::move(delay_lookup)) {}

std::vector<int> NoteTimeEstimator::BuildLookupTable(int check_upper_y,
                                                     int check_lower_y) {
    int n = check_lower_y - check_upper_y + 1;
    std::vector<int> delay_lookup(n);

    for (int y = check_upper_y; y <= check_lower_y; ++y) {
        int delay_time_ms = static_cast<int>(
            3.496e-08 * std::pow(y, 4) - 5.01e-05 * std::pow(y, 3) +
            0.02721 * std::pow(y, 2) - 7.978 * y + 1310);
        delay_lookup[y - check_upper_y] = delay_time_ms;
    }
    return delay_lookup;
}

int NoteTimeEstimator::EstimateHitTime(int pos_y) {
    return (pos_y >= 0 && pos_y < delay_lookup_.size()) ? delay_lookup_[pos_y]
                                                        : -1;
}

} // namespace psh