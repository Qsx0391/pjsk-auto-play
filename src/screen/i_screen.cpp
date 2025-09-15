#include "screen/i_screen.h"

#include <spdlog/spdlog.h>

#include "common/time_utils.h"

namespace psh {

Frame IScreen::GetFrame() {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    frame_ = Frame{Capture(), GetCurrentTimeMs()};
    return frame_;
}

Frame IScreen::GetFrame(int max_interval_ms) {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    if (GetCurrentTimeMs() - frame_.capture_time_ms >= max_interval_ms) {
        frame_ = Frame{Capture(), GetCurrentTimeMs()};
    }
    return frame_;
}

} // namespace psh