#include "mumu/mumu_lib_loader.h"

#include <spdlog/spdlog.h>

namespace psh {

template <typename Func>
bool LoadFunction(HMODULE hlib, const std::string& func_name,
                  std::function<Func>& target) {
    auto func =
        reinterpret_cast<Func*>(GetProcAddress(hlib, func_name.c_str()));
    if (func == nullptr) {
        spdlog::error("Failed to load function {}", func_name);
        return false;
    }
    target = func;
    return true;
}

bool MumuLibLoader::Init(const std::filesystem::path& mumu_path) {
    MumuLibLoader& inst = Instance();
    std::lock_guard<std::mutex> lock(inst.init_mutex_);
    if (!inst.inited_) {
        auto lib_path = mumu_path / "shell" / "sdk" / "external_renderer_ipc";
        auto new_lib_path = mumu_path / "nx_device" / "12.0" / "shell" / "sdk" /
                            "external_renderer_ipc.dll";
        HMODULE hlib;
        if (!(hlib = LoadLibraryW(lib_path.c_str())) &&
            !(hlib = LoadLibraryW(new_lib_path.c_str()))) {
            spdlog::error("Failed to load library from {}", lib_path.string());
            return false;
        }
        inst.hlib_ = hlib;

        // clang-format off
        inst.inited_ =
            LoadFunction(inst.hlib_, kConnectFuncName,           inst.connect_func_                      ) &&
            LoadFunction(inst.hlib_, kDisconnectFuncName,        inst.disconnect_func_                   ) &&
            LoadFunction(inst.hlib_, kGetDisplayIdFuncName,      inst.get_display_id_func_               ) &&
            LoadFunction(inst.hlib_, kCaptureDisplayFuncName,    inst.capture_display_func_              ) &&
            LoadFunction(inst.hlib_, kInputTextFuncName,         inst.input_text_func_                   ) &&
            LoadFunction(inst.hlib_, kInputEventTouchDown,       inst.input_event_touch_down_func_       ) &&
            LoadFunction(inst.hlib_, kInputEventTouchUp,         inst.input_event_touch_up_func_         ) &&
            LoadFunction(inst.hlib_, kInputEventKeyDown,         inst.input_event_key_down_func_         ) &&
            LoadFunction(inst.hlib_, kInputEventKeyUp,           inst.input_event_key_up_func_           ) &&
            LoadFunction(inst.hlib_, kInputEventFingerTouchDown, inst.input_event_finger_touch_down_func_) &&
            LoadFunction(inst.hlib_, kInputEventFingerTouchUp,   inst.input_event_finger_touch_up_func_  );
        return inst.inited_;
        // clang-format on
    }
    return true;
}

void MumuLibLoader::Uninit() {
    MumuLibLoader& inst = Instance();
    std::lock_guard<std::mutex> lock(inst.init_mutex_);
    if (inst.inited_) {
        FreeLibrary(inst.hlib_);
        inst.inited_ = false;
    }
}

int MumuLibLoader::Connect(const wchar_t* path, int index) {
    return Instance().connect_func_(path, index);
}

void MumuLibLoader::Disconnect(int handle) {
    Instance().disconnect_func_(handle);
}

int MumuLibLoader::GetDisplayId(int handle, const char* pkg, int app_index) {
    return Instance().get_display_id_func_(handle, pkg, app_index);
}

int MumuLibLoader::CaptureDisplay(int handle, unsigned int display_id,
                                  int buffer_size, int* width, int* height,
                                  unsigned char* pixels) {
    return Instance().capture_display_func_(handle, display_id, buffer_size,
                                            width, height, pixels);
}

int MumuLibLoader::InputText(int handle, int size, const char* buf) {
    return Instance().input_text_func_(handle, size, buf);
}

int MumuLibLoader::InputEventTouchDown(int handle, int displayid, int x,
                                       int y) {
    return Instance().input_event_touch_down_func_(handle, displayid, x, y);
}

int MumuLibLoader::InputEventTouchUp(int handle, int displayid) {
    return Instance().input_event_touch_up_func_(handle, displayid);
}

int MumuLibLoader::InputEventKeyDown(int handle, int displayid, int key_code) {
    return Instance().input_event_key_down_func_(handle, displayid, key_code);
}

int MumuLibLoader::InputEventKeyUp(int handle, int displayid, int key_code) {
    return Instance().input_event_key_up_func_(handle, displayid, key_code);
}

int MumuLibLoader::InputEventFingerTouchDown(int handle, int displayid,
                                             int finger_id, int x_point,
                                             int y_point) {
    return Instance().input_event_finger_touch_down_func_(
        handle, displayid, finger_id, x_point, y_point);
}

int MumuLibLoader::InputEventFingerTouchUp(int handle, int displayid,
                                           int slot_id) {
    return Instance().input_event_finger_touch_up_func_(handle, displayid,
                                                        slot_id);
}

MumuLibLoader& MumuLibLoader::Instance() {
    static MumuLibLoader inst;
    return inst;
}

} // namespace psh