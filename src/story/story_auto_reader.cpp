#include "story/story_auto_reader.h"

#include <spdlog/spdlog.h>

#include "common/finalizer.hpp"

namespace psh {

StoryAutoReader::StoryAutoReader(TouchController &touch) : touch_(touch) {}

StoryAutoReader::~StoryAutoReader() { Stop(); }

bool StoryAutoReader::Start() {
    if (run_flag_.exchange(true, std::memory_order_acquire)) {
        spdlog::info("Auto read already running");
        return false;
    }
    if (worker_.joinable()) {
        worker_.join();
    }
    worker_ = std::thread(&StoryAutoReader::MainLoop, this);

    return true;
}

void StoryAutoReader::Stop() {
    run_flag_.store(false, std::memory_order_release);
    if (worker_.joinable()) {
        worker_.join();
    }
}

void StoryAutoReader::MainLoop() {
    try {
        spdlog::info("Auto read started");
        Finalizer finalizer([this]() {
            emit stopped();
            spdlog::info("Auto read stopped");
        });
        while (run_flag_.load(std::memory_order_acquire)) {
            touch_.TouchTap(kAutoClickPoint);
            std::this_thread::sleep_for(
                std::chrono::milliseconds(click_delay_ms));
        }
    } catch (const std::exception &e) {
        spdlog::error("Auto read error: {}", e.what());
	}
}

} // namespace psh