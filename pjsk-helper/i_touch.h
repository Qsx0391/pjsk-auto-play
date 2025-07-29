#pragma once

#ifndef I_TOUCH_H_
#define I_TOUCH_H_

#include <memory>
#include <queue>
#include <mutex>
#include <thread>

#include "framework.h"

namespace psh {

class ITouch {
public:
    ITouch();
    virtual ~ITouch();

    virtual void TouchDown(Point pos, int slot_id) = 0;
    virtual void TouchUp(int slot_id) = 0;
    virtual void TouchMove(Point pos, int slot_id) { TouchDown(pos, slot_id); }

    void TouchTap(Point pos, int slot_id, int duration_ms);
    void TouchTapAsyn(Point pos, int slot_id, int duration_ms);
    void DelayTouchTap(int delay_ms, Point pos, int slot_id,
                       int duration_ms = 20);

    void TouchSlide(Point from, Point to, int slot_id, int duration_ms = 100,
                    int step_delay_ms = 5, double slope_in = 1,
                    double slope_out = 1);
    void TouchSlideAsyn(Point from, Point to, int slot_id,
                        int duration_ms = 100, int step_delay_ms = 5,
                        double slope_in = 1, double slope_out = 1);
    void DelayTouchSlide(int delay_ms, Point from, Point to, int slot_id,
                         int duration_ms = 100, int step_delay_ms = 5,
                         double slope_in = 1, double slope_out = 1);

private:
    enum class TouchType {
        Down,
		Up,
		Move
    };

    struct TouchTask {
        int64_t execute_time_ns;
        Point pos;
        int slot_index;
        TouchType type;

        bool operator<(const TouchTask& other) const {
            return execute_time_ns > other.execute_time_ns;
        }
    };

    void AddTouchTask(int64_t execute_time_ns, Point pos, int slot_index, TouchType type);
    void ProcessTouch(const TouchTask& task);
    void ProcessTouchTasksLoop();

    std::priority_queue<TouchTask> touch_tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;

    std::atomic_bool run_flag_ = true;
    std::thread touch_thread_;
};

class IKeyboard {
public:
    virtual void KeyDown(int key) = 0;
    virtual void KeyUp(int key) = 0;
    virtual void KeyTap(int key, int duration_ms);

    virtual ~IKeyboard() = default;
};

} // namespace psh

#endif // I_TOUCH_H_