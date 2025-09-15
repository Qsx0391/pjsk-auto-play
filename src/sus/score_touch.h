#include <vector>

#include <MikuMikuWorld/NoteTypes.h>
#include <MikuMikuWorld/Score.h>

namespace psh {

struct NoteTouch {
    int delay_ms;
    float lane;
    bool friction;
    MikuMikuWorld::FlickType flick;
};

struct HoldStepTouch {
    int delay_ms;
    float lane;
};

struct HoldTouch {
    NoteTouch start;
    NoteTouch end;
    std::vector<HoldStepTouch> steps;
};

struct ScoreTouch {
    std::vector<NoteTouch> notes;
    std::vector<HoldTouch> holds;
};

ScoreTouch ScoreToTouch(const MikuMikuWorld::Score& score);

} // namespace psh