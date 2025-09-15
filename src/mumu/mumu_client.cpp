#include "mumu/mumu_client.h"

#include <spdlog/spdlog.h>

namespace psh {
using MLL = MumuLibLoader;

MumuClient::MumuClient(const QString& mumu_path_str, int mumu_inst_index,
                       const QString& package_name)
    : mumu_path_(mumu_path_str.toUtf8().constData()),
      mumu_inst_index_(mumu_inst_index),
      package_name_(package_name) {
    Init();
}

MumuClient::~MumuClient() { Uninit(); }

bool MumuClient::Init() {
    inited_ = MLL::Init(mumu_path_) &&
              ConnectMumu(mumu_path_, mumu_inst_index_) && InitScreencap();
    return inited_;
}

void MumuClient::Uninit() {
    inited_ = false;
    if (mumu_handle_ != 0) {
        MLL::Disconnect(mumu_handle_);
        mumu_handle_ = 0;
    }
}

cv::Mat MumuClient::Capture() {
    int display_id = GetDisplayId();
    int ret = MLL::CaptureDisplay(
        mumu_handle_, display_id, static_cast<int>(display_buffer_.size()),
        &display_width_, &display_height_, display_buffer_.data());
    if (ret) {
        if (!Init()) {
            throw std::runtime_error("Failed to capture display");
        }
        display_id = GetDisplayId();
        ret = MLL::CaptureDisplay(
            mumu_handle_, display_id, static_cast<int>(display_buffer_.size()),
            &display_width_, &display_height_, display_buffer_.data());
        if (ret) {
			throw std::runtime_error("Failed to capture display");
		}
    }

    cv::Mat raw(display_height_, display_width_, CV_8UC4,
                display_buffer_.data());
    cv::Mat bgr;
    cv::cvtColor(raw, bgr, cv::COLOR_RGBA2BGR);
    cv::Mat dst;
    cv::flip(bgr, dst, 0);

    return dst;
}

void MumuClient::TouchDown(int slot_index, cv::Point pos) {
    if (slot_index != -1) {
        MLL::InputEventFingerTouchDown(mumu_handle_, GetDisplayId(), slot_index,
                                       pos.x, pos.y);
    }
}

void MumuClient::TouchUp(int slot_index) {
    if (slot_index != -1) {
        MLL::InputEventFingerTouchUp(mumu_handle_, GetDisplayId(), slot_index);
    }
}

void MumuClient::TouchMove(int slot_index, cv::Point pos) {
    TouchDown(slot_index, pos);
}

void MumuClient::KeyDown(int key) {
    MLL::InputEventKeyDown(mumu_handle_, GetDisplayId(), key);
}

void MumuClient::KeyUp(int key) {
    MLL::InputEventKeyUp(mumu_handle_, GetDisplayId(), key);
}

bool MumuClient::ConnectMumu(const std::filesystem::path& mumu_path,
                             int mumu_inst_index) {
    mumu_handle_ = MLL::Connect(mumu_path.c_str(), mumu_inst_index);

    if (mumu_handle_ == 0) {
        spdlog::error("Failed to connect mumu. path: {}, inst index: {}",
                      mumu_path.string(), mumu_inst_index);
        return false;
    }

    return true;
}

int MumuClient::GetDisplayId() {
    return MLL::GetDisplayId(mumu_handle_, package_name_.toUtf8().constData(),
                             0);
}

bool MumuClient::InitScreencap() {
    int display_id = GetDisplayId();
    if (display_id < 0) {
        spdlog::error("Failed to get display id from {}",
                      package_name_.toUtf8().constData());
        return false;
    }

    int ret = MLL::CaptureDisplay(mumu_handle_, display_id, 0, &display_width_,
                                  &display_height_, nullptr);
    if (ret) {
        spdlog::error("Failed to capture display. code: {}", ret);
        return false;
    }

    display_buffer_.resize(display_width_ * display_height_ * 4);
    return true;
}

} // namespace psh