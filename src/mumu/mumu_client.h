#pragma once

#ifndef PSH_MUMU_MUMU_CLIENT_H_
#define PSH_MUMU_MUMU_CLIENT_H_

#include <qstring.h>

#include <opencv2/opencv.hpp>

#include "mumu/mumu_lib_loader.h"
#include "screen/i_screen.h"
#include "touch/i_touch.h"

namespace psh {

class MumuClient : public IScreen, public ITouch {
public:
    inline static const std::vector<int> kSupportedSlots = {1, 2, 3, 4, 5,
                                                            6, 7, 8, 9, 10};

    inline static const QString kDefaultPackageName = "default";

    MumuClient(const QString& mumu_path_str, int mumu_inst_index,
               const QString& package_name);
    virtual ~MumuClient();
    bool Init();
    void Uninit();

    cv::Mat Capture() override;
    int GetDisplayWidth() override { return display_width_; }
    int GetDisplayHeight() override { return display_height_; }

    void TouchDown(int slot_index, cv::Point pos) override;
    void TouchUp(int slot_index) override;
    void TouchMove(int slot_index, cv::Point pos) override;
    const std::vector<int>& GetSupportedSlots() const override {
        return kSupportedSlots;
    }

    void KeyDown(int key);
    void KeyUp(int key);

private:
    bool ConnectMumu(const std::filesystem::path& mumu_path,
                     int mumu_inst_index);
    int GetDisplayId();
    bool InitScreencap();

    bool inited_ = false;
    int mumu_handle_ = 0;
    int display_width_ = -1;
    int display_height_ = -1;
    std::filesystem::path mumu_path_;
    int mumu_inst_index_ = -1;
    QString package_name_;
    std::vector<uint8_t> display_buffer_;
};

} // namespace psh

#endif // !PSH_MUMU_MUMU_CLIENT_H_