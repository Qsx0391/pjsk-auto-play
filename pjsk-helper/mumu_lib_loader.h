#pragma once

#include <string>
#include <functional>
#include <mutex>
#include <filesystem>
#include <Windows.h>

#include "external_renderer_ipc.h"

#ifndef MUMU_LIBL_LOADER_H_
#define MUMU_LIBL_LOADER_H_

namespace psh {

class MumuLibLoader {
public:
    static bool Init(const std::filesystem::path& mumu_path);
    static void Uninit();

    static int Connect(const wchar_t* path, int index);
    static void Disconnect(int handle);

    static int GetDisplayId(int handle, const char* pkg, int appIndex);
    static int CaptureDisplay(int handle, unsigned int displayid,
                              int buffer_size, int* width, int* height,
                              unsigned char* pixels);
    static int InputText(int handle, int size, const char* buf);
    static int InputEventTouchDown(int handle, int displayid, int x, int y);
    static int InputEventTouchUp(int handle, int displayid);
    static int InputEventKeyDown(int handle, int displayid, int key_code);
    static int InputEventKeyUp(int handle, int displayid, int key_code);
    static int InputEventFingerTouchDown(int handle, int displayid,
										 int finger_id, int x_point,
										 int y_point);
    static int InputEventFingerTouchUp(int handle, int displayid, int slot_id);

private:
    MumuLibLoader() = default;
    ~MumuLibLoader() = default;
    MumuLibLoader(const MumuLibLoader&) = delete;
    MumuLibLoader& operator=(const MumuLibLoader&) = delete;

    static MumuLibLoader& Instance();

    inline static const std::string kConnectFuncName = "nemu_connect";
    inline static const std::string kDisconnectFuncName = "nemu_disconnect";
    inline static const std::string kGetDisplayIdFuncName =
        "nemu_get_display_id";
    inline static const std::string kCaptureDisplayFuncName =
        "nemu_capture_display";
    inline static const std::string kInputTextFuncName = "nemu_input_text";
    inline static const std::string kInputEventTouchDown = "nemu_input_event_touch_down";
    inline static const std::string kInputEventTouchUp = "nemu_input_event_touch_up";
    inline static const std::string kInputEventKeyDown = "nemu_input_event_key_down";
    inline static const std::string kInputEventKeyUp = "nemu_input_event_key_up";
    inline static const std::string kInputEventFingerTouchDown =
		"nemu_input_event_finger_touch_down";
    inline static const std::string kInputEventFingerTouchUp =
        "nemu_input_event_finger_touch_up";

    std::function<decltype(nemu_connect)> connect_func_;
    std::function<decltype(nemu_disconnect)> disconnect_func_;
    std::function<decltype(nemu_get_display_id)> get_display_id_func_;
    std::function<decltype(nemu_capture_display)> capture_display_func_;
    std::function<decltype(nemu_input_text)> input_text_func_;
    std::function<decltype(nemu_input_event_touch_down)> input_event_touch_down_func_;
    std::function<decltype(nemu_input_event_touch_up)> input_event_touch_up_func_;
    std::function<decltype(nemu_input_event_key_down)> input_event_key_down_func_;
    std::function<decltype(nemu_input_event_key_up)> input_event_key_up_func_;
    std::function<decltype(nemu_input_event_finger_touch_down)>
		input_event_finger_touch_down_func_;
    std::function<decltype(nemu_input_event_finger_touch_up)>
        input_event_finger_touch_up_func_;

    HMODULE hlib_;

    bool inited_ = false;
    std::mutex init_mutex_;
};

} // namespace psh

#endif // MUMU_LIBL_LOADER_H_