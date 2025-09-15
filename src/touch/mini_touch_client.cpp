#include "mini_touch_client.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

#include <spdlog/spdlog.h>
#include <common/command.h>

#pragma comment(lib, "ws2_32.lib")

namespace psh {

MiniTouchCommand::MiniTouchCommand(MiniTouchClient& touch)
    : touch_(touch) {}

MiniTouchCommand& MiniTouchCommand::U(
    int tid) {
    buffer_ += "u " + std::to_string(tid) + "\n";
    return *this;
}

MiniTouchCommand& MiniTouchCommand::D(
    int tid, cv::Point pos) {
    buffer_ += "d " + std::to_string(tid) + " " +
               std::to_string(touch_.TouchY(pos.y)) + " " +
               std::to_string(pos.x) + " 50\n";
    return *this;
}

MiniTouchCommand& MiniTouchCommand::M(
    int tid, cv::Point pos) {
    buffer_ += "m " + std::to_string(tid) + " " +
               std::to_string(touch_.TouchY(pos.y)) + " " +
               std::to_string(pos.x) + " 50\n";
    return *this;
}

MiniTouchCommand& MiniTouchCommand::C() {
    buffer_ += "c\n";
    return *this;
}

MiniTouchClient::MiniTouchClient(const QString& host, int port,
                                 int screen_height)
    : screen_height_(screen_height) {
    WSADATA wsa_data;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (ret != 0) {
        spdlog::error("WSAStartup failed with error: {}", ret);
        return;
    }

    sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_ == INVALID_SOCKET) {
        spdlog::warn("Socket creation failed");
        return;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host.toUtf8().constData(), &server_addr.sin_addr);

    if (connect(sock_, (sockaddr*)&server_addr, sizeof(server_addr)) ==
        SOCKET_ERROR) {
        spdlog::warn("Socket [{}:{}] creation failed",
                     host.toUtf8().constData(), port);
        closesocket(sock_);
    }
}

MiniTouchClient::~MiniTouchClient() {
    Close();
    WSACleanup();
}

void MiniTouchClient::TouchDown(int slot_index, cv::Point pos) {
    if (slot_index == -1) {
        MiniTouchCommand(*this).D(slot_index, pos).C().Send();
    }
}

void MiniTouchClient::TouchUp(int slot_index) {
    if (slot_index != -1) {
        MiniTouchCommand(*this).U(slot_index).C().Send();
    }
}

void MiniTouchClient::TouchMove(int slot_index, cv::Point pos) {
    if (slot_index != -1) {
        MiniTouchCommand(*this).M(slot_index, pos).Send();
    }
}

void MiniTouchClient::Close() {
    if (sock_ != INVALID_SOCKET) {
        closesocket(sock_);
        sock_ = INVALID_SOCKET;
    }
    sock_ = INVALID_SOCKET;
}

void MiniTouchClient::StartMiniTouchService(const QString& mumu_path,
                                            int adb_port, int service_port) {
    QString adb_path = mumu_path + "/shell/adb.exe";

    auto KillADB = [&]() -> bool {
        QString result =
            ExecCommand("cmd.exe", {"/C", "netstat -ano | findstr 5037"});
        if (result.isEmpty()) {
            return false;
        }
        QString pid = result.split(" ").last().trimmed();
        ExecCommand("taskkill.exe", {"/F", "/PID", pid});
        return true;
    };

    auto ConnectADB = [&](bool retry) {
        QString result = ExecCommand(
            adb_path, {"connect", QString("127.0.0.1:%1").arg(adb_port)});
        if (retry && result.indexOf("connection reset") != -1 && KillADB()) {
            result = ExecCommand(
                adb_path, {"connect", QString("127.0.0.1:%1").arg(adb_port)});
        }
        if (result.indexOf("connected to") == -1) {
            spdlog::error("ADB connect failed: {}",
                          result.toUtf8().constData());
            throw std::runtime_error("ADB connect failed");
        }
        ExecCommand(adb_path, {"forward", QString("tcp:%1").arg(service_port),
                               "localabstract:minitouch"});
    };

    auto CheckMiniTouchStarted = [&]() -> bool {
        QString result = ExecCommand(
            adb_path, {"shell", "ps -ef | grep minitouch | grep -v grep"});
        if (result.indexOf("minitouch") != -1) {
            spdlog::info("MiniTouch is already running");
            return true;
        }
        return false;
    };

    auto StartMiniTouch = [&]() {
        ExecCommand(
            adb_path,
            {"shell", "nohup /data/local/tmp/minitouch > /dev/null 2>&1 &"});
    };

    ConnectADB(true);
    if (CheckMiniTouchStarted()) {
        return;
    }
    StartMiniTouch();
    if (!CheckMiniTouchStarted()) {
        spdlog::error("Failed to start MiniTouch service");
        throw std::runtime_error("Failed to start MiniTouch service");
    }
}

void MiniTouchClient::SendCommand(const std::string& cmd) {
    std::lock_guard<std::mutex> lock(send_mutex_);
    int ret = send(sock_, cmd.c_str(), static_cast<int>(cmd.size()), 0);
    if (ret == SOCKET_ERROR) {
        spdlog::error("Send command failed: {}", cmd);
    }
}

MiniTouchCommand MiniTouchClient::Command() {
    return MiniTouchCommand(*this);
}

void MiniTouchCommand::Send() {
    touch_.SendCommand(buffer_);
    buffer_.clear();
}

} // namespace psh