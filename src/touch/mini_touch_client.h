#pragma once

#ifndef PSH_TOUCH_MINI_TOUCH_CLIENT_H_
#define PSH_TOUCH_MINI_TOUCH_CLIENT_H_

#include <string>
#include <mutex>
#include <qstring.h>

#include <opencv2/opencv.hpp>

#include "touch/i_touch.h"

namespace psh {

class MiniTouchClient;

class MiniTouchCommand {
public:
    MiniTouchCommand(MiniTouchClient& touch);

    MiniTouchCommand& U(int slot_index);
    MiniTouchCommand& D(int slot_index, cv::Point pos);
    MiniTouchCommand& M(int slot_index, cv::Point pos);
    MiniTouchCommand& C();
    void Send();

private:
    MiniTouchClient& touch_;
    std::string buffer_;
};

class MiniTouchClient : public ITouch {
public:
    inline static const std::vector<int> kSupportedSlots = {0, 1, 2, 3, 4,
                                                            5, 6, 7, 8, 9};

    MiniTouchClient(const QString& host = "127.0.0.1", int port = 16384,
                    int screen_height = 720);
    virtual ~MiniTouchClient();

    void TouchDown(int slot_index, cv::Point pos) override;
    void TouchUp(int slot_index) override;
    void TouchMove(int slot_index, cv::Point pos) override;
    const std::vector<int>& GetSupportedSlots() const override {
        return kSupportedSlots;
    }

    static void StartMiniTouchService(const QString& mumu_path, int adb_port,
                                      int service_port);

    void Close();
    void SendCommand(const std::string& cmd);
    MiniTouchCommand Command();

private:
    int TouchY(int y) const { return screen_height_ - y; }

    friend class MiniTouchCommand;

    int sock_ = -1;
    int screen_height_;
    std::mutex send_mutex_;
};

} // namespace psh

#endif // !PSH_TOUCH_MINI_TOUCH_CLIENT_H_