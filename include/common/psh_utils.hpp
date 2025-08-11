#pragma once

#ifndef PSH_UTILS_HPP_
#define PSH_UTILS_HPP_

#include <chrono>

namespace psh {

template <typename F>
class Finalizer {
public:
    explicit Finalizer(F&& finalize) noexcept(
        std::is_nothrow_move_constructible_v<F>)
        : finalize_(std::move(finalize)) {}

    ~Finalizer() noexcept {
        try {
            finalize_();
        } catch (...) {
        }
    }

    Finalizer(const Finalizer&) = delete;
    Finalizer& operator=(const Finalizer&) = delete;

    Finalizer(Finalizer&&) noexcept(std::is_nothrow_move_constructible_v<F>) =
        default;
    Finalizer& operator=(Finalizer&&) noexcept(
        std::is_nothrow_move_assignable_v<F>) = default;

private:
    F finalize_;
};

inline int64_t GetCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

} // namespace psh

#endif // !PSH_UTILS_HPP_