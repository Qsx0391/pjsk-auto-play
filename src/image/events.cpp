#include "events.h"

#include <spdlog/spdlog.h>

namespace psh {

bool Event::Check(const cv::Mat &frame, double threshold) const {
    for (auto &check_imgs : check_imgs_) {
		bool all_match = true;
		for (auto img : check_imgs) {
			if (!img->Check(frame, threshold)) {
				all_match = false;
				break;
			}
		}
		if (all_match) {
			return true;
		}
	}
    return false;
}

void Event::ClickBtn(ITouch& touch, const std::string& btn_name,
                     int delay_ms) const {
    auto it = btn_set_.find(btn_name);
    if (it != btn_set_.end()) {
        touch.TouchTapAsyn(it->second, 20, delay_ms);
	} else {
		spdlog::warn("Button {} not found in event", btn_name);
	}
}

} // namespace psh