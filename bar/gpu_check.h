#pragma once
// The GPU check (J107.2, the user's request 2026-10-26 #13): Settings → Help
// Claude fix lag → Check the GPU. For about 20 seconds the bar draws as it
// is, then with one feature off at a time (soft shade, light beams, lamp
// shadows, bounced light, the mirrors, fewer lamps per pixel, fewer pixels),
// measuring each one's frames (the time between frames and the GPU's time),
// then puts everything back. The results go into the phone report
// (window.loopedPerf "extra") and show while it runs, so a report names
// what costs the most on that phone.
namespace app {
void startGpuCheck(double nowMs);
// Each frame: steps through the check (does nothing when none is running).
void updateGpuCheck(double nowMs);
bool gpuCheckRunning();
}  // namespace app
