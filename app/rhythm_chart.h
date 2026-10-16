#pragma once
// Makes a 4-key rhythm game chart from any song, by listening to it: the
// audio is cut into short frames, an FFT finds where new sounds start
// (onsets) in four frequency bands, the tempo and the beat's phase are
// estimated, notes snap to the beat grid (beats, eighths or sixteenths by
// difficulty), and each note's lane follows the pitch of the sound (low
// sounds to the left, high to the right) while avoiding awkward repeats.
#include <cstddef>
#include <vector>

namespace app {

struct ChartNote {
  double time;  // seconds into the song
  int lane;     // 0..3
};

struct Chart {
  std::vector<ChartNote> notes;  // sorted by time
  double bpm = 0;
  double length = 0;  // seconds
};

enum class Difficulty { kEasy = 0, kNormal = 1, kHard = 2 };

// `samples` is the song in mono at `rate` samples per second.
Chart makeChart(const float* samples, size_t count, float rate, Difficulty difficulty);

}  // namespace app
