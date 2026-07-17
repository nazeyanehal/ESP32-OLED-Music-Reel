# (=^･ω･^=) OLED Music Reel

### an aesthetic, always-alive "now playing" widget for ESP32 + SSD1306

```
        /\_/\        ♪彡 lyrics slide, sparkles twinkle,
       ( o.o )        the cat keeps breathing — nothing ever freezes
        > ^ <
```

A tiny 128×64 OLED screen turned into a Pinterest/Instagram-reel-style music
companion: a song title, lyrics that slide in on a timeline, a hand-doodled
cat that idles/blinks/dances/naps on its own, floating hearts, twinkling
corner sparkles, a bouncing music note, and a progress bar — all animated
smoothly at ~28 FPS with **zero `delay()` calls**.

(灬^ω^灬)ﾉ if you're skimming: jump to [Quick Start](#-quick-start) or the
[wiring diagram](#-wiring).

---

## (っ˘̩╭╮˘̩)っ Table of Contents

- [Overview](#-overview)
- [Features](#-features)
- [Hardware](#-hardware)
- [Wiring](#-wiring)
- [Quick Start](#-quick-start)
- [How It's Structured](#-how-its-structured)
- [Adding Your Own Lyrics](#-adding-your-own-lyrics)
- [Adding More Songs](#-adding-more-songs)
- [Adding New Cat Expressions](#-adding-new-cat-expressions)
- [Customizing Bold Text](#-customizing-bold-text)
- [Memory Footprint](#-memory-footprint)
- [Known Limitations](#-known-limitations)
- [Credits](#-credits)

---

## ฅ^•ﻌ•^ฅ Overview

This is a single self-contained Arduino sketch (`OLED_MusicReel.ino`) for an
**ESP32 + SSD1306 128×64 I2C OLED**. It simulates a "now playing" music
widget — title up top, lyrics in the middle that slide up on their own
timeline, an animated cat doodle near the bottom, and a filling progress
bar — the kind of aesthetic OLED clip you'd see on a Pinterest board or an
Instagram reel of someone's desk setup.

Everything is driven off `millis()`. There is no `delay()` anywhere in the
sketch, so the cat keeps breathing, sparkles keep twinkling, and the
progress bar keeps filling all at the same time, independently, without
ever blocking each other.

```
┌────────────────────────────────┐
│ Untitled Track              ♪   │  <- drawHeader()
│ ─────────────────────────────── │
│                                  │
│      you should know i          │  <- drawLyrics() (sliding)
│                                  │
│         /\_/\      *          . │  <- drawCharacter() + drawEffects()
│        ( ^ω^ )   .              │
│         > ^ <          *        │
│  ▓▓▓▓▓▓▓▓░░░░░░░░░░░░░░░░░░░░░░  │  <- drawProgressBar()
└────────────────────────────────┘
```

---

## ( ・ω・)ノ Features

- **Cat character, fully animated** — 16 hand-designed 32×32 frames sharing
  one consistent head/ear shape: idle ×2 (breathing), blink, happy,
  surprised, heart-eyes, dance ×2, walk ×2, eat ×2, sleep, excited,
  look-left, look-right.
- **Autonomous mood AI** — the cat randomly cycles through expressions on
  its own (mostly idle, occasionally something cuter), each with its own
  bounce/shift motion, so it never looks static.
- **Sliding lyric timeline** — a `LyricLine{ time, text }` struct array;
  lines ease-slide up and out as new ones ease-slide in, timed against
  `millis()`.
- **Auto word-wrap** — lyric lines too wide for the screen automatically
  split onto two rows instead of getting cut off.
- **Compact font + faux-bold** — lyrics render in the small `TomThumb` GFX
  font with a double-draw bold effect so more text fits comfortably.
- **Particle effects** — twinkling corner sparkles ✧, floating hearts ♡
  that drift upward and fade, a bouncing music note ♪.
- **Progress bar** that fills continuously against the song's duration.
- **Loading + ending screens** — a pulsing-dots loader before playback and
  a "thanks for listening" outro after the track ends, both non-blocking.
- **100% PROGMEM graphics** — every bitmap and lyric array lives in flash,
  not RAM.
- **No `delay()`, anywhere** — every subsystem is its own `millis()` timer.

---

## (=①ω①=) Hardware

| Part | Notes |
|---|---|
| ESP32 DevKit (any variant) | Dual-core, 3.3V logic |
| SSD1306 OLED, 128×64, **I2C** | Not the SPI variant — this sketch talks I2C |
| 4 jumper wires | VCC, GND, SDA, SCL |

> ⚠️ Make sure your OLED module is the **I2C** version (usually 4 pins:
> `GND VCC SCL SDA`), not the SPI version (usually 6–7 pins).

---

## (づ｡◕‿‿◕｡)づ Wiring

```
              ESP32 DevKit                      SSD1306 OLED (I2C, 128x64)
        ┌─────────────────────┐               ┌─────────────────────┐
        │                     │               │                     │
        │              3V3 ●──┼───────────────┼──● VCC              │
        │              GND ●──┼───────────────┼──● GND              │
        │          GPIO21 ●───┼───────────────┼──● SDA              │
        │          GPIO22 ●───┼───────────────┼──● SCL              │
        │                     │               │                     │
        └─────────────────────┘               └─────────────────────┘
```

| ESP32 Pin | OLED Pin | Purpose |
|---|---|---|
| `3V3` | `VCC` | Power (3.3V — **do not** use 5V unless your board is rated for it) |
| `GND` | `GND` | Ground |
| `GPIO21` | `SDA` | I2C data |
| `GPIO22` | `SCL` | I2C clock |

The sketch initializes I2C explicitly on those pins:

```cpp
Wire.begin(PIN_SDA, PIN_SCL);   // PIN_SDA = 21, PIN_SCL = 22
```

If your OLED doesn't light up, double-check its I2C address — most are
`0x3C`, but some are `0x3D`. That's set near the top of the sketch:

```cpp
#define SCREEN_ADDRESS 0x3C
```

---

## ( ˶ˆ ᗜ ˆ˶ ) Quick Start

1. **Install the ESP32 board package**
   Arduino IDE → `File > Preferences` → add this to *Additional Board
   Manager URLs*:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   Then `Tools > Board > Boards Manager` → search **esp32** → install.

2. **Install libraries** (`Sketch > Include Library > Manage Libraries`)
   - `Adafruit GFX Library`
   - `Adafruit SSD1306`
   *(`Wire.h` and the `TomThumb` font ship with those, no separate install
   needed.)*

3. **Wire it up** as shown [above](#-wiring).

4. **Select your board** under `Tools > Board` (e.g. "ESP32 Dev Module")
   and the correct COM/serial port.

5. **Open `OLED_MusicReel.ino`, hit Upload.** ٩(ˊᗜˋ*)و

---

## 灬(꒪ꇴ꒪)灬 How It's Structured

The sketch is deliberately split into small single-purpose functions rather
than one giant `loop()`:

```
loop()
 ├─ updatePlayerState()   → loading / playing / ended state machine
 ├─ updateCharacter()     → mood AI, blinking, breathing, frame timing
 ├─ updateEffects()       → sparkles, hearts, note bounce
 ├─ updateLyrics()        → figures out which lyric line is "now"
 └─ render()
     ├─ drawHeader()
     ├─ drawLyrics()      → sliding, wrapping, bold lyric text
     ├─ drawCharacter()
     ├─ drawEffects()
     ├─ drawProgressBar()
     └─ drawLoadingScreen() / drawEndingScreen()
```

A single frame limiter at the top of `loop()` caps everything to
`TARGET_FPS` (28 by default) using `millis()`, so the display is only ever
pushed once per frame — no flicker, no wasted I2C traffic.

---

## (｡•́︿•̀｡) Adding Your Own Lyrics

> **Note:** this repo ships with **placeholder lyric text** on purpose —
> real song lyrics are copyrighted, so you'll need to paste your own in.

Find the `SONG1_LYRICS[]` array (search for `PASTE YOUR OWN TIMESTAMPED
LYRICS HERE`) and edit the entries:

```cpp
const LyricLine SONG1_LYRICS[] PROGMEM = {
  { 0,     "your first line here" },
  { 3000,  "your second line here" },
  { 6500,  "your third line here" },
  // ...
};
```

- `time` is milliseconds from the start of playback (`t = 0`).
- Lines of any reasonable length are safe — anything too wide for the
  screen automatically wraps onto two rows.
- Update the song's total duration (used by the progress bar) where it's
  registered in `songList[]`:

```cpp
Song songList[] = {
  { "Untitled Track", SONG1_LYRICS, SONG1_LYRICS_LEN, 38000UL },
  //   ^ title                                          ^ total length, ms
};
```

---

## (๑˃ᴗ˂)ﻭ Adding More Songs

1. Make a new lyric array the same way as `SONG1_LYRICS`:
   ```cpp
   const LyricLine SONG2_LYRICS[] PROGMEM = { ... };
   const uint8_t SONG2_LYRICS_LEN = sizeof(SONG2_LYRICS) / sizeof(LyricLine);
   ```
2. Add it to `songList[]`:
   ```cpp
   Song songList[] = {
     { "Song One", SONG1_LYRICS, SONG1_LYRICS_LEN, 38000UL },
     { "Song Two", SONG2_LYRICS, SONG2_LYRICS_LEN, 45000UL },
   };
   ```
3. *(Optional)* to auto-advance through the list instead of looping the
   same track, change the `STATE_ENDED` branch in `updatePlayerState()` to
   increment `currentSongIndex` before resetting to `STATE_LOADING`.

---

## (=^-ω-^=) Adding New Cat Expressions

1. Draw a new 32×32 monochrome frame (any tool that exports a 1-bit
   bitmap works).
2. Add it as `const unsigned char cat_myNewPose[] PROGMEM = { ... };`
   next to the other bitmaps.
3. Add a value to `enum CharMood` (before `MOOD_COUNT`).
4. Add a `case MOOD_MYNEWPOSE:` branch in `getCharacterFrame()` returning
   `cat_myNewPose`.
5. Give it a chance to actually appear by adding a branch/weight inside
   the mood-transition roll in `updateCharacter()`.

For a *multi-frame* animation (like dance/walk), define an array of frame
pointers plus a length/ms constant, and hook it into the `frameMs` /
`frameLen` block near the top of `updateCharacter()`.

---

## ✎ Customizing Bold Text

Lyric bolding is a simple `true`/`false` flag passed into
`drawWrappedLyric()` from inside `drawLyrics()` — search for
`drawWrappedLyric(buf, y, true)` (it appears twice: once for the outgoing
line, once for the incoming line) and change `true` → `false` to turn
bold off. Change both together so the transition stays consistent.

> Note: the stock GFX fonts don't have a real italic variant, so this
> project only implements faux-bold (a 1px-offset double draw) — no
> italics.

---

## ( -_-)旦~ Memory Footprint

- **Flash (PROGMEM):** every bitmap (16 cat frames + 5 icons) plus all
  lyric arrays live in flash, not RAM — the whole graphics set is well
  under 3KB, negligible next to an ESP32's several MB of flash.
- **RAM:** dominated by the SSD1306 frame buffer itself
  (128×64/8 = 1024 bytes) plus a handful of small state/particle structs —
  a tiny fraction of the ESP32's ~320KB RAM.
- No `String` class, no dynamic allocation anywhere → no heap
  fragmentation risk even running for days at a time.

---

## (๑ↀᆺↀ๑) Known Limitations

- No true italic text (stock GFX fonts don't support it — see above).
- Lyrics are time-based, not tied to an actual audio source — there's no
  audio playback in this sketch, just a visual timeline. Pair it with
  your own audio trigger (Bluetooth, an amp module, serial command, etc.)
  if you want it synced to real playback.
- Single OLED size assumed (128×64). Other resolutions will need layout
  constants (`CHAR_BASE_X/Y`, `LYRIC_Y`, etc.) adjusted.

---

## ( ๑´•ω•) Credits

Cat character frames were procedurally redrawn (same proportions, matched
expressions) based on a hand-doodled reference sheet of cat poses, then
packed into individual 32×32 monochrome PROGMEM bitmaps for this project.

Built with [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)
and [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306).

```
     ,     ,
     (\____/)
      (_oo_)      thanks for reading, now go make it purr (=^･ω･^=)
      (u--u)
     _/|  |\_
```
