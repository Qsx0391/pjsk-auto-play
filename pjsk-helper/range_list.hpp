#pragma once

#ifndef RANGE_LIST_HPP_
#define RANGE_LIST_HPP_

#include <vector>
#include <memory>

namespace psh {

template <typename T> 
struct DefaultCmp {
    bool operator()(const T& t1, const T& t2) const { return t1 == t2; }
};

template <typename T, typename Cmp = DefaultCmp<T>>
class RangeList {
public:
    class Entry {
    public:
        friend class RangeList;

        Entry() = default;
        int GetBegin() const { return begin_; }
        int GetEnd() const { return end_; }

        std::shared_ptr<T> data;

    private:
        Entry(int begin, int end, std::shared_ptr<T> data)
            : begin_(begin), end_(end), data(data) {}

        int begin_, end_;
    };

    RangeList() = default;
    RangeList(int begin, int end, const T& data)
        : entries_({Entry{begin, end, std::make_shared<T>(data)}}) {}
    virtual ~RangeList() {}
    RangeList(const RangeList&) = default;
    RangeList(RangeList&&) = default;
    RangeList& operator=(const RangeList&) = default;
    RangeList& operator=(RangeList&&) = default;

    void Insert(int begin, int end, const T& data) {
        for (auto it = entries_.begin(); it != entries_.end(); ++it) {
            if (end <= it->end_) {
                if (begin == it->begin_ && end == it->end_) {
                    it->data = std::make_shared<T>(data);
                } else if (begin == it->begin_) {
                    it = entries_.insert(it + 1,
                                         Entry{end + 1, it->end_, it->data});
                    --it;
                    it->end_ = end;
                    it->data = std::make_shared<T>(data);
                } else if (end == it->end_) {
                    it = entries_.insert(
                        it, Entry{it->begin_, begin - 1, it->data});
                    ++it;
                    it->begin_ = begin;
                    it->data = std::make_shared<T>(data);
                } else {
                    int cur_end = it->end_;
                    it->end_ = begin - 1;
                    it = entries_.insert(
                        it + 1, Entry{begin, end, std::make_shared<T>(data)});
                    it = entries_.insert(
                        it + 1, Entry{end + 1, cur_end, (it - 1)->data});
                }
                return;
            }
        }
        entries_.push_back(Entry{begin, end, std::make_shared<T>(data)});
    }

    void Merge() {
        int i = 0;
        for (int j = 0; j < entries_.size();) {
            int k = j + 1;
            for (; k < entries_.size(); ++k) {
                if (!cmp_(*entries_[j].data, *entries_[k].data)) {
                    break;
                }
            }
            entries_[i++] =
                Entry{entries_[j].begin_, entries_[k - 1].end_, entries_[j].data};
            j = k;
        }
        entries_.resize(i);
    }

    Entry& operator[](int index) { return entries_[index]; }

    size_t Size() { return entries_.size(); }

private:
    std::vector<Entry> entries_;
    Cmp cmp_;
};

} // namespace psh

#endif // !RANGE_LIST_HPP_
