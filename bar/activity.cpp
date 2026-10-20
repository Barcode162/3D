#include "bar/activity.h"

namespace app {

bool ActivityManager::seated() const {
  return current_ == Activity::kSit || current_ == Activity::kDrive || current_ == Activity::kPiano;
}

std::string ActivityManager::blocked(Activity next, int param) const {
  const auto who = [&](Activity kind) { return usedBy ? usedBy(kind, param) : std::string(); };
  switch (next) {
    case Activity::kSit:
    case Activity::kDrive:
    case Activity::kPiano: {
      const std::string name = who(Activity::kSit);
      if (!name.empty()) return name + " is sitting there.";
      break;
    }
    case Activity::kRhythm: {
      const std::string name = who(Activity::kRhythm);
      if (!name.empty()) return name + " is playing. One player at a time.";
      break;
    }
    case Activity::kDarts: {
      const std::string name = who(Activity::kDarts);
      if (!name.empty()) return name + " is throwing. One player at a time.";
      break;
    }
    case Activity::kEmote:
      if (seated() || current_ == Activity::kRhythm || current_ == Activity::kDarts) {
        return "Stand up first to do an emote.";
      }
      break;
    default:
      break;
  }
  return "";
}

bool ActivityManager::request(Activity next, int param, double nowMs, std::string* why) {
  const std::string reason = blocked(next, param);
  if (!reason.empty()) {
    if (why) *why = reason;
    return false;
  }
  state_ = State::kRequesting;
  set(next, param, nowMs);  // granted on the spot (the room will answer here, 5.2)
  return true;
}

void ActivityManager::set(Activity next, int param, double nowMs) {
  if (next == current_ && param == param_) {
    state_ = State::kActive;
    return;
  }
  const Activity ended = current_;
  current_ = next;
  param_ = param;
  sinceMs_ = nowMs;
  state_ = State::kActive;
  if (ended != Activity::kNone && ended != next && onLeave) onLeave(ended);
}

bool commandAllowed(const std::string& name, double a, bool seated, bool onShift) {
  if (name == "emote") return a >= 1 && a < static_cast<double>(static_cast<int>(Emote::kCount));
  if (name == "make") return onShift && a >= 0;
  if (name == "endShift") return onShift;
  if (name == "standUp") return seated;
  return true;
}

}  // namespace app
