#include "i_touch.h"

#include <thread>
#include <cmath>

#include "spdlog/spdlog.h"

namespace {

int64_t GetCurrentTimeNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

int64_t MsToNs(int ms) { return static_cast<int64_t>(ms) * 1'000'000; }

double CubicSpline(double slope_0, double slope_1, double t) {
    const double a = slope_0;
    const double b = -(2 * slope_0 + slope_1 - 3);
    const double c = -(-slope_0 - slope_1 + 2);
    return a * t + b * std::pow(t, 2) + c * std::pow(t, 3);
};

int Lerp(int a, int b, double t) { return static_cast<int>(a + (b - a) * t); };

} // namespace

namespace psh {

ITouch::ITouch(int slot_index_begin, int slot_index_end)
    : slot_index_begin_(slot_index_begin),
      slot_index_end_(slot_index_end),
      unused_slots_(slot_index_end - slot_index_begin + 1) {
    for (int i = slot_index_begin; i <= slot_index_end; ++i) {
        unused_slots_.insert(i);
    }
    touch_thread_ = std::thread([this]() { ProcessTouchTasksLoop(); });
}

ITouch::~ITouch() {
    run_flag_ = false;
    cv_.notify_all();
    if (touch_thread_.joinable()) {
        touch_thread_.join();
    }
}

int ITouch::GetSlot() {
    if (unused_slots_.empty()) {
        return -1;
    }
    auto slot = unused_slots_.begin();
    int slot_index = *slot;
    unused_slots_.erase(slot);
    return slot_index;
}

void ITouch::ReleaseSlot(int slot_index) { unused_slots_.insert(slot_index); }

void ITouch::TouchUpAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    touch_tasks_ = {};
    for (int i = slot_index_begin_; i <= slot_index_end_; ++i) {
        if (unused_slots_.find(i) == unused_slots_.end()) {
            TouchUp(i);
        }
    }
}

void ITouch::TouchTap(cv::Point pos, int duration_ms) {
    int slot_index = TouchDown(pos);
    if (slot_index == -1) {
        throw std::runtime_error("No available slots for touch");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    TouchUp(slot_index);
}

void ITouch::TouchTapAsyn(cv::Point pos, int duration_ms, int delay_ms) {
    int64_t execute_time_ns = GetCurrentTimeNs() + MsToNs(delay_ms);
    auto task = std::make_unique<TouchTask>();
    task->Add(TouchType::Down, pos, execute_time_ns);
    task->Add(TouchType::Up, cv::Point{},
              execute_time_ns + MsToNs(duration_ms));
    AddTouchTask(std::move(task));
}

void psh::ITouch::TouchSlide(cv::Point from, cv::Point to, int duration_ms,
                             int step_delay_ms, double slope_in,
                             double slope_out) {
    int slot_index = TouchDown(from);
    if (slot_index == -1) {
        throw std::runtime_error("No available slots for touch");
    }
    for (int cur_duration_ms = step_delay_ms; cur_duration_ms <= duration_ms;
         cur_duration_ms += step_delay_ms) {
        double progress =
            CubicSpline(slope_in, slope_out,
                        static_cast<double>(cur_duration_ms) / duration_ms);
        int cur_x = static_cast<int>(Lerp(from.x, to.x, progress));
        int cur_y = static_cast<int>(Lerp(from.y, to.y, progress));
        std::this_thread::sleep_for(std::chrono::milliseconds(step_delay_ms));
        TouchMove({cur_x, cur_y}, slot_index);
    }
    TouchUp(slot_index);
}

void ITouch::TouchSlideAsyn(cv::Point from, cv::Point to, int duration_ms,
                            int step_delay_ms, double slope_in,
                            double slope_out, int delay_ms) {
    int64_t execute_time_ns = GetCurrentTimeNs() + MsToNs(delay_ms);
    int64_t step_delay_ns = MsToNs(step_delay_ms);
    int64_t duration_ns = MsToNs(duration_ms);
    auto task = std::make_unique<TouchTask>();
    task->Add(TouchType::Down, from, execute_time_ns);
    for (int cur_duration_ns = step_delay_ns; cur_duration_ns <= duration_ns;
         cur_duration_ns += step_delay_ns) {
        double progress =
            CubicSpline(slope_in, slope_out,
                        static_cast<double>(cur_duration_ns) / duration_ns);
        int cur_x = static_cast<int>(Lerp(from.x, to.x, progress));
        int cur_y = static_cast<int>(Lerp(from.y, to.y, progress));
        task->Add(TouchType::Move, cv::Point{cur_x, cur_y},
                  execute_time_ns + cur_duration_ns);
    }
    task->Add(TouchType::Up, cv::Point{}, execute_time_ns + duration_ns);
    AddTouchTask(std::move(task));
}

void ITouch::AddTouchTask(std::unique_ptr<TouchTask> task) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (touch_tasks_.empty() ||
        (*task)->execute_time_ns < (*touch_tasks_.top())->execute_time_ns) {
        cv_.notify_one();
    }
    touch_tasks_.push(std::move(task));
}

void ITouch::ProcessTouch(TouchTask &task, int64_t cur_time_ns) {
    while (task.HasNext() && cur_time_ns >= task->execute_time_ns) {
        switch (task->type) {
            case TouchType::Down:
                task.SetSlotIndex(TouchDown(task->pos));
                if (task.GetSlotIndex() != -1) {
                    spdlog::info("Touch down at ({}, {}) on slot {}",
                                 task->pos.x, task->pos.y, task.GetSlotIndex());
                } else {
                    do {
                        task.Next();
                    } while (task.HasNext() && task->type != TouchType::Down);
                    spdlog::warn("No available slots for touch");
                }
                break;
            case TouchType::Up:
                if (task.GetSlotIndex() != -1) {
                    TouchUp(task.GetSlotIndex());
                }
                break;
            case TouchType::Move:
                if (task.GetSlotIndex() != -1) {
                    TouchMove(task->pos, task.GetSlotIndex());
                }
                break;
        }
        task.Next();
    }
}

void ITouch::ProcessTouchTasksLoop() {
    while (run_flag_) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (touch_tasks_.empty()) {
            cv_.wait(lock);
            continue;
        }

        auto cur_time_ns = GetCurrentTimeNs();
        if ((*touch_tasks_.top())->execute_time_ns > cur_time_ns) {
            cv_.wait_for(lock, std::chrono::nanoseconds(
                                   (*touch_tasks_.top())->execute_time_ns -
                                   cur_time_ns));
            continue;
        }

        auto task = std::move(const_cast<std::unique_ptr<TouchTask>&>(touch_tasks_.top()));
        touch_tasks_.pop();
        ProcessTouch(*task, cur_time_ns);
        if (task->HasNext()) {
            touch_tasks_.push(std::move(task));
        }
        lock.unlock();
    }
}

void IKeyboard::KeyTap(int key, int duration_ms) {
    std::thread([=]() {
        KeyDown(key);
        std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
        KeyUp(key);
    }).detach();
}

} // namespace psh