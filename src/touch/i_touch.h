#pragma once

#ifndef PSH_TOUCH_I_TOUCH_H_
#define PSH_TOUCH_I_TOUCH_H_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include <opencv2/opencv.hpp>

namespace psh {

enum class TouchAction { Down, Up, Move };

struct TouchTask {
    TouchAction action;
    cv::Point pos;
    int64_t execute_time_ns;
};

class ITouch {
public:
    virtual ~ITouch() {}
    virtual void TouchDown(int slot_id, cv::Point pos) = 0;
    virtual void TouchUp(int slot_id) = 0;
    virtual void TouchMove(int slot_id, cv::Point pos) = 0;
    virtual const std::vector<int>& GetSupportedSlots() const = 0;
};

class TouchExecutor;
class TouchController;

class TouchTaskStream {
public:
    TouchTaskStream() = default;
    TouchTaskStream(const std::vector<TouchTask>& tasks) : tasks_(tasks) {}
    TouchTaskStream(std::vector<TouchTask>&& tasks)
        : tasks_(std::move(tasks)) {}
    TouchTaskStream(const TouchTaskStream& other) = default;
    TouchTaskStream(TouchTaskStream&& other) = default;

    bool Empty() const { return tasks_.empty(); }
    const TouchTask& GetCurrent() const { return tasks_[cur_index_]; }

    void AddTask(int64_t execute_time_ms, TouchAction action, cv::Point pos);
    void AddTap(int64_t execute_time_ms, cv::Point pos, int duration_ms = 20);
    void AddSlide(int64_t execute_time_ms, cv::Point from, cv::Point to,
                  int duration_ms = 100, int step_delay_ms = 5,
                  double slope_in = 1, double slope_out = 1,
                  bool touch_down = true, bool touch_up = true);

private:
    friend class TouchExecutor;

    bool HasNext() { return cur_index_ < tasks_.size(); }
    void Next() { ++cur_index_; }

    int cur_index_ = 0;
    int slot_index_ = -1;
    std::vector<TouchTask> tasks_;
};

class TouchExecutor {
public:
    virtual ~TouchExecutor();

    void SetBaseTime(int64_t base_time_ms);

    bool Start();
    void Shutdown(bool force);
    void Stop();

    void Execute(std::unique_ptr<TouchTaskStream> task_stream);
    void Execute(const TouchTaskStream& task_stream);
    void Execute(TouchTaskStream&& task_stream);

    void TouchTap(int64_t execute_time_ms, cv::Point pos, int duration_ms = 20);
    void TouchSlide(int64_t execute_time_ms, cv::Point from, cv::Point to,
                    int duration_ms = 100, int step_delay_ms = 5,
                    double slope_in = 1, double slope_out = 1);

private:
    friend class TouchController;

    static const int kStop = 0;
    static const int kShutdown = 1;
    static const int kRun = 2;

    TouchExecutor(TouchController& touch) : touch_(touch) {}
    int64_t GetExecuteTime(const TouchTaskStream& task) const;
    void ProcessTouch(TouchTaskStream& task_stream, int64_t cur_time_ns);
    void ProcessTouchTasksLoop();

    TouchController& touch_;

    struct TouchTaskStreamComp {
        bool operator()(const std::shared_ptr<TouchTaskStream>& lhs,
                        const std::shared_ptr<TouchTaskStream>& rhs) const {
            return lhs->GetCurrent().execute_time_ns >
                   rhs->GetCurrent().execute_time_ns;
        }
    };

    std::priority_queue<std::shared_ptr<TouchTaskStream>,
                        std::vector<std::shared_ptr<TouchTaskStream>>,
                        TouchTaskStreamComp>
        touch_tasks_;

    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic_int run_flag_ = kStop;
    std::thread touch_thread_;
    std::unordered_set<int> slots_;

    int64_t base_time_ns_ = 0;
};

class TouchController {
public:
    TouchController(ITouch& touch);
    virtual ~TouchController() {}

    int TouchDown(cv::Point pos);
    void TouchUp(int slot);
    void TouchMove(int slot, cv::Point pos);
    void TouchTap(cv::Point pos, int duration_ms = 20);

    TouchExecutor CreateExecutor();

    std::vector<cv::Point> GetCurrentTouchPoints();

private:
    friend class TouchExecutor;

    inline static const cv::Point kUnusedSlotPos = cv::Point{-1, -1};

    ITouch& touch_;

    std::mutex mutex_;
    std::queue<int> unused_slots_;
    std::unordered_map<int, cv::Point> slot_pos_map_;
};

} // namespace psh

#endif // !PSH_TOUCH_I_TOUCH_H_