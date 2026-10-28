#include "bar/projects/mania/stars.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace app {
namespace {

constexpr double kIndividualDecay = 0.125, kOverallDecay = 0.30, kReleaseThreshold = 30;
constexpr double kSection = 400, kDecayWeight = 0.9, kStarScale = 0.018;

struct Obj {
  double start, end;  // ms
  int col;
};

double decay(double value, double ms, double base) { return value * std::pow(base, ms / 1000.0); }
bool bigger(double a, double b) { return a - b > 1; }  // osu!'s Precision.DefinitelyBigger(a, b, 1)

// Every 400 ms section's peak strain, as osu!mania's Strain skill keeps them.
std::vector<double> peaks(const std::vector<ChartNote>& notes) {
  std::vector<Obj> objs;
  objs.reserve(notes.size());
  for (const ChartNote& n : notes) {
    const double s = n.time * 1000, e = n.end > n.time ? n.end * 1000 : s;
    objs.push_back({s, e, std::clamp(n.lane, 0, 3)});
  }
  std::stable_sort(objs.begin(), objs.end(), [](const Obj& a, const Obj& b) {
    const long ra = std::lround(a.start), rb = std::lround(b.start);
    return ra != rb ? ra < rb : a.col < b.col;
  });
  std::vector<double> out;
  if (objs.size() < 2) return out;
  std::array<double, 4> individual{};
  double highest = 0, overall = 1, peak = 0, sectionEnd = 0;
  std::array<const Obj*, 4> prev{};      // the last note processed in each column (its snapshot)
  std::array<double, 4> lastStart{};      // for the column's gap
  std::array<bool, 4> seen{};
  const Obj* last = nullptr;              // the previous difficulty object
  // osu! skips the first note (it has no previous one); its column history
  // starts from the second.
  for (size_t i = 1; i < objs.size(); ++i) {
    const Obj& o = objs[i];
    const double delta = o.start - objs[i - 1].start;
    if (i == 1) sectionEnd = std::ceil(o.start / kSection) * kSection;
    while (o.start > sectionEnd) {
      out.push_back(peak);
      const double dt = sectionEnd - (last ? last->start : o.start);
      peak = decay(highest, dt, kIndividualDecay) + decay(overall, dt, kOverallDecay);
      sectionEnd += kSection;
    }
    if (last) prev[static_cast<size_t>(last->col)] = last;
    const double columnGap = seen[static_cast<size_t>(o.col)] ? o.start - lastStart[static_cast<size_t>(o.col)] : o.start;
    // Individual: +2 a note, ×1.25 if something held starts before and ends after it.
    double holdI = 1;
    for (const Obj* p : prev) {
      if (p && bigger(p->end, o.end) && bigger(o.start, p->start)) {
        holdI = 1.25;
        break;
      }
    }
    double& ind = individual[static_cast<size_t>(o.col)];
    ind = decay(ind, columnGap, kIndividualDecay) + 2.0 * holdI;
    highest = delta <= 1 ? std::max(highest, ind) : ind;
    // Overall: +1 a note, more for a hold released apart from the others.
    bool overlapping = false;
    double holdO = 1, closest = std::fabs(o.end - o.start);
    for (const Obj* p : prev) {
      if (!p) continue;
      overlapping |= bigger(p->end, o.start) && bigger(o.end, p->end) && bigger(o.start, p->start);
      if (bigger(p->end, o.end) && bigger(o.start, p->start)) holdO = 1.25;
      closest = std::min(closest, std::fabs(o.end - p->end));
    }
    const double holdAdd = overlapping ? 1.0 / (1.0 + std::exp(0.27 * (kReleaseThreshold - closest))) : 0.0;
    overall = decay(overall, delta, kOverallDecay) + (1 + holdAdd) * holdO;
    peak = std::max(peak, highest + overall);
    seen[static_cast<size_t>(o.col)] = true;
    lastStart[static_cast<size_t>(o.col)] = o.start;
    last = &o;
  }
  out.push_back(peak);
  return out;
}

}  // namespace

double starRating(const std::vector<ChartNote>& notes) {
  std::vector<double> p = peaks(notes);
  std::sort(p.begin(), p.end(), [](double a, double b) { return a > b; });
  double total = 0, weight = 1;
  for (double v : p) {
    if (v <= 0) continue;
    total += v * weight;
    weight *= kDecayWeight;
  }
  return total * kStarScale;
}

std::vector<double> strainSections(const std::vector<ChartNote>& notes) { return peaks(notes); }

}  // namespace app
