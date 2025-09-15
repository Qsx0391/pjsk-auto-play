#pragma once

#ifndef PSH_COMMON_TIME_UTILS_H_
#define PSH_COMMON_TIME_UTILS_H_

#include <cstdint>

namespace psh {

int64_t GetCurrentTimeMs();
int64_t GetCurrentTimeNs();
inline int64_t MsToNs(int64_t ms) { return ms * 1'000'000; }

} // namespace psh

#endif // !PSH_COMMON_TIME_UTILS_H_