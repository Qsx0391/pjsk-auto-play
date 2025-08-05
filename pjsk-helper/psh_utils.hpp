#pragma once

#ifndef PSH_UTILS_HPP_
#define PSH_UTILS_HPP_

#include <chrono>

namespace psh {

inline int64_t GetCurrentTimeMs() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
}

} // namespace psh

#endif // !PSH_UTILS_HPP_