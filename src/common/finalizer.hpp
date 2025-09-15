#pragma once

#ifndef PSH_COMMON_FINALIZER_HPP_
#define PSH_COMMON_FINALIZER_HPP_

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
        } catch (const std::exception& e) {
            spdlog::error("Finalizer error: {}", e.what());
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

} // namespace psh

#endif // !PSH_COMMON_FINALIZER_HPP_
