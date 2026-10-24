# The phone

What the user's phone (iPhone, Safari, iOS 26) allows; every design choice
respects it. 60 fps on an iPhone 15 Pro Max is the budget.

- **WebGPU** is there (iOS 26). **No WebXR**: AR would mean exporting USDZ for AR Quick Look.
- **Sound** only starts after a tap; **motion sensors** need a permission tap.
- **No threads**: the hosting can't send the headers SharedArrayBuffer needs (COOP/COEP), so no pthreads in the browser.
- **No Web MIDI** on iPhone Safari (a real MIDI keyboard works on computers and Android).
- **Memory per tab is limited**; long sessions heat the phone up and slow it down.
- **Timers are coarse**: `performance.now()` is rounded to about 1 ms.
- **Forms**: the keyboard and the photo picker only work with real HTML fields (so menus and accounts are HTML over the game).
- **Game controllers** (Xbox, PlayStation) work through the Gamepad API.
- The phone is the real visual and speed test: `?perf=1` and Settings → Send report.
