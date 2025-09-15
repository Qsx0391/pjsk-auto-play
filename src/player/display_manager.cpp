#include "display_manager.h"

#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>

#include "player/auto_play_constant.h"

namespace psh {

void DisplayManager::StartDisplay() {
    Instance().display_enable_.store(true, std::memory_order_release);
}

void DisplayManager::StopDisplay() {
    Instance().display_enable_.store(false, std::memory_order_release);
}

bool DisplayManager::UpdateDisplay(
    const cv::Mat& frame, TouchController& touch,
    const std::vector<std::pair<NoteColor, std::vector<Note>>>& notes) {
    auto& inst = Instance();
    if (inst.display_enable_.load(std::memory_order_acquire)) {
        if (!inst.window_created_) {
            cv::namedWindow(kWindowName, cv::WINDOW_NORMAL);
            spdlog::info("Screen display started");
            inst.window_created_ = true;
        }

        auto notes_empty = [&notes]() -> bool {
            for (const auto& each : notes) {
                if (!each.second.empty()) {
                    return false;
                }
            }
            return true;
        };

        auto touch_points = touch.GetCurrentTouchPoints();
        cv::Mat img =
            notes_empty() && touch_points.empty() ? frame : frame.clone();
        for (const auto& [color_enum, notes] : notes) {
            cv::Scalar color;
            switch (color_enum) {
                case NoteColor::Yellow: color = {0, 255, 255}; break;
                case NoteColor::Blue: color = {255, 0, 0}; break;
                case NoteColor::Green: color = {0, 255, 0}; break;
                case NoteColor::Red: color = {0, 0, 255}; break;
            }
            for (const auto& note : notes) {
                cv::rectangle(img, note.box, color, 5);
            }
        }
        for (const auto& p : touch_points) {
            cv::circle(img, p, 30, {0, 0, 255}, -1);
        }
        cv::imshow(kWindowName, img);
        cv::waitKey(1);
        if (cv::getWindowProperty(kWindowName, cv::WND_PROP_VISIBLE) == 1) {
            return true;
        }
    }
    if (inst.window_created_) {
        inst.CloseWindow();
    }
    return false;
}

DisplayManager& DisplayManager::Instance() {
    static DisplayManager instance;
    return instance;
}

void DisplayManager::CloseWindow() {
	try {
		cv::destroyWindow(kWindowName);
	} catch (...) {
	}
	emit displayOff();
	spdlog::info("Screen display stopped");
	window_created_ = false;
}

} // namespace psh