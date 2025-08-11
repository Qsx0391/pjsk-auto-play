#pragma once

#ifndef AUTO_PLAY_CONFIG_H_
#define AUTO_PLAY_CONFIG_H_

namespace psh {

struct TrackConfig {
    int upper_len;
    int lower_len;
    int height;
    int hit_line_y;
    int check_upper_y;
    int check_lower_y;
    int dx;
    int dy;
};

struct PlayConfig {
    int hold_cnt;
    float color_delta;
    int tap_duration_ms;
    int check_loop_delay_ms;
    int hit_delay_ms;
};

inline static const PlayConfig kDefaultPlayConfig = {6, 10, 20, 50, -30};

inline static const TrackConfig kDefaultTrackConfig = {
    59, 1260, 720, 567, 0, 70, 10, 0,
};

} // namespace psh

#endif // !AUTO_PLAY_CONFIG_H_
