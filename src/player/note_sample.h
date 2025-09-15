#pragma once

#ifndef PSH_PLAYER_NOTE_SAMPLE_H_
#define PSH_PLAYER_NOTE_SAMPLE_H_

#include "player/note_finder.h"

#include <vector>

namespace psh {

struct NoteSample {
    Note note;
    int count = 1;
    bool touched = false;
    NoteSample(const Note &note) : note(note) {}
    void AddSample(const Note &sample);
};

} // namespace psh

#endif // !PSH_PLAYER_NOTE_SAMPLE_H_