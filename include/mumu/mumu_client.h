#pragma once

#ifndef MUMU_CLIENT_H_
#define MUMU_CLIENT_H_

#include <qstring.h>

#include <opencv2/opencv.hpp>

#include "mumu_lib_loader.h"
#include "i_screen.h"
#include "i_touch.h"

namespace psh {

class MumuClient : public IScreen, public ITouch, public IKeyboard {
    using Mumu = MumuLibLoader;

public:
    static const int kSlotIndexBegin = 1;
    static const int kSlotIndexEnd = 10;

    inline static const QString kDefaultPackageName = "default";

    MumuClient(const QString& mumu_path_str, int mumu_inst_index,
               const QString& package_name);
    virtual ~MumuClient();
    void Uninit();
    cv::Mat Screencap() override;
    int GetDisplayWidth() override { return display_width_; }
    int GetDisplayHeight() override { return display_height_; }

    int TouchDown(cv::Point pos) override;
    void TouchDown(int slot_index, cv::Point pos) override;
    void TouchUp(int slot_index) override;
    void TouchMove(cv::Point pos, int slot_index) override;

    void KeyDown(int key) override;
    void KeyUp(int key) override;

private:
    bool ConnectMumu(const std::filesystem::path& mumu_path,
                     int mumu_inst_index);
    int GetDisplayId();
    bool InitScreencap();

    bool inited_ = false;
    int mumu_handle_ = 0;
    int display_width_ = -1;
    int display_height_ = -1;
    QString package_name_;

    std::vector<uint8_t> display_buffer_;
};

} // namespace psh

#endif // MUMU_CLIENT_H_