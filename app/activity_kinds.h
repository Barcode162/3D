#pragma once
// What you can be doing (sent to other players as numbers) and the emotes.
// No engine code here, so tests can use it on its own.

namespace app {

// What the body is doing besides walking. Sent to other players as a
// number, so the order must not change (add new ones at the end).
enum class Activity : int {
  kNone = 0,
  kSit,      // on any seat
  kDrive,    // in a racing sim seat, hands on the wheel
  kPiano,    // at the piano bench, hands on the keys
  kRhythm,   // standing at the 4K MANIA cabinet, tapping
  kDarts,    // at the oche, aiming and throwing
  kBartend,  // behind the bar: shaking, pouring
  kDrink,    // sipping a drink
  kEmote,    // one of the emotes below
};

// Emotes from the emote menu (also sent as numbers; append only).
enum class Emote : int {
  kNone = 0, kWave, kDance, kClap, kCheer, kBow, kPoint, kShrug, kLaugh, kSalute, kThumbsUp, kFacepalm, kCount
};

}  // namespace app
