#pragma once
// What you're doing: one activity at a time (sitting, the piano, 4K Mania,
// darts, an emote...) and the rules for starting one. A request goes
// Idle → Requesting → Active; leaving goes back to Idle. For now requests are
// granted on the spot; when the room decides who uses what (RESTRUCTURE 5.2),
// Requesting waits for its answer. Pure logic (no engine): tested by
// tools/native/activity_test.cpp.
#include <functional>
#include <string>

#include "app/activity_kinds.h"

namespace app {

class ActivityManager {
 public:
  enum class State { kIdle, kRequesting, kActive };

  // Who else is using a seat or machine ("" = nobody): set by the game.
  // `kind` is kSit for every seat, kRhythm / kDarts for the machines.
  std::function<std::string(Activity kind, int param)> usedBy;
  // Runs when an activity ends (e.g. frees a machine in the room).
  std::function<void(Activity ended)> onLeave;

  // Why `next` (on seat or machine `param`) can't start now, as a message
  // for the player; empty when it can.
  std::string blocked(Activity next, int param) const;
  // Starts `next` if allowed (leaving the current one). Returns false and
  // sets `why` when it's blocked; nothing changes then.
  bool request(Activity next, int param, double nowMs, std::string* why = nullptr);
  // Switches without the rules (the game already checked, or the activity
  // ended by itself, e.g. an emote finishing).
  void set(Activity next, int param, double nowMs);
  void leave(double nowMs) { set(Activity::kNone, 0, nowMs); }
  // The same activity starts over (its animation, e.g. a new dart throw).
  void restart(double nowMs) { sinceMs_ = nowMs; }

  Activity current() const { return current_; }
  int param() const { return param_; }
  State state() const { return current_ == Activity::kNone ? State::kIdle : state_; }
  double sinceMs() const { return sinceMs_; }
  // Seated in any way (a stool, a sim rig, the piano bench).
  bool seated() const;

 private:
  Activity current_ = Activity::kNone;
  int param_ = 0;
  State state_ = State::kIdle;
  double sinceMs_ = 0;
};

// The menu's commands, checked before they're used: an emote in range,
// drinks only on a bartender shift, standing up only when seated.
bool commandAllowed(const std::string& name, double a, bool seated, bool onShift);

}  // namespace app
