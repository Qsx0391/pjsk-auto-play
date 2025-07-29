#include "i_touch.h"

#include <thread>
#include <cmath>

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

ITouch::ITouch() {
    touch_thread_ = std::thread([this]() { ProcessTouchTasksLoop(); });
}

ITouch::~ITouch() {
    run_flag_ = false;
    cv_.notify_all();
    if (touch_thread_.joinable()) {
        touch_thread_.join();
    }
}

void ITouch::TouchTap(Point pos, int slot_id, int duration_ms) {
    TouchDown(pos, slot_id);
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    TouchUp(slot_id);
}

void ITouch::TouchTapAsyn(Point pos, int slot_id, int duration_ms) {
    TouchDown(pos, slot_id);
    AddTouchTask(GetCurrentTimeNs() + MsToNs(duration_ms), Point{}, slot_id,
                 TouchType::Up);
}

void ITouch::DelayTouchTap(int delay_ms, Point pos, int slot_id,
                           int duration_ms) {
    int64_t execute_time_ns = GetCurrentTimeNs() + MsToNs(delay_ms);
    AddTouchTask(execute_time_ns, pos, slot_id, TouchType::Down);
    AddTouchTask(execute_time_ns + MsToNs(duration_ms), Point{}, slot_id,
                 TouchType::Up);
}

void psh::ITouch::TouchSlide(Point from, Point to, int slot_id, int duration_ms,
                             int step_delay_ms, double slope_in,
                             double slope_out) {
    TouchDown(from, slot_id);
    for (int cur_duration_ms = step_delay_ms; cur_duration_ms <= duration_ms;
         cur_duration_ms += step_delay_ms) {
        double progress =
            CubicSpline(slope_in, slope_out,
                        static_cast<double>(cur_duration_ms) / duration_ms);
        int cur_x = static_cast<int>(Lerp(from.x, to.x, progress));
        int cur_y = static_cast<int>(Lerp(from.y, to.y, progress));
        std::this_thread::sleep_for(std::chrono::milliseconds(step_delay_ms));
        TouchMove({cur_x, cur_y}, slot_id);
    }
    TouchUp(slot_id);
}

void ITouch::TouchSlideAsyn(Point from, Point to, int slot_id, int duration_ms,
                            int step_delay_ms, double slope_in,
                            double slope_out) {
    int64_t cur_time_ns = GetCurrentTimeNs();
    int64_t step_delay_ns = MsToNs(step_delay_ms);
    int64_t duration_ns = MsToNs(duration_ms);
    TouchDown(from, slot_id);
    for (int cur_duration_ns = step_delay_ns; cur_duration_ns <= duration_ns;
         cur_duration_ns += step_delay_ns) {
        double progress =
            CubicSpline(slope_in, slope_out,
                        static_cast<double>(cur_duration_ns) / duration_ns);
        int cur_x = static_cast<int>(Lerp(from.x, to.x, progress));
        int cur_y = static_cast<int>(Lerp(from.y, to.y, progress));
        AddTouchTask(cur_time_ns + cur_duration_ns, {cur_x, cur_y}, slot_id,
                     TouchType::Move);
    }
    AddTouchTask(cur_time_ns + duration_ns, Point{}, slot_id, TouchType::Up);
}

void ITouch::DelayTouchSlide(int delay_ms, Point from, Point to, int slot_id,
                             int duration_ms, int step_delay_ms,
                             double slope_in, double slope_out) {
    int64_t execute_time_ns = GetCurrentTimeNs() + MsToNs(delay_ms);
    int64_t step_delay_ns = MsToNs(step_delay_ms);
    int64_t duration_ns = MsToNs(duration_ms);
    AddTouchTask(execute_time_ns, from, slot_id, TouchType::Down);
    for (int cur_duration_ns = step_delay_ns; cur_duration_ns <= duration_ns;
         cur_duration_ns += step_delay_ns) {
        double progress =
            CubicSpline(slope_in, slope_out,
                        static_cast<double>(cur_duration_ns) / duration_ns);
        int cur_x = static_cast<int>(Lerp(from.x, to.x, progress));
        int cur_y = static_cast<int>(Lerp(from.y, to.y, progress));
        AddTouchTask(execute_time_ns + cur_duration_ns, {cur_x, cur_y}, slot_id,
                     TouchType::Move);
    }
    AddTouchTask(execute_time_ns + duration_ns, Point{}, slot_id,
                 TouchType::Up);
}

void ITouch::AddTouchTask(int64_t execute_time_ns, Point pos, int slot_index,
                          TouchType type) {
    std::lock_guard<std::mutex> lock(mutex_);
    touch_tasks_.push(TouchTask{execute_time_ns, pos, slot_index, type});
    if (touch_tasks_.size() == 1 ||
        execute_time_ns < touch_tasks_.top().execute_time_ns) {
        cv_.notify_one();
    }
}

void ITouch::ProcessTouch(const TouchTask &task) {
    switch (task.type) {
        case TouchType::Down: TouchDown(task.pos, task.slot_index); break;
        case TouchType::Up: TouchUp(task.slot_index); break;
        case TouchType::Move: TouchMove(task.pos, task.slot_index); break;
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
        if (touch_tasks_.top().execute_time_ns > cur_time_ns) {
            cv_.wait_for(lock,
                         std::chrono::nanoseconds(
                             touch_tasks_.top().execute_time_ns - cur_time_ns));
            continue;
        }

        auto task = touch_tasks_.top();
        touch_tasks_.pop();
        lock.unlock();

        ProcessTouch(task);
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