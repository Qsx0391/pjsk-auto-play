#pragma once

#ifndef MINI_TOUCH_CLIENT_H_
#define MINI_TOUCH_CLIENT_H_

#include <string>
#include <mutex>
#include <qstring.h>

#include <opencv2/opencv.hpp>

#include "i_touch.h"

namespace psh {

class MiniTouchClient : public ITouch {
public:
    static const int kSlotIndexBegin = 0;
    static const int kSlotIndexEnd = 9;

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

    MiniTouchClient(const QString& host = "127.0.0.1", int port = 16384,
                    int screen_height = 720);
    virtual ~MiniTouchClient();

    int TouchDown(cv::Point pos) override;
    void TouchUp(int slot_index) override;
    void TouchMove(cv::Point pos, int slot_index) override;

    void Close();

    static void StartMiniTouchService(const QString& mumu_path, int adb_port,
                                      int service_port);

    void SendCommand(const std::string& cmd);
    MiniTouchCommand Command();

private:
    int TouchY(int y) const { return screen_height_ - y; }

    int sock_ = -1;
    int screen_height_;
    std::mutex send_mutex_;
};

} // namespace psh

#endif // MINI_TOUCH_CLIENT_H_