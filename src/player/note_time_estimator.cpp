#include "player/note_time_estimator.h"

#include <spdlog/spdlog.h>

#include "common/time_utils.h"

namespace {
using namespace psh;
using NTE = NoteTimeEstimator;

constexpr double pow_constexpr(double base, unsigned int exp) {
    return (exp == 0)       ? 1
           : (exp % 2 == 0) ? pow_constexpr(base * base, exp / 2)
                            : base * pow_constexpr(base, exp - 1);
}

// clang-format off
constexpr int CalcSpeed6x(int y) {
    return static_cast<int>(
        1.24432e-23 * pow_constexpr(y, 10) - 5.0288e-20  * pow_constexpr(y, 9) +
        8.80485e-17 * pow_constexpr(y, 8)  - 8.78621e-14 * pow_constexpr(y, 7) +
        5.52955e-11 * pow_constexpr(y, 6)  - 2.2944e-08  * pow_constexpr(y, 5) +
        6.3891e-06  * pow_constexpr(y, 4)  - 0.00120429  * pow_constexpr(y, 3) +
        0.159045    * pow_constexpr(y, 2)  - 17.9278 * y + 2068.31);
}

constexpr int CalcSpeed8x(int y) {
    return static_cast<int>(
        6.51134e-24 * pow_constexpr(y, 10) - 2.62712e-20 * pow_constexpr(y, 9) +
        4.64885e-17 * pow_constexpr(y, 8)  - 4.74537e-14 * pow_constexpr(y, 7) +
        3.0849e-11  * pow_constexpr(y, 6)  - 1.32885e-08 * pow_constexpr(y, 5) +
        3.83616e-06 * pow_constexpr(y, 4)  - 0.000744718 * pow_constexpr(y, 3) +
        0.100764    * pow_constexpr(y, 2)  - 11.6628 * y + 1370.31);
}

constexpr int CalcSpeed9x(int y) {
    return static_cast<int>(
        5.39941e-24 * pow_constexpr(y, 10) - 2.36393e-20 * pow_constexpr(y, 9) +
        4.4621e-17  * pow_constexpr(y, 8)  - 4.76016e-14 * pow_constexpr(y, 7) +
        3.16038e-11 * pow_constexpr(y, 6)  - 1.35728e-08 * pow_constexpr(y, 5) +
        3.81559e-06 * pow_constexpr(y, 4)  - 0.000704868 * pow_constexpr(y, 3) +
        0.0885754   * pow_constexpr(y, 2)  - 9.40113 * y + 1052.77);
}

constexpr int CalcSpeed10x(int y) {
    return static_cast<int>(
        -1.09901e-23 * pow_constexpr(y, 10) + 3.57946e-20 * pow_constexpr(y, 9) -
        4.83534e-17  * pow_constexpr(y, 8)  + 3.45687e-14 * pow_constexpr(y, 7) -
        1.3484e-11   * pow_constexpr(y, 6)  + 2.30944e-09 * pow_constexpr(y, 5) +
        2.23416e-07  * pow_constexpr(y, 4)  - 0.000193387 * pow_constexpr(y, 3) +
        0.0429477    * pow_constexpr(y, 2)  - 6.21282 * y + 764.064);
}

constexpr int CalcSpeed11x(int y) {
    return static_cast<int>(
        -3.24517e-24 * pow_constexpr(y, 10) + 1.0857e-20  * pow_constexpr(y, 9) -
        1.49143e-17  * pow_constexpr(y, 8)  + 1.04939e-14 * pow_constexpr(y, 7) -
        3.57645e-12  * pow_constexpr(y, 6)  + 1.04744e-10 * pow_constexpr(y, 5) +
        3.85198e-07  * pow_constexpr(y, 4)  - 0.000153805 * pow_constexpr(y, 3) +
        0.030589     * pow_constexpr(y, 2)  - 4.29532 * y + 518.311);
}
// clang-format on

template <auto CalcFunc>
constexpr std::array<int, NTE::kDelayLoopupSize> BuildLookup() {
    std::array<int, NTE::kDelayLoopupSize> lookup{};
    for (int y = 0; y < lookup.size(); ++y) {
        lookup[y] = CalcFunc(y);
    }
    return lookup;
}

constexpr auto kSpeed6xLookup = BuildLookup<CalcSpeed6x>();
constexpr auto kSpeed8xLookup = BuildLookup<CalcSpeed8x>();
constexpr auto kSpeed9xLookup = BuildLookup<CalcSpeed9x>();
constexpr auto kSpeed10xLookup = BuildLookup<CalcSpeed10x>();
constexpr auto kSpeed11xLookup = BuildLookup<CalcSpeed11x>();

const std::array<int, NTE::kDelayLoopupSize> &GetLookupTable(
    SpeedFactor speed_factor) {
    switch (speed_factor) {
        case SpeedFactor::kSpeed6x: return kSpeed6xLookup;
        case SpeedFactor::kSpeed8x: return kSpeed8xLookup;
        case SpeedFactor::kSpeed9x: return kSpeed9xLookup;
        case SpeedFactor::kSpeed10x: return kSpeed10xLookup;
        case SpeedFactor::kSpeed11x: return kSpeed11xLookup;
        default: throw std::invalid_argument("Unsupported speed factor");
    }
}

} // namespace

namespace psh {

NoteTimeEstimator::NoteTimeEstimator(SpeedFactor speed_factor)
    : delay_lookup_(&GetLookupTable(speed_factor)) {}

int NoteTimeEstimator::EstimateHitTime(int pos_y) {
    return (*delay_lookup_)[pos_y];
}

void NoteTimeEstimator::SetSpeedFactor(SpeedFactor speed_factor) {
    delay_lookup_ = &GetLookupTable(speed_factor);
}

} // namespace psh