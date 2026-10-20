// The activity rules (bar/activity.*): what you may start from what, who
// else is using a seat or machine, the menu's command checks.
#include <cstdio>
#include <string>

#include "bar/activity.h"

using app::Activity;
using app::ActivityManager;

static int failed = 0;
static void check(bool ok, const char* what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) ++failed;
}

int main() {
  {
    ActivityManager m;
    check(m.state() == ActivityManager::State::kIdle && m.current() == Activity::kNone, "starts idle");
    check(m.request(Activity::kSit, 3, 100), "sitting on a free seat is allowed");
    check(m.current() == Activity::kSit && m.param() == 3 && m.seated() && m.sinceMs() == 100 &&
              m.state() == ActivityManager::State::kActive,
          "...and you're seated on seat 3 from t = 100");
    std::string why;
    check(!m.request(Activity::kEmote, 1, 200, &why) && why == "Stand up first to do an emote." &&
              m.current() == Activity::kSit,
          "an emote while seated is blocked, and you stay seated");
    m.leave(300);
    check(m.current() == Activity::kNone && m.state() == ActivityManager::State::kIdle, "standing up: idle again");
  }
  {
    ActivityManager m;
    m.usedBy = [](Activity kind, int param) {
      return kind == Activity::kSit && param == 5 ? std::string("Bob") : std::string();
    };
    std::string why;
    check(!m.request(Activity::kSit, 5, 0, &why) && why == "Bob is sitting there.", "Bob's seat is taken");
    check(m.request(Activity::kPiano, 6, 0), "another seat (the piano bench) is free");
  }
  {
    ActivityManager m;
    Activity ended = Activity::kNone;
    int leaves = 0;
    m.onLeave = [&](Activity a) { ended = a; ++leaves; };
    m.request(Activity::kRhythm, 0, 0);
    std::string why;
    check(!m.request(Activity::kEmote, 1, 10, &why), "an emote during 4K Mania is blocked");
    m.leave(20);
    check(ended == Activity::kRhythm && leaves == 1, "leaving 4K Mania frees the machine (onLeave once)");
    m.request(Activity::kEmote, 1, 30);
    m.request(Activity::kEmote, 2, 40);
    check(m.sinceMs() == 40 && leaves == 1, "a second emote starts over without a leave");
    m.usedBy = [](Activity kind, int) { return kind == Activity::kDarts ? std::string("Ann") : std::string(); };
    check(!m.request(Activity::kDarts, 0, 50, &why) && why == "Ann is throwing. One player at a time.",
          "darts in use by Ann");
  }
  check(!app::commandAllowed("standUp", 0, false, false), "standUp is ignored when not seated");
  check(app::commandAllowed("standUp", 0, true, false), "standUp works when seated");
  check(!app::commandAllowed("emote", 0, false, false) && !app::commandAllowed("emote", 99, false, false) &&
            app::commandAllowed("emote", 3, false, false),
        "emotes must be in range");
  check(!app::commandAllowed("make", 1, false, false) && app::commandAllowed("make", 1, false, true),
        "drinks only on a bartender shift");
  check(app::commandAllowed("look", 2, false, false), "other commands pass");
  std::printf(failed ? "ACTIVITY TEST FAILED (%d)\n" : "ACTIVITY TEST PASSED\n", failed);
  return failed ? 1 : 0;
}
