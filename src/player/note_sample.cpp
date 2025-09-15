#include "player/note_sample.h"

#include "player/auto_play_constant.h"

namespace psh {

void NoteSample::AddSample(const Note& sample) {
    // clang-format off
    note.is_slide    |= sample.is_slide;
    note.hold        = sample.hold;
    note.box         = sample.box;
    note.hit_pos.y   = (note.hit_pos.y * count + sample.hit_pos.y) / (count + 1);
    note.hit_time_ms = (note.hit_time_ms * count + sample.hit_time_ms) / (count + 1);
    ++count;
    // clang-format on
}

} // namespace psh