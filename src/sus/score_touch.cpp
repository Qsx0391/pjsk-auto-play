#include "score_touch.h"

#include <functional>
#include <unordered_set>

namespace MMW = MikuMikuWorld;

namespace {
using namespace psh;

float lerp(float start, float end, float percentage) {
    return start + percentage * (end - start);
}

float easeIn(float start, float end, float ratio) {
    return lerp(start, end, ratio * ratio);
}

float easeOut(float start, float end, float ratio) {
    return lerp(start, end, 1 - (1 - ratio) * (1 - ratio));
}

std::function<float(float, float, float)> getEaseFunction(MMW::EaseType ease) {
    switch (ease) {
        case MMW::EaseType::EaseIn: return easeIn;
        case MMW::EaseType::EaseOut: return easeOut;
        default: break;
    }
    return lerp;
}

float TicksToMs(int ticks, int beat_ticks, float bpm) {
    return static_cast<int64_t>(ticks) * 60 * 1000 / bpm / beat_ticks;
}

int AccumulateTicks(int tick, int beat_ticks, std::vector<MMW::Tempo> bpms) {
    float total = 0;
    int acc_ticks = 0;
    int last_bpm = 0;
    for (int i = 0; i < bpms.size() - 1; ++i) {
        last_bpm = i;
        int ticks = bpms[i + 1].tick - bpms[i].tick;
        if (acc_ticks + ticks >= tick) break;

        acc_ticks += ticks;
        total +=
            TicksToMs(bpms[i + 1].tick - bpms[i].tick, beat_ticks, bpms[i].bpm);
        last_bpm = i + 1;
    }
    total +=
        TicksToMs(tick - bpms[last_bpm].tick, beat_ticks, bpms[last_bpm].bpm);
    return static_cast<int>(total);
}

NoteTouch NoteToTouch(const MMW::Note &note, const MMW::Score &score) {
    NoteTouch ret;
    ret.delay_ms = AccumulateTicks(note.tick, 480, score.tempoChanges);
    ret.lane = note.lane + note.width / 2.0;
    ret.friction = note.friction;
    ret.flick = note.flick;
    return ret;
}

void InterpolateHoldStep(std::vector<HoldStepTouch> &hold_steps, int begin_ms,
                         int begin_lane, int end_ms, int end_lane,
                         MMW::EaseType ease_type) {
    auto ease_func = getEaseFunction(ease_type);
    int duration = end_ms - begin_ms;
    for (int i = 0; i <= duration; i += 10) {
        if (hold_steps.empty() ||
            hold_steps.back().delay_ms < begin_ms + i - 10) {
            hold_steps.push_back(HoldStepTouch{
                begin_ms + i,
                ease_func(begin_lane, end_lane, i / (float)duration)});
        }
    }
}

} // namespace

namespace psh {

ScoreTouch ScoreToTouch(const MMW::Score &score) {
    ScoreTouch ret;
    std::unordered_set<int> hold_note_ids;
    for (const auto &[id, hold] : score.holdNotes) {
        HoldTouch ht;
        ht.start = NoteToTouch(score.notes.at(hold.start.ID), score);
        ht.end = NoteToTouch(score.notes.at(hold.end), score);
        hold_note_ids.insert(hold.start.ID);
        hold_note_ids.insert(hold.end);
        int prev_ms = ht.start.delay_ms;
        float prev_lane = ht.start.lane;
        MMW::EaseType prev_ease = hold.start.ease;
        for (const auto &step : hold.steps) {
            hold_note_ids.insert(step.ID);
            const auto &note = score.notes.at(step.ID);
            int curr_ms = AccumulateTicks(note.tick, 480, score.tempoChanges);
            float curr_lane = note.lane + note.width / 2.0;
            if (step.type != MMW::HoldStepType::Skip) {
                InterpolateHoldStep(ht.steps, prev_ms, prev_lane, curr_ms,
                                    curr_lane, prev_ease);
                prev_ms = curr_ms;
                prev_lane = curr_lane;
                prev_ease = step.ease;
            }
        }
        InterpolateHoldStep(ht.steps, prev_ms, prev_lane, ht.end.delay_ms,
                            ht.end.lane, prev_ease);
        ret.holds.push_back(ht);
    }

    for (const auto &[id, note] : score.notes) {
        if (hold_note_ids.find(id) == hold_note_ids.end()) {
            ret.notes.push_back(NoteToTouch(note, score));
        }
    }
    return ret;
}

} // namespace psh