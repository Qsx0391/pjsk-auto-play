#pragma once

#ifndef I_TOUCH_H_
#define I_TOUCH_H_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <queue>
#include <mutex>
#include <thread>

#include <opencv2/opencv.hpp>
#include <unordered_set>

namespace psh {

class ITouch {
public:
    ITouch(int slot_index_begin, int slot_index_end);
    virtual ~ITouch();

    int GetSlot();
    void ReleaseSlot(int slot_index);

    virtual int TouchDown(cv::Point pos) = 0;
    virtual void TouchDown(int slot_id, cv::Point pos) = 0;
    virtual void TouchUp(int slot_id) = 0;
    virtual void TouchMove(cv::Point pos, int slot_id) = 0;

    void TouchUpAll();

    void TouchTap(cv::Point pos, int duration_ms = 20);
    void TouchTapAsyn(cv::Point pos, int duration_ms = 20, int delay_ms = 0);

    void TouchSlide(cv::Point from, cv::Point to, int duration_ms = 100,
                    int step_delay_ms = 5, double slope_in = 1,
                    double slope_out = 1);
    void TouchSlideAsyn(cv::Point from, cv::Point to, int duration_ms = 100,
                        int step_delay_ms = 5, double slope_in = 1,
                        double slope_out = 1, int delay_ms = 0);

private:
    enum class TouchType { Down, Up, Move };

    class TouchTask {
    public:
        struct TaskNode {
            TouchType type;
            cv::Point pos;
            int64_t execute_time_ns;
        };

        TouchTask() = default;

        void Add(TouchType type, cv::Point pos, int64_t execute_time_ns) {
            tasks.push_back(TaskNode{type, pos, execute_time_ns});
        }

        bool HasNext() { return cur_index < tasks.size(); }
        void Next() { ++cur_index; }

        void SetSlotIndex(int index) { slot_index = index; }
        int GetSlotIndex() const { return slot_index; }

        const TaskNode* operator->() const { return &tasks[cur_index]; }

        bool operator<(const TouchTask& other) const {
            return (*this)->execute_time_ns > other->execute_time_ns;
        }

    private:
        int cur_index = 0;
        int slot_index = -1;
        std::vector<TaskNode> tasks;
    };

    struct TouchTaskComp {
        bool operator()(const std::unique_ptr<TouchTask>& lhs,
                        const std::unique_ptr<TouchTask>& rhs) const {
            return *lhs < *rhs;
        }
    };

    void AddTouchTask(std::unique_ptr<TouchTask> task);
    void ProcessTouch(TouchTask& task, int64_t cur_time_ns);
    void ProcessTouchTasksLoop();

    std::priority_queue<std::unique_ptr<TouchTask>,
                        std::vector<std::unique_ptr<TouchTask>>, TouchTaskComp>
        touch_tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;

    std::atomic_bool run_flag_ = true;
    std::thread touch_thread_;

    const int slot_index_begin_;
    const int slot_index_end_;
    std::unordered_set<int> unused_slots_;
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