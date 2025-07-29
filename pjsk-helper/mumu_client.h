#pragma once

#ifndef MUMU_CLIENT_H_
#define MUMU_CLIENT_H_

#include <qstring.h>

#include "opencv2/opencv.hpp"

#include "mumu_lib_loader.h"
#include "i_screen.h"
#include "i_touch.h"
#include "framework.h"

namespace psh {

class MumuClient : public IScreen, public ITouch, public IKeyboard {
    using Mumu = MumuLibLoader;

public:
    MumuClient(const QString& mumu_path_str, int mumu_inst_index,
               const QString& package_name);
    virtual ~MumuClient();
    void Uninit();
    cv::Mat Screencap() override;
    int GetDisplayWidth() override { return display_width_; }
    int GetDisplayHeight() override { return display_height_; }

    void TouchDown(Point pos, int slot_id) override;
    void TouchUp(int slot_id) override;

    void KeyDown(int key) override;
    void KeyUp(int key) override;

private:
    bool ConnectMumu(const std::filesystem::path& mumu_path,
                     int mumu_inst_index);
    int GetDisplayId();
    bool InitScreencap();

    std::vector<uint8_t> display_buffer_;

    bool inited_ = false;
    int mumu_handle_ = 0;

    QString package_name_;

    int display_width_ = -1;
    int display_height_ = -1;
};

} // namespace psh

#endif // MUMU_CLIENT_H_