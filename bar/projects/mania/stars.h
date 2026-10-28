#pragma once
// Star rating for a 4K chart, the way osu!mania rates it (J117.7), so "4★"
// here means what it means there. A port of osu!mania's published formula
// (ppy/osu, osu.Game.Rulesets.Mania/Difficulty, version 20241007, MIT
// licence, Copyright (c) ppy Pty Ltd): every note adds to a strain for its
// column (fading to 12.5 % a second) and an overall strain (fading to 30 % a
// second), with more when something else is held or a release is awkward;
// the hardest moment of each 400 ms counts, the hardest sections most
// (weights 0.9^n), times 0.018.
#include <vector>

#include "bar/projects/mania/rhythm_chart.h"

namespace app {

// `notes` in any order; times in seconds. 0 for an empty chart.
double starRating(const std::vector<ChartNote>& notes);

// The same strain, section by section (for the map checker's "spikes"):
// one value per 400 ms section from the first note.
std::vector<double> strainSections(const std::vector<ChartNote>& notes);

}  // namespace app
