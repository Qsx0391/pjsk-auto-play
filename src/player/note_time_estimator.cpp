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
    /*
               10             9             8             7             6
    6.511e-24 x  - 2.627e-20 x + 4.649e-17 x - 4.745e-14 x + 3.085e-11 x
                 5             4             3          2
    - 1.329e-08 x + 3.836e-06 x - 0.0007447 x + 0.1008 x - 11.66 x + 1370
    */

    int n = check_lower_y - check_upper_y + 1;
    std::vector<int> delay_lookup(n);

    for (int y = check_upper_y; y <= check_lower_y; ++y) {
        int delay_time_ms = static_cast<int>(
            6.511e-24 * std::pow(y, 10) - 2.627e-20 * std::pow(y, 9) +
            4.649e-17 * std::pow(y, 8) - 4.745e-14 * std::pow(y, 7) +
            3.085e-11 * std::pow(y, 6) - 1.329e-08 * std::pow(y, 5) +
            3.836e-06 * std::pow(y, 4) - 0.0007447 * std::pow(y, 3) +
            0.1008 * std::pow(y, 2) - 11.66 * y + 1370);
        delay_lookup[y - check_upper_y] = delay_time_ms;
    }
    return delay_lookup;
}

int NoteTimeEstimator::EstimateHitTime(int pos_y) {
    return (pos_y >= 0 && pos_y < delay_lookup_.size()) ? delay_lookup_[pos_y]
                                                        : -1;
}

} // namespace psh