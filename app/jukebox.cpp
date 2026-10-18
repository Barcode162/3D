#include "app/jukebox.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "engine/font.h"
#include "engine/log.h"

namespace app {
namespace {

constexpr const char* kBuiltIn[][2] = {
    {"smooth-lovin.m4a", "Smooth Lovin"},
    {"bossa-antigua.m4a", "Bossa Antigua"},
    {"night-on-the-docks-sax.m4a", "Night on the Docks - Sax"},
    {"lobby-time.m4a", "Lobby Time"},
};

// Menu colors (display space, 0..1).
constexpr engine::Vec4 kPanel{0.06f, 0.025f, 0.05f, 0.93f};
constexpr engine::Vec4 kNeonPink{1.0f, 0.3f, 0.65f, 1.0f};
constexpr engine::Vec4 kNeonCyan{0.35f, 0.9f, 1.0f, 1.0f};
constexpr engine::Vec4 kCream{1.0f, 0.88f, 0.68f, 1.0f};
constexpr engine::Vec4 kRow{1.0f, 1.0f, 1.0f, 0.06f};
constexpr engine::Vec4 kRowPlaying{1.0f, 0.3f, 0.65f, 0.22f};
constexpr engine::Vec4 kButton{0.35f, 0.9f, 1.0f, 0.14f};

// Jukebox-style selection codes: A1..A9, B1..B9, ...
std::string songCode(int index) {
  return std::string(1, static_cast<char>('A' + index / 9)) + static_cast<char>('1' + index % 9);
}

// Cuts text to fit `maxChars`, ending in "..." when shortened.
std::string fit(const std::string& s, int maxChars) {
  if (static_cast<int>(s.size()) <= maxChars) return s;
  return s.substr(0, static_cast<size_t>(std::max(maxChars - 3, 1))) + "...";
}

}  // namespace

// --- Jukebox ---------------------------------------------------------------

void Jukebox::init(engine::Vec3 speakerPosition) {
  for (const auto& t : kBuiltIn) songs_.push_back({t[1], std::string("assets/music/") + t[0], true});
  source_.init(2.0f);  // full volume within 2 m, then fades with distance
  source_.setPosition(speakerPosition);
}

void Jukebox::play(int index, double offsetSeconds, bool fromNetwork) {
  if (songs_.empty()) return;
  current_ = index % static_cast<int>(songs_.size());
  ++songsStarted_;
  if (!fromNetwork) ++localChanges_;
  const Song& song = songs_[static_cast<size_t>(current_)];
  source_.play(song.url, offsetSeconds);
  LOG_INFO("Jukebox: playing \"%s\"%s.", song.title.c_str(), fromNetwork ? " (chosen by another player)" : "");
}

void Jukebox::stop(bool fromNetwork) {
  if (current_ < 0) return;
  source_.pause();
  current_ = -1;
  if (!fromNetwork) ++localChanges_;
  LOG_INFO("Jukebox: stopped.");
}

void Jukebox::update() {
  // New songs from the phone: add them and play the first one right away.
  const int userCount = engine::audio::userSongCount();
  if (userCount > userSongsSeen_) {
    const int firstNew = static_cast<int>(songs_.size());
    for (int i = userSongsSeen_; i < userCount; ++i) {
      songs_.push_back({engine::audio::userSongName(i), engine::audio::userSongUrl(i), false});
    }
    LOG_INFO("Jukebox: added %d song(s) from your phone.", userCount - userSongsSeen_);
    userSongsSeen_ = userCount;
    play(firstNew);
  }
  if (current_ >= 0 && source_.finished()) {
    // A built-in song while online: the room picks the next one for
    // everyone. Your own songs (only on your phone) still go on here.
    if (followRoom_ && songs_[static_cast<size_t>(current_)].builtIn) return;
    play(current_ + 1);
  }
}

std::string Jukebox::prompt() const {
  if (current_ < 0) return "Jukebox\nTap to choose a song";
  return "Now playing: " + songs_[static_cast<size_t>(current_)].title + "\nTap to choose a song";
}

// --- JukeboxMenu -----------------------------------------------------------

void JukeboxMenu::open() {
  open_ = true;
  page_ = -1;  // jump to the page of the current song on the next draw
}

void JukeboxMenu::close() {
  open_ = false;
  placePicker({}, 1);
}

void JukeboxMenu::placePicker(const Rect& r, float pixelRatio) {
  const Rect css{r.x / pixelRatio, r.y / pixelRatio, r.w / pixelRatio, r.h / pixelRatio};
  if (css.x == pickerRect_.x && css.y == pickerRect_.y && css.w == pickerRect_.w && css.h == pickerRect_.h) return;
  pickerRect_ = css;
  engine::audio::setSongPickerRect(css.x, css.y, css.w, css.h);
}

void JukeboxMenu::tap(engine::Vec2 tap, Jukebox& jukebox) {
  const float x = tap.x * screenW_, y = tap.y * screenH_;
  if (!panel_.contains(x, y)) return close();  // tapping outside closes it
  for (size_t i = 0; i < rows_.size(); ++i) {
    if (rows_[i].contains(x, y)) return jukebox.play(firstRow_ + static_cast<int>(i));
  }
  if (pageCount_ > 1 && prev_.contains(x, y)) page_ = (page_ + pageCount_ - 1) % pageCount_;
  if (pageCount_ > 1 && next_.contains(x, y)) page_ = (page_ + 1) % pageCount_;
  if (stop_.contains(x, y)) jukebox.stop();
  if (close_.contains(x, y)) close();
  // Load song is handled by the native picker sitting on top of it.
}

void JukeboxMenu::draw(engine::TextRenderer& text, const Jukebox& jukebox, float W, float H, float p) {
  if (!open_) return;
  using engine::font::kAdvance;
  using engine::font::kCapHeight;
  screenW_ = W;
  screenH_ = H;
  const auto& songs = jukebox.songs();
  const int count = static_cast<int>(songs.size());

  // Layout, in device pixels (p = device pixels per CSS pixel).
  const float pad = 14 * p, rowH = 38 * p, rowGap = 4 * p, buttonH = 40 * p, pagerH = 36 * p;
  const float titleCap = 18 * p, rowCap = 11 * p;
  const float panelW = std::min(W - 24 * p, 380 * p);
  const float header = pad + titleCap + 18 * p;
  const float footer = pad + buttonH + pad;
  const float maxRowsH = H - 150 * p - header - footer - pagerH;
  const int perPage = std::clamp(static_cast<int>(maxRowsH / (rowH + rowGap)), 3, 8);
  pageCount_ = (count + perPage - 1) / perPage;
  if (page_ < 0) page_ = std::max(jukebox.current(), 0) / perPage;
  page_ = std::min(page_, pageCount_ - 1);
  firstRow_ = page_ * perPage;
  const int shown = std::min(perPage, count - firstRow_);
  // One page: fit the panel to the songs. Several: keep its size steady.
  const int rowSlots = pageCount_ > 1 ? perPage : shown;
  const float listH = static_cast<float>(rowSlots) * (rowH + rowGap) + (pageCount_ > 1 ? pagerH : 0);
  const float panelH = header + listH + footer;
  panel_ = {(W - panelW) / 2, std::max(90 * p, (H - panelH) / 2), panelW, panelH};

  // Panel with a neon frame.
  const float border = 2 * p;
  text.addRect(panel_.x, panel_.y, panel_.w, panel_.h, kPanel);
  text.addRect(panel_.x, panel_.y, panel_.w, border, kNeonPink);
  text.addRect(panel_.x, panel_.y + panel_.h - border, panel_.w, border, kNeonPink);
  text.addRect(panel_.x, panel_.y, border, panel_.h, kNeonPink);
  text.addRect(panel_.x + panel_.w - border, panel_.y, border, panel_.h, kNeonPink);

  engine::TextStyle title;
  title.align = 0.5f;
  title.color = kNeonPink;
  title.outlineWidth = 0.9f;
  text.addScreen("JUKEBOX", W / 2, panel_.y + pad, titleCap, title);

  // Song rows.
  const float rowX = panel_.x + pad, rowW = panel_.w - 2 * pad;
  const float charW = kAdvance * rowCap / kCapHeight;
  const int maxChars = static_cast<int>((rowW - 2 * pad - 3 * charW) / charW);
  rows_.clear();
  float y = panel_.y + header;
  for (int i = 0; i < shown; ++i, y += rowH + rowGap) {
    const int index = firstRow_ + i;
    const bool playing = index == jukebox.current();
    const Rect r{rowX, y, rowW, rowH};
    rows_.push_back(r);
    text.addRect(r.x, r.y, r.w, r.h, playing ? kRowPlaying : kRow);
    engine::TextStyle style;
    style.outline.w = 0.6f;
    const float textY = r.y + (r.h - rowCap) / 2;
    style.color = kNeonCyan;
    text.addScreen(songCode(index), r.x + pad * 0.7f, textY, rowCap, style);
    style.color = playing ? kNeonPink : kCream;
    const Song& song = songs[static_cast<size_t>(index)];
    text.addScreen(fit(song.title, maxChars), r.x + pad * 0.7f + 3 * charW, textY, rowCap, style);
  }
  y = panel_.y + header + static_cast<float>(rowSlots) * (rowH + rowGap);

  engine::TextStyle buttonText;
  buttonText.align = 0.5f;
  buttonText.color = kCream;
  buttonText.outline.w = 0.6f;
  auto button = [&](Rect& r, float bx, float by, float bw, float bh, const char* label) {
    r = {bx, by, bw, bh};
    text.addRect(r.x, r.y, r.w, r.h, kButton);
    text.addScreen(label, r.x + r.w / 2, r.y + (r.h - rowCap) / 2, rowCap, buttonText);
  };
  if (pageCount_ > 1) {
    const float gap = 56 * p;  // the page number sits in between
    const float half = (rowW - gap) / 2;
    button(prev_, rowX, y, half, pagerH - 6 * p, "< Prev");
    button(next_, rowX + half + gap, y, half, pagerH - 6 * p, "Next >");
    char pageText[32];
    std::snprintf(pageText, sizeof(pageText), "%d/%d", page_ + 1, pageCount_);
    engine::TextStyle small = buttonText;
    small.color = kNeonCyan;
    text.addScreen(pageText, W / 2, y + (pagerH - 6 * p - 9 * p) / 2, 9 * p, small);
    y += pagerH;
  }

  // Footer buttons.
  const float by = panel_.y + panel_.h - pad - buttonH;
  const float bw = (rowW - 2 * pad * 0.6f) / 3;
  button(load_, rowX, by, bw, buttonH, "Load song");
  button(stop_, rowX + bw + pad * 0.6f, by, bw, buttonH, "Stop");
  button(close_, rowX + 2 * (bw + pad * 0.6f), by, bw, buttonH, "Close");
  placePicker(load_, p);
}

}  // namespace app
