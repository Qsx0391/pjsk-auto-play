#pragma once

#ifndef RING_BUFFER_HPP_
#define RING_BUFFER_HPP_

#include <memory>
#include <atomic>
#include <optional>

namespace psh {

template <typename T>
class RingBuffer {
public:
    RingBuffer(size_t n)
        : size_(SizeFor(n)), buffer_(std::make_unique<T[]>(size_)) {}
    RingBuffer(const RingBuffer<T>&) = delete;
    RingBuffer(RingBuffer<T>&&) = delete;
    ~RingBuffer() = default;

    bool Push(const T& item) {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (tail + 1) & (size_ - 1);
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        buffer_[tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    bool Push(T&& item) {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (tail + 1) & (size_ - 1);
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        buffer_[tail] = std::move(item);
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    void Pop() {
        size_t head = head_.load(std::memory_order_relaxed);
        if (head != tail_.load(std::memory_order_acquire)) {
            head_.store((head + 1) & (size_ - 1), std::memory_order_release);
        }
    }

    std::optional<T> Front() const {
        size_t head = head_.load(std::memory_order_relaxed);
        if (head == tail_.load(std::memory_order_acquire)) {
            return std::nullopt;
        }
        return buffer_[head];
    }

    void Clear() {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

private:
    size_t SizeFor(size_t n) {
        --n;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return n + 1;
    }

    size_t size_;
    std::atomic<size_t> head_ = 0;
    std::atomic<size_t> tail_ = 0;
    std::unique_ptr<T[]> buffer_;
};

} // namespace psh

#endif // !RING_BUFFER_HPP_