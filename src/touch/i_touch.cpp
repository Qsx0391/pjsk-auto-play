#include "i_touch.h"

#include <thread>
#include <cmath>

#include <spdlog/spdlog.h>

#include "common/time_utils.h"

namespace {

double CubicSpline(double slope_0, double slope_1, double t) {
    const double a = slope_0;
    const double b = -(2 * slope_0 + slope_1 - 3);
    const double c = -(-slope_0 - slope_1 + 2);
    return a * t + b * std::pow(t, 2) + c * std::pow(t, 3);
};

int Lerp(int a, int b, double t) { return static_cast<int>(a + (b - a) * t); };

} // namespace

namespace psh {

void TouchTaskStream::AddTask(int64_t execute_time_ms, TouchAction action,
                              cv::Point pos) {
    if (tasks_.empty()) {
        if (action == TouchAction::Up || action == TouchAction::Move) {
            throw std::runtime_error("First task must be TouchAction::Down");
        }
    } else {
        TouchAction prev = tasks_.back().action;
        if ((action == TouchAction::Down && prev != TouchAction::Up) ||
            (action == TouchAction::Move && prev == TouchAction::Up) ||
            (action == TouchAction::Up && prev == TouchAction::Up)) {
            throw std::runtime_error("Invalid touch action sequence");
        }
    }
    tasks_.push_back(TouchTask{action, pos, MsToNs(execute_time_ms)});
}

void TouchTaskStream::AddTap(int64_t execute_time_ms, cv::Point pos,
                             int duration_ms) {
    AddTask(execute_time_ms, TouchAction::Down, pos);
    AddTask(execute_time_ms + duration_ms, TouchAction::Up, {});
}

void TouchTaskStream::AddSlide(int64_t execute_time_ms, cv::Point from,
                               cv::Point to, int duration_ms, int step_delay_ms,
                               double slope_in, double slope_out,
                               bool touch_down, bool touch_up) {
    if (touch_down) {
        AddTask(execute_time_ms, TouchAction::Down, from);
    }
    for (int64_t cur_duration = step_delay_ms; cur_duration < duration_ms;
         cur_duration += step_delay_ms) {
        double progress =
            CubicSpline(slope_in, slope_out,
                        static_cast<double>(cur_duration) / duration_ms);
        int cur_x = static_cast<int>(Lerp(from.x, to.x, progress));
        int cur_y = static_cast<int>(Lerp(from.y, to.y, progress));
        AddTask(execute_time_ms + cur_duration, TouchAction::Move,
                cv::Point{cur_x, cur_y});
    }
    if (touch_up) {
        AddTask(execute_time_ms + duration_ms, TouchAction::Up, {});
    }
}

TouchExecutor::~TouchExecutor() { Shutdown(true); }

void TouchExecutor::SetBaseTime(int64_t base_time_ms) {
    base_time_ns_ = MsToNs(base_time_ms);
    cv_.notify_one();
}

bool TouchExecutor::Start() {
    int expected = kStop;
    if (touch_thread_.joinable() ||
        !run_flag_.compare_exchange_strong(expected, kRun)) {
        return false;
    }
    touch_thread_ = std::thread(&TouchExecutor::ProcessTouchTasksLoop, this);
    return true;
}

void TouchExecutor::Shutdown(bool force) {
    if (!force) {
        run_flag_.store(kShutdown, std::memory_order_release);
        cv_.notify_one();
        if (touch_thread_.joinable()) {
            touch_thread_.join();
        }
    } else {
        Stop();
    }
    std::lock_guard<std::mutex> lock(mutex_);
    for (const int slot : slots_) {
        touch_.TouchUp(slot);
    }
    while (!touch_tasks_.empty()) {
        touch_tasks_.pop();
    }
}

void TouchExecutor::Stop() {
    run_flag_.store(kStop, std::memory_order_release);
    cv_.notify_one();
    if (touch_thread_.joinable()) {
        touch_thread_.join();
    }
}

void TouchExecutor::Execute(std::unique_ptr<TouchTaskStream> task_stream) {
    if (task_stream->Empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    bool notify = touch_tasks_.empty() ||
                  task_stream->GetCurrent().execute_time_ns <
                      touch_tasks_.top()->GetCurrent().execute_time_ns;
    touch_tasks_.push(std::move(task_stream));
    if (notify) {
        cv_.notify_all();
    }
}

void TouchExecutor::Execute(const TouchTaskStream &task_stream) {
    Execute(std::make_unique<TouchTaskStream>(task_stream));
}

void TouchExecutor::Execute(TouchTaskStream &&task_stream) {
    Execute(std::make_unique<TouchTaskStream>(std::move(task_stream)));
}

void TouchExecutor::TouchTap(int64_t execute_time_ms, cv::Point pos,
                             int duration_ms) {
    auto task_stream = std::make_unique<TouchTaskStream>();
    task_stream->AddTap(execute_time_ms, pos, duration_ms);
    Execute(std::move(task_stream));
}

void TouchExecutor::TouchSlide(int64_t execute_time_ms, cv::Point from,
                               cv::Point to, int duration_ms, int step_delay_ms,
                               double slope_in, double slope_out) {
    auto task_stream = std::make_unique<TouchTaskStream>();
    task_stream->AddSlide(execute_time_ms, from, to, duration_ms, step_delay_ms,
                          slope_in, slope_out);
    Execute(std::move(task_stream));
}

int64_t TouchExecutor::GetExecuteTime(const TouchTaskStream &task) const {
    return base_time_ns_ + task.GetCurrent().execute_time_ns;
}

void TouchExecutor::ProcessTouch(TouchTaskStream &task_stream,
                                 int64_t cur_time_ns) {
    do {
        const auto &curr = task_stream.GetCurrent();
        switch (curr.action) {
            case TouchAction::Down: {
                int slot = touch_.TouchDown(curr.pos);
                if (slot == -1) {
                    task_stream.cur_index_ = task_stream.tasks_.size();
                    return;
                } else {
                    task_stream.slot_index_ = slot;
                }
                slots_.insert(slot);
                break;
            }
            case TouchAction::Up: {
                int slot = task_stream.slot_index_;
                touch_.TouchUp(slot);
                slots_.erase(slot);
                break;
            }
            case TouchAction::Move:
                touch_.TouchMove(task_stream.slot_index_, curr.pos);
                break;
        }
        task_stream.Next();
    } while (task_stream.HasNext() &&
             cur_time_ns >= GetExecuteTime(task_stream));
}

void TouchExecutor::ProcessTouchTasksLoop() {
    std::unique_lock<std::mutex> lock(mutex_);
    int run_flag;
    while ((run_flag = run_flag_.load(std::memory_order_acquire)) != kStop) {
        if (touch_tasks_.empty()) {
            if (run_flag == kShutdown) {
                return;
            }
            cv_.wait(lock);
            continue;
        }

        int64_t cur_time_ns = GetCurrentTimeNs();
        if (GetExecuteTime(*touch_tasks_.top()) > cur_time_ns) {
            cv_.wait_for(
                lock, std::chrono::nanoseconds(
                          GetExecuteTime(*touch_tasks_.top()) - cur_time_ns));
            continue;
        }

        do {
            std::shared_ptr<TouchTaskStream> task_stream = touch_tasks_.top();
            touch_tasks_.pop();
            ProcessTouch(*task_stream, cur_time_ns);
            if (task_stream->HasNext()) {
                touch_tasks_.push(std::move(task_stream));
            }
        } while (!touch_tasks_.empty() &&
                 GetExecuteTime(*touch_tasks_.top()) <=
                     (cur_time_ns = GetCurrentTimeNs()));
    }
}

TouchController::TouchController(ITouch &touch) : touch_(touch) {
    for (int slot : touch.GetSupportedSlots()) {
        unused_slots_.push(slot);
        slot_pos_map_[slot] = kUnusedSlotPos;
    }
}

TouchExecutor TouchController::CreateExecutor() { return TouchExecutor(*this); }

std::vector<cv::Point> TouchController::GetCurrentTouchPoints() {
    std::vector<cv::Point> points;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &pair : slot_pos_map_) {
        if (pair.second != kUnusedSlotPos) {
            points.push_back(pair.second);
        }
    }
    return points;
}

int TouchController::TouchDown(cv::Point pos) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (unused_slots_.empty()) {
        spdlog::warn("No available slots for touch");
        return -1;
    }
    int slot = unused_slots_.front();
    touch_.TouchDown(slot, pos);
    unused_slots_.pop();
    slot_pos_map_[slot] = pos;
    spdlog::debug("Touch down at slot {} position ({}, {})", slot, pos.x,
                  pos.y);
    return slot;
}

void TouchController::TouchUp(int slot) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = slot_pos_map_.find(slot);
    if (it == slot_pos_map_.end()) {
        spdlog::warn("Slot {} is not supported", slot);
        return;
    }
    if (it->second == kUnusedSlotPos) {
        spdlog::warn("Slot {} is not in use", slot);
        return;
    }
    touch_.TouchUp(slot);
    it->second = kUnusedSlotPos;
    unused_slots_.push(slot);
    spdlog::debug("Touch up at slot {}", slot);
}

void TouchController::TouchMove(int slot, cv::Point pos) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = slot_pos_map_.find(slot);
    if (it == slot_pos_map_.end()) {
        spdlog::warn("Slot {} is not supported", slot);
        return;
    }
    if (it->second == kUnusedSlotPos) {
        spdlog::warn("Slot {} is not in use", slot);
        return;
    }
    touch_.TouchMove(slot, pos);
    it->second = pos;
    spdlog::debug("Touch move at slot {} position ({}, {})", slot, pos.x,
                  pos.y);
}

void TouchController::TouchTap(cv::Point pos, int duration_ms) {
    int slot = TouchDown(pos);
    if (slot != -1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
        TouchUp(slot);
    }
}

} // namespace psh