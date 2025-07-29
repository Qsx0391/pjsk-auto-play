#pragma once

#ifndef MINI_TOUCH_CLIENT_H_
#define MINI_TOUCH_CLIENT_H_

#include <string>
#include <mutex>
#include <qstring.h>

#include "i_touch.h"
#include "framework.h"

namespace psh {

class MiniTouchClient : public ITouch {
public:
    class MiniTouchCommand {
    public:
        MiniTouchCommand(MiniTouchClient& touch);

        MiniTouchCommand& U(int slot_id);
        MiniTouchCommand& D(int slot_id, Point pos);
        MiniTouchCommand& M(int slot_id, Point pos);
        MiniTouchCommand& C();
        void Send();

    private:
        MiniTouchClient& touch_;
        std::string buffer_;
    };

    MiniTouchClient(const QString& host = "127.0.0.1", int port = 16384,
                    int screen_height = 720);
    virtual ~MiniTouchClient();

    void TouchDown(Point pos, int slot_id) override;
    void TouchUp(int slot_id) override;
    void TouchMove(Point pos, int slot_id) override;

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