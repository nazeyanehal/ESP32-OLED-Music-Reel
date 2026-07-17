/*
  ===========================================================================
   OLED MUSIC REEL  —  ESP32 + SSD1306 128x64 "Aesthetic Now-Playing" Widget
  ===========================================================================

  A fully animated, Pinterest/Instagram-reel-style "now playing" display:
    - Song title header
    - Lyrics that slide in/out in sync with a timeline
    - A hand-doodled cat character with a dozen animation frames
      (idle/breathing, blinking, happy, surprised, heart-eyes, dancing,
       walking, eating, sleeping, excited)
    - Floating hearts, drifting sparkles, and a bouncing music note
    - A smoothly filling progress bar
    - A non-blocking loading screen and an ending screen

  ---------------------------------------------------------------------------
  HARDWARE
  ---------------------------------------------------------------------------
    ESP32 DevKit  +  SSD1306 128x64 I2C OLED
    SDA -> GPIO21
    SCL -> GPIO22
    VCC -> 3V3, GND -> GND

  ---------------------------------------------------------------------------
  LIBRARIES  (install via Library Manager)
  ---------------------------------------------------------------------------
    "Adafruit GFX Library"      by Adafruit
    "Adafruit SSD1306"          by Adafruit
    (Wire.h is bundled with the ESP32 core)

  ---------------------------------------------------------------------------
  DESIGN PRINCIPLES USED IN THIS SKETCH
  ---------------------------------------------------------------------------
    1. NO delay() ANYWHERE. Every animated element is driven by its own
       millis()-based timer, so nothing in the sketch ever blocks anything
       else — the character keeps breathing while lyrics scroll, sparkles
       drift, and the progress bar fills, all at once.
    2. All bitmap/animation data lives in PROGMEM (flash), not RAM.
    3. The screen is only pushed to the physical OLED (display.display())
       once per animation frame, at a fixed target FPS — never redrawn
       ad-hoc — which avoids flicker and wasted SPI/I2C bandwidth.
    4. Code is split into small single-purpose functions (drawHeader,
       drawLyrics, drawCharacter, drawProgressBar, drawEffects,
       updateAnimation, updateLyrics, updateEffects...) called once per
       frame from loop(), which itself stays tiny.

  ---------------------------------------------------------------------------
  HOW THE CHARACTER BITMAPS WERE MADE
  ---------------------------------------------------------------------------
  The uploaded reference sheet showed a cute square-bodied doodle cat with
  triangular ears, dot eyes, and a tiny mouth mark, drawn across many
  expressions (happy, surprised, sleepy, excited, eating, etc). Those poses
  were used as the visual reference and re-drawn as a matched family of
  32x32 monochrome pixel-art frames (same head shape/proportions, different
  eyes/mouth/ears/limbs per expression) so they animate as one consistent
  character rather than looking like unrelated stickers.
  ===========================================================================
*/

#include <string.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/TomThumb.h>   // compact bitmap font (bundled with Adafruit GFX)
                              // used only for lyrics, so long lines actually
                              // fit the 128px width without truncating.

// ============================================================================
// DISPLAY / HARDWARE CONFIG
// ============================================================================
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1     // Share ESP32 reset pin
#define SCREEN_ADDRESS 0x3C   // Most common SSD1306 I2C address (0x3D on some boards)

#define PIN_SDA 21
#define PIN_SCL 22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ============================================================================
// TIMING CONFIG
// ============================================================================
const uint16_t TARGET_FPS      = 28;                     // 25-30 FPS target
const uint16_t FRAME_INTERVAL  = 1000 / TARGET_FPS;       // ms per frame (~36ms)

// ============================================================================
// ------------------------  BITMAP DATA (PROGMEM)  --------------------------
// Procedurally generated from the reference doodle sheet: same 32x32 head
// shape/proportions across every expression, only eyes/mouth/ears/limbs
// change between frames so the animation reads as one continuous character.
// ============================================================================
// ==================== AUTO-GENERATED BITMAP DATA ====================
// Generated procedurally from doodle-style cat reference art.
// Format: Adafruit GFX 1-bit bitmap, MSB-first, row-padded to byte.

// cat_idle1  32x32px
const unsigned char cat_idle1[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x40, 0x01, 0x00,
  0x00, 0xC0, 0x01, 0x80,
  0x00, 0xE0, 0x03, 0x80,
  0x01, 0xE0, 0x03, 0xC0,
  0x03, 0xE0, 0x03, 0xE0,
  0x03, 0xF0, 0x07, 0xE0,
  0x07, 0xF0, 0x07, 0xF0,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x13, 0x00, 0x00, 0x64,
  0x06, 0x00, 0x00, 0x30,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x08, 0x08, 0x18,
  0x0C, 0x1C, 0x1C, 0x18,
  0x0D, 0xC8, 0x09, 0x98,
  0x0E, 0x00, 0x00, 0x78,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x02, 0x20, 0x18,
  0x0C, 0x01, 0x40, 0x18,
  0x0C, 0x00, 0x80, 0x18,
  0x06, 0x00, 0x00, 0x30,
  0x03, 0x00, 0x00, 0x60,
  0x01, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x80,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

// cat_idle2  32x32px
const unsigned char cat_idle2[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x40, 0x01, 0x00,
  0x00, 0xC0, 0x01, 0x80,
  0x00, 0xE0, 0x03, 0x80,
  0x01, 0xE0, 0x03, 0xC0,
  0x03, 0xE0, 0x03, 0xE0,
  0x03, 0xF0, 0x07, 0xE0,
  0x07, 0xF0, 0x07, 0xF0,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x13, 0x00, 0x00, 0x64,
  0x06, 0x00, 0x00, 0x30,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x08, 0x08, 0x18,
  0x0C, 0x1C, 0x1C, 0x18,
  0x0D, 0xC8, 0x09, 0x98,
  0x0E, 0x00, 0x00, 0x78,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x02, 0x20, 0x18,
  0x0C, 0x01, 0x40, 0x18,
  0x0C, 0x00, 0x80, 0x18,
  0x06, 0x00, 0x00, 0x30,
  0x03, 0x00, 0x00, 0x60,
  0x01, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x80,
  0x00, 0x00, 0x00, 0x00,
};

// cat_blink  32x32px
const unsigned char cat_blink[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x40, 0x01, 0x00,
  0x00, 0xC0, 0x01, 0x80,
  0x00, 0xE0, 0x03, 0x80,
  0x01, 0xE0, 0x03, 0xC0,
  0x03, 0xE0, 0x03, 0xE0,
  0x03, 0xF0, 0x07, 0xE0,
  0x07, 0xF0, 0x07, 0xF0,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x13, 0x00, 0x00, 0x64,
  0x06, 0x00, 0x00, 0x30,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x3E, 0x3E, 0x18,
  0x0D, 0xFE, 0x3F, 0x98,
  0x0E, 0x00, 0x00, 0x78,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x02, 0x20, 0x18,
  0x0C, 0x01, 0x40, 0x18,
  0x0C, 0x00, 0x80, 0x18,
  0x06, 0x00, 0x00, 0x30,
  0x03, 0x00, 0x00, 0x60,
  0x01, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x80,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

// cat_happy  32x32px
const unsigned char cat_happy[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x40, 0x01, 0x00,
  0x00, 0xC0, 0x01, 0x80,
  0x00, 0xE0, 0x03, 0x80,
  0x01, 0xE0, 0x03, 0xC0,
  0x03, 0xE0, 0x03, 0xE0,
  0x03, 0xF0, 0x07, 0xE0,
  0x07, 0xF0, 0x07, 0xF0,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x13, 0x00, 0x00, 0x64,
  0x06, 0x00, 0x00, 0x30,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x08, 0x08, 0x18,
  0x0C, 0x14, 0x14, 0x18,
  0x0C, 0x14, 0x14, 0x18,
  0x0D, 0xE2, 0x23, 0x98,
  0x0E, 0x00, 0x00, 0x78,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x0C, 0x18, 0x18,
  0x0C, 0x0E, 0x38, 0x18,
  0x06, 0x07, 0xF0, 0x30,
  0x03, 0x01, 0xC0, 0x60,
  0x01, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x80,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

// cat_surprised  32x32px
const unsigned char cat_surprised[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x40, 0x01, 0x00,
  0x00, 0xC0, 0x01, 0x80,
  0x00, 0xE0, 0x03, 0x80,
  0x01, 0xE0, 0x03, 0xC0,
  0x03, 0xE0, 0x03, 0xE0,
  0x03, 0xF0, 0x07, 0xE0,
  0x07, 0xF0, 0x07, 0xF0,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x13, 0x00, 0x00, 0x64,
  0x06, 0x00, 0x00, 0x30,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x18, 0x0C, 0x18,
  0x0C, 0x24, 0x12, 0x18,
  0x0C, 0x5A, 0x2D, 0x18,
  0x0C, 0x5A, 0x2D, 0x18,
  0x0C, 0x24, 0x12, 0x18,
  0x0C, 0x18, 0x0C, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x01, 0xC0, 0x18,
  0x0C, 0x02, 0x20, 0x18,
  0x0C, 0x02, 0x20, 0x18,
  0x06, 0x02, 0x20, 0x30,
  0x03, 0x01, 0xC0, 0x60,
  0x01, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x80,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

// cat_heart_eyes  32x32px
const unsigned char cat_heart_eyes[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x40, 0x01, 0x00,
  0x00, 0xC0, 0x01, 0x80,
  0x00, 0xE0, 0x03, 0x80,
  0x01, 0xE0, 0x03, 0xC0,
  0x03, 0xE0, 0x03, 0xE0,
  0x03, 0xF0, 0x07, 0xE0,
  0x07, 0xF0, 0x07, 0xF0,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x13, 0x00, 0x00, 0x64,
  0x06, 0x00, 0x00, 0x30,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x36, 0x36, 0x18,
  0x0C, 0x7F, 0x7F, 0x18,
  0x0C, 0x7F, 0x7F, 0x18,
  0x0C, 0x3E, 0x3E, 0x18,
  0x0C, 0x3E, 0x3E, 0x18,
  0x0C, 0x1C, 0x1C, 0x18,
  0x0C, 0x1C, 0x1C, 0x18,
  0x0C, 0x08, 0x08, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x0C, 0x18, 0x18,
  0x0C, 0x0E, 0x38, 0x18,
  0x06, 0x07, 0xF0, 0x30,
  0x03, 0x01, 0xC0, 0x60,
  0x01, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x80,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

// cat_dance1  32x32px
const unsigned char cat_dance1[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x40, 0x00, 0x00,
  0x00, 0xC0, 0x00, 0x00,
  0x00, 0xE0, 0x00, 0x00,
  0x01, 0xE0, 0x01, 0x00,
  0x03, 0xE0, 0x01, 0x80,
  0x03, 0xF0, 0x03, 0x80,
  0x07, 0xF0, 0x03, 0xC0,
  0x0F, 0xFF, 0xFF, 0xE0,
  0x0F, 0xFF, 0xFF, 0xF0,
  0x1C, 0x00, 0x0F, 0xF0,
  0x18, 0x00, 0x00, 0xF8,
  0x30, 0x00, 0x00, 0x64,
  0x30, 0x00, 0x00, 0x60,
  0x30, 0x00, 0x00, 0x60,
  0x30, 0x00, 0x00, 0x60,
  0x30, 0x00, 0x00, 0x60,
  0x30, 0x20, 0x20, 0x60,
  0x30, 0x70, 0x70, 0x60,
  0x30, 0x20, 0x20, 0x60,
  0x30, 0x00, 0x00, 0x60,
  0x30, 0x00, 0x00, 0x60,
  0x30, 0x00, 0x00, 0x60,
  0x30, 0x08, 0x80, 0x60,
  0x30, 0x05, 0x00, 0x60,
  0x30, 0x02, 0x00, 0x60,
  0x18, 0x00, 0x00, 0xD0,
  0x0C, 0x00, 0x01, 0xB8,
  0x07, 0xFF, 0xFF, 0x1E,
  0x03, 0xFF, 0xFE, 0x07,
  0x00, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x00,
};

// cat_dance2  32x32px
const unsigned char cat_dance2[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x01, 0x80,
  0x00, 0x00, 0x03, 0x80,
  0x00, 0x40, 0x03, 0xC0,
  0x00, 0xC0, 0x03, 0xE0,
  0x00, 0xE0, 0x07, 0xE0,
  0x01, 0xE0, 0x07, 0xF0,
  0x03, 0xFF, 0xFF, 0xF8,
  0x07, 0xFF, 0xFF, 0xF8,
  0x07, 0xF8, 0x00, 0x1C,
  0x0F, 0x80, 0x00, 0x0C,
  0x13, 0x00, 0x00, 0x06,
  0x03, 0x00, 0x00, 0x06,
  0x03, 0x00, 0x00, 0x06,
  0x03, 0x00, 0x00, 0x06,
  0x03, 0x00, 0x00, 0x06,
  0x03, 0x02, 0x02, 0x06,
  0x03, 0x07, 0x07, 0x06,
  0x03, 0x02, 0x02, 0x06,
  0x03, 0x00, 0x00, 0x06,
  0x03, 0x00, 0x00, 0x06,
  0x03, 0x00, 0x00, 0x06,
  0x03, 0x00, 0x88, 0x06,
  0x03, 0x00, 0x50, 0x06,
  0x03, 0x00, 0x20, 0x06,
  0x07, 0x80, 0x00, 0x0C,
  0x1E, 0xC0, 0x00, 0x18,
  0x38, 0x7F, 0xFF, 0xF0,
  0x10, 0x3F, 0xFF, 0xE0,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

// cat_walk1  32x32px
const unsigned char cat_walk1[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x40, 0x01, 0x00,
  0x00, 0xC0, 0x01, 0x80,
  0x00, 0xE0, 0x03, 0x80,
  0x01, 0xE0, 0x03, 0xC0,
  0x03, 0xE0, 0x03, 0xE0,
  0x03, 0xF0, 0x07, 0xE0,
  0x07, 0xF0, 0x07, 0xF0,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x13, 0x00, 0x00, 0x64,
  0x06, 0x00, 0x00, 0x30,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x08, 0x08, 0x18,
  0x0C, 0x1C, 0x1C, 0x18,
  0x0C, 0x08, 0x08, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x02, 0x60, 0x18,
  0x0C, 0x01, 0x80, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x06, 0x00, 0x00, 0x30,
  0x03, 0x00, 0x00, 0x60,
  0x01, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x80,
  0x00, 0x70, 0x07, 0x00,
  0x00, 0x20, 0x03, 0x80,
};

// cat_walk2  32x32px
const unsigned char cat_walk2[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x40, 0x01, 0x00,
  0x00, 0xC0, 0x01, 0x80,
  0x00, 0xE0, 0x03, 0x80,
  0x01, 0xE0, 0x03, 0xC0,
  0x03, 0xE0, 0x03, 0xE0,
  0x03, 0xF0, 0x07, 0xE0,
  0x07, 0xF0, 0x07, 0xF0,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x13, 0x00, 0x00, 0x64,
  0x06, 0x00, 0x00, 0x30,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x08, 0x08, 0x18,
  0x0C, 0x1C, 0x1C, 0x18,
  0x0C, 0x08, 0x08, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x02, 0x60, 0x18,
  0x0C, 0x01, 0x80, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x06, 0x00, 0x00, 0x30,
  0x03, 0x00, 0x00, 0x60,
  0x01, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x80,
  0x00, 0x1C, 0x1C, 0x00,
  0x00, 0x0E, 0x08, 0x00,
};

// cat_eat1  32x32px
const unsigned char cat_eat1[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x40, 0x01, 0x00,
  0x00, 0xC0, 0x01, 0x80,
  0x00, 0xE0, 0x03, 0x80,
  0x01, 0xE0, 0x03, 0xC0,
  0x03, 0xE0, 0x03, 0xE0,
  0x03, 0xF0, 0x07, 0xE0,
  0x07, 0xF0, 0x07, 0xF0,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x13, 0x00, 0x00, 0x64,
  0x06, 0x00, 0x00, 0x30,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x08, 0x08, 0x18,
  0x0C, 0x1C, 0x1C, 0x18,
  0x0C, 0x08, 0x08, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x02, 0x20, 0x18,
  0x0C, 0x01, 0x40, 0x18,
  0x0C, 0x00, 0x80, 0x18,
  0x06, 0x00, 0x01, 0xB0,
  0x03, 0x00, 0x02, 0x60,
  0x01, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x80,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

// cat_eat2  32x32px
const unsigned char cat_eat2[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x40, 0x01, 0x00,
  0x00, 0xC0, 0x01, 0x80,
  0x00, 0xE0, 0x03, 0x80,
  0x01, 0xE0, 0x03, 0xC0,
  0x03, 0xE0, 0x03, 0xE0,
  0x03, 0xF0, 0x07, 0xE0,
  0x07, 0xF0, 0x07, 0xF0,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x13, 0x00, 0x00, 0x64,
  0x06, 0x00, 0x00, 0x30,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x08, 0x08, 0x18,
  0x0C, 0x1C, 0x1C, 0x18,
  0x0C, 0x08, 0x08, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x01, 0xC0, 0x18,
  0x0C, 0x03, 0xE0, 0x18,
  0x0C, 0x07, 0xF1, 0xD8,
  0x0C, 0x07, 0xF2, 0x38,
  0x06, 0x03, 0xE2, 0x30,
  0x03, 0x01, 0xC2, 0x60,
  0x01, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x80,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

// cat_sleep  32x32px
const unsigned char cat_sleep[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x40, 0x01, 0x00,
  0x00, 0xC0, 0x01, 0x80,
  0x00, 0xE0, 0x03, 0x80,
  0x01, 0xE0, 0x03, 0xC0,
  0x03, 0xFF, 0xFF, 0xE0,
  0x07, 0xFF, 0xFF, 0xF0,
  0x07, 0xF8, 0x0F, 0xF0,
  0x0F, 0x00, 0x00, 0x78,
  0x1C, 0x00, 0x00, 0x1C,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x3E, 0x3E, 0x18,
  0x0C, 0x3E, 0x3E, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x01, 0xC0, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x06, 0x00, 0x00, 0x30,
  0x03, 0x00, 0x00, 0x60,
  0x01, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x80,
};

// cat_excited  32x32px
const unsigned char cat_excited[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x40, 0x01, 0x00,
  0x00, 0xC0, 0x01, 0x80,
  0x00, 0xE0, 0x03, 0x80,
  0x01, 0xE0, 0x03, 0xC0,
  0x03, 0xE0, 0x03, 0xE0,
  0x03, 0xF0, 0x07, 0xE0,
  0x07, 0xF0, 0x07, 0xF0,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x13, 0x00, 0x00, 0x64,
  0x06, 0x00, 0x00, 0x30,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x1C, 0x1C, 0x18,
  0x0C, 0x2A, 0x2A, 0x18,
  0x0C, 0xFE, 0x3F, 0x18,
  0x0F, 0x2A, 0x2A, 0xD8,
  0x0C, 0x1C, 0x1C, 0x38,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x01, 0xC0, 0x18,
  0x0C, 0x03, 0xE0, 0x18,
  0x0C, 0x07, 0xF0, 0x18,
  0x0C, 0x07, 0xF0, 0x18,
  0x06, 0x07, 0xF0, 0x30,
  0x03, 0x03, 0xE0, 0x60,
  0x01, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x80,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

// cat_look_left  32x32px
const unsigned char cat_look_left[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x40, 0x01, 0x00,
  0x00, 0xC0, 0x01, 0x80,
  0x00, 0xE0, 0x03, 0x80,
  0x01, 0xE0, 0x03, 0xC0,
  0x03, 0xE0, 0x03, 0xE0,
  0x03, 0xF0, 0x07, 0xE0,
  0x07, 0xF0, 0x07, 0xF0,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x13, 0x00, 0x00, 0x64,
  0x06, 0x00, 0x00, 0x30,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x20, 0x20, 0x18,
  0x0C, 0x70, 0x70, 0x18,
  0x0D, 0xE0, 0x21, 0x98,
  0x0E, 0x00, 0x00, 0x78,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x02, 0x60, 0x18,
  0x0C, 0x01, 0x80, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x06, 0x00, 0x00, 0x30,
  0x03, 0x00, 0x00, 0x60,
  0x01, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x80,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

// cat_look_right  32x32px
const unsigned char cat_look_right[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x40, 0x01, 0x00,
  0x00, 0xC0, 0x01, 0x80,
  0x00, 0xE0, 0x03, 0x80,
  0x01, 0xE0, 0x03, 0xC0,
  0x03, 0xE0, 0x03, 0xE0,
  0x03, 0xF0, 0x07, 0xE0,
  0x07, 0xF0, 0x07, 0xF0,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x0F, 0xFF, 0xFF, 0xF8,
  0x13, 0x00, 0x00, 0x64,
  0x06, 0x00, 0x00, 0x30,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x02, 0x02, 0x18,
  0x0C, 0x07, 0x07, 0x18,
  0x0D, 0xC2, 0x03, 0x98,
  0x0E, 0x00, 0x00, 0x78,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x0C, 0x02, 0x60, 0x18,
  0x0C, 0x01, 0x80, 0x18,
  0x0C, 0x00, 0x00, 0x18,
  0x06, 0x00, 0x00, 0x30,
  0x03, 0x00, 0x00, 0x60,
  0x01, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x80,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

// icon_note  8x8px
const unsigned char icon_note[] PROGMEM = {
  0x04,
  0x07,
  0x04,
  0x04,
  0x1C,
  0x24,
  0x24,
  0x18,
};

// icon_heart  8x8px
const unsigned char icon_heart[] PROGMEM = {
  0x00,
  0x66,
  0xFF,
  0xFF,
  0x7E,
  0x3C,
  0x38,
  0x10,
};

// icon_star  8x8px
const unsigned char icon_star[] PROGMEM = {
  0x10,
  0x10,
  0x10,
  0xF0,
  0x0F,
  0x08,
  0x08,
  0x08,
};

// icon_sparkle  6x6px
const unsigned char icon_sparkle[] PROGMEM = {
  0x20,
  0x20,
  0xE0,
  0x1C,
  0x10,
  0x10,
};

// icon_zzz  8x8px
const unsigned char icon_zzz[] PROGMEM = {
  0x00,
  0xF8,
  0x10,
  0x20,
  0x20,
  0x40,
  0xF8,
  0x00,
};


// ============================================================================
// ------------------------  ANIMATION FRAME TABLES  -------------------------
// Each "mood" is a small array of bitmap pointers (kept in PROGMEM as an
// array of pointers) plus how many ms to hold each frame. updateCharacter()
// walks through these tables using millis() — no delay() involved.
// ============================================================================
#define CAT_W 32
#define CAT_H 32
#define ICON_W 8
#define ICON_H 8
#define SPARKLE_W 6
#define SPARKLE_H 6

// Idle / breathing loop (2 frames)
const unsigned char* const ANIM_IDLE[] = { cat_idle1, cat_idle2 };
const uint8_t ANIM_IDLE_LEN = 2;
const uint16_t ANIM_IDLE_MS = 450;

// Blink overlay (shown briefly on top of idle)
const unsigned char* const ANIM_BLINK[] = { cat_blink };

// Dance (2 frames, tilt left/right)
const unsigned char* const ANIM_DANCE[] = { cat_dance1, cat_dance2 };
const uint8_t ANIM_DANCE_LEN = 2;
const uint16_t ANIM_DANCE_MS = 180;

// Walk (2 frames, feet alternate)
const unsigned char* const ANIM_WALK[] = { cat_walk1, cat_walk2 };
const uint8_t ANIM_WALK_LEN = 2;
const uint16_t ANIM_WALK_MS = 200;

// Eat (2 frames, mouth open/close on a crumb)
const unsigned char* const ANIM_EAT[] = { cat_eat1, cat_eat2 };
const uint8_t ANIM_EAT_LEN = 2;
const uint16_t ANIM_EAT_MS = 260;

// Single-pose expressions (one frame, held with a gentle bounce/pulse)
// (cat_happy, cat_surprised, cat_heart_eyes, cat_excited, cat_sleep,
//  cat_look_left, cat_look_right used directly by pointer, see below)

// ============================================================================
// ------------------------  CHARACTER MOOD STATE  ----------------------------
// ============================================================================
enum CharMood {
  MOOD_IDLE = 0,
  MOOD_HAPPY,
  MOOD_SURPRISED,
  MOOD_HEART,
  MOOD_DANCE,
  MOOD_WALK,
  MOOD_EAT,
  MOOD_SLEEP,
  MOOD_EXCITED,
  MOOD_LOOK_LEFT,
  MOOD_LOOK_RIGHT,
  MOOD_COUNT
};

CharMood currentMood       = MOOD_IDLE;
unsigned long moodStartMs  = 0;
unsigned long moodHoldMs   = 4000;

unsigned long lastIdleFrameMs = 0;
uint8_t idleFrameIndex        = 0;

unsigned long lastMultiFrameMs = 0;
uint8_t multiFrameIndex        = 0;

bool blinkActive           = false;
unsigned long blinkEndMs    = 0;
unsigned long nextBlinkAtMs = 0;

int8_t  charX = 48;                 // base x position (character is 32px wide, centered-ish)
const int8_t CHAR_BASE_X = 48;
const int8_t CHAR_BASE_Y = 30;      // top-left y of the 32x32 character box

// ============================================================================
// ------------------------  LYRICS DATA STRUCTURE  --------------------------
// Timestamps are in milliseconds from the moment playback starts (t=0).
// To sync with a real track, just fill in the correct ms offsets.
// ============================================================================
struct LyricLine {
  uint32_t    time;   // ms from song start when this line should appear
  const char* text;   // lyric text. Any length up to LYRIC_BUF_SIZE-1 chars is
                       // safe — drawLyrics() copies into a fixed buffer sized
                       // by LYRIC_BUF_SIZE below, and long lines auto-wrap to
                       // two rows on screen (see drawWrappedLyric()).
};

/////////////////////////////////////////////////////////////////////////////
// PASTE YOUR OWN TIMESTAMPED LYRICS HERE
// -----------------------------------------------------------------------
// NOTE ON COPYRIGHT: this sketch intentionally ships with placeholder
// lines instead of real song lyrics — commercial lyrics are copyrighted
// text, so they aren't included here. Replace the placeholder array below
// with your own lines and real timestamps (ms from song start). Keep the
// LyricLine{ time, "text" } format and end the array with the closing
// brace exactly as shown; nothing else in the sketch needs to change.
/////////////////////////////////////////////////////////////////////////////
const LyricLine SONG1_LYRICS[] PROGMEM = {
  { 0,     "( intro )" },
  { 3000,  "-- line 1 goes here --" },
  { 6500,  "-- line 2 goes here --" },
  { 10000, "-- line 3 goes here --" },
  { 13500, "-- line 4 goes here --" },
  { 17000, "-- line 5 goes here --" },
  { 20500, "-- line 6 goes here --" },
  { 24000, "-- line 7 goes here --" },
  { 27500, "-- line 8 goes here --" },
  { 31000, "-- line 9 goes here --" },
  { 34500, "-- line 10 goes here --" },
};
const uint8_t SONG1_LYRICS_LEN = sizeof(SONG1_LYRICS) / sizeof(LyricLine);
/////////////////////////////////////////////////////////////////////////////
// END OF EDITABLE LYRICS SECTION
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// ------------------------  SONG DATA STRUCTURE  ----------------------------
// Add more Song entries + point songList at more slots to support several
// tracks (see "HOW TO ADD MORE SONGS" in the writeup below the code).
// ============================================================================
struct Song {
  const char*       title;
  const LyricLine*  lyrics;
  uint8_t           lyricCount;
  uint32_t          durationMs;   // total track length, drives the progress bar
};

Song songList[] = {
  { "Untitled Track", SONG1_LYRICS, SONG1_LYRICS_LEN, 38000UL },
};
const uint8_t SONG_COUNT = sizeof(songList) / sizeof(Song);
uint8_t currentSongIndex = 0;

// ============================================================================
// ------------------------  PLAYBACK STATE MACHINE  -------------------------
// ============================================================================
enum PlayerState { STATE_LOADING, STATE_PLAYING, STATE_ENDED };
PlayerState playerState   = STATE_LOADING;
unsigned long stateStartMs = 0;
unsigned long songStartMs  = 0;     // millis() when playback ("t=0") began

const unsigned long LOADING_DURATION_MS = 2200;
const unsigned long ENDED_DURATION_MS   = 3000;

// ============================================================================
// ------------------------  LYRIC SCROLL / TRANSITION STATE  ----------------
// ============================================================================
int8_t  activeLyricIdx   = -1;   // index currently shown/animating in
int8_t  lastLyricIdx     = -1;   // index that was showing before this change
unsigned long lyricChangeMs = 0;
const unsigned long LYRIC_SLIDE_MS = 260;   // duration of the slide transition
const int LYRIC_Y = 22;                     // vertical center of the lyric viewport
const int LYRIC_SLIDE_DIST = 16;            // px traveled during slide

// Buffer used to copy a lyric line out of PROGMEM before drawing it.
// THIS WAS THE BUG: it used to be sized 32, and any lyric line 32+ chars
// long (with its null terminator) overflowed it and corrupted the stack —
// which is exactly why playback would stall partway through a song. 64 is
// comfortable headroom for normal song lines; bump it further if you ever
// write a single lyric line longer than 63 characters.
#define LYRIC_BUF_SIZE 64

// ============================================================================
// ------------------------  PARTICLE EFFECTS  -------------------------------
// ============================================================================
struct Sparkle {
  int8_t  x, y;
  bool    active;
  unsigned long nextToggleMs;
  bool    on;
};
#define NUM_SPARKLES 4
Sparkle sparkles[NUM_SPARKLES];

struct Heart {
  float   x, y;
  bool    active;
  unsigned long spawnMs;
};
#define NUM_HEARTS 3
Heart hearts[NUM_HEARTS];
unsigned long nextHeartSpawnMs = 0;

// Bouncing music note (single, top-right corner)
float noteBouncePhase = 0;

// ============================================================================
// ------------------------  FRAME TIMER  -------------------------------------
// ============================================================================
unsigned long lastFrameMs = 0;

// ============================================================================
// ------------------------  FORWARD DECLARATIONS  ----------------------------
// (The Arduino IDE auto-generates these; they're spelled out explicitly here
//  so the sketch also compiles cleanly with plain gcc/g++ toolchains.)
// ============================================================================
void updatePlayerState(unsigned long now);
void updateCharacter(unsigned long now);
const unsigned char* getCharacterFrame(unsigned long now, int8_t &bounceYOut, int8_t &shiftXOut);
void updateLyrics(unsigned long now);
void initEffects();
void updateEffects(unsigned long now);
void render(unsigned long now);
void drawHeader();
void drawLyrics(unsigned long now);
void drawCenteredText(const char* text, int16_t y);
void drawTextCenteredAtY(const char* text, int16_t y, bool bold);
void drawWrappedLyric(const char* text, int16_t centerY, bool bold);
void drawCharacter(unsigned long now);
void drawEffects(unsigned long now);
void drawProgressBar(unsigned long now);
void drawLoadingScreen(unsigned long now);
void drawEndingScreen(unsigned long now);
void drawCenteredTextF(const char* text, int16_t y);

// ============================================================================
// ============================    SETUP    ===================================
// ============================================================================
void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_SDA, PIN_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) { /* halt — no delay(), but nothing else to do if the screen is dead */ }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);

  randomSeed(analogRead(0));

  initEffects();

  stateStartMs = millis();
  playerState  = STATE_LOADING;
}

// ============================================================================
// ============================     LOOP     ===================================
// ============================================================================
void loop() {
  unsigned long now = millis();

  // ---- Frame limiter: only do work / redraw at TARGET_FPS, never block ----
  if (now - lastFrameMs < FRAME_INTERVAL) {
    return;                       // nothing to do yet, loop again immediately
  }
  lastFrameMs = now;

  updatePlayerState(now);
  updateCharacter(now);
  updateEffects(now);
  if (playerState == STATE_PLAYING) {
    updateLyrics(now);
  }

  render(now);
}

// ============================================================================
// ------------------------  PLAYER STATE MACHINE  ----------------------------
// ============================================================================
void updatePlayerState(unsigned long now) {
  switch (playerState) {
    case STATE_LOADING:
      if (now - stateStartMs >= LOADING_DURATION_MS) {
        playerState  = STATE_PLAYING;
        stateStartMs = now;
        songStartMs  = now;
        activeLyricIdx = -1;
        lastLyricIdx   = -1;
      }
      break;

    case STATE_PLAYING: {
      const Song& song = songList[currentSongIndex];
      if (now - songStartMs >= song.durationMs) {
        playerState  = STATE_ENDED;
        stateStartMs = now;
      }
      break;
    }

    case STATE_ENDED:
      if (now - stateStartMs >= ENDED_DURATION_MS) {
        // Loop back to the start of the (same) song. To auto-advance to the
        // next track instead, do: currentSongIndex = (currentSongIndex+1) % SONG_COUNT;
        playerState  = STATE_LOADING;
        stateStartMs = now;
      }
      break;
  }
}

// ============================================================================
// ------------------------  CHARACTER AI / ANIMATION  ------------------------
// ============================================================================
void updateCharacter(unsigned long now) {
  // ---- Idle breathing loop (always advances in the background) ----
  if (now - lastIdleFrameMs >= ANIM_IDLE_MS) {
    lastIdleFrameMs = now;
    idleFrameIndex = (idleFrameIndex + 1) % ANIM_IDLE_LEN;
  }

  // ---- Multi-frame moods (dance/walk/eat) advance their own frame index ----
  uint16_t frameMs = 0;
  uint8_t  frameLen = 0;
  if (currentMood == MOOD_DANCE) { frameMs = ANIM_DANCE_MS; frameLen = ANIM_DANCE_LEN; }
  else if (currentMood == MOOD_WALK) { frameMs = ANIM_WALK_MS; frameLen = ANIM_WALK_LEN; }
  else if (currentMood == MOOD_EAT)  { frameMs = ANIM_EAT_MS;  frameLen = ANIM_EAT_LEN; }

  if (frameLen > 0 && now - lastMultiFrameMs >= frameMs) {
    lastMultiFrameMs = now;
    multiFrameIndex = (multiFrameIndex + 1) % frameLen;
  }

  // ---- Random blink, but only layered over plain idle ----
  if (currentMood == MOOD_IDLE) {
    if (!blinkActive && now >= nextBlinkAtMs) {
      blinkActive = true;
      blinkEndMs  = now + 130;               // short blink
    }
    if (blinkActive && now >= blinkEndMs) {
      blinkActive = false;
      nextBlinkAtMs = now + random(2000, 5000);   // schedule next blink
    }
  } else {
    blinkActive = false;
  }

  // ---- Mood transition: after moodHoldMs, decide the next mood ----
  if (now - moodStartMs >= moodHoldMs) {
    moodStartMs = now;
    multiFrameIndex = 0;

    // Weighted-ish random pick: most of the time go back to idle so the
    // character doesn't feel manic, but keep it "alive" with variety.
    if (currentMood != MOOD_IDLE) {
      currentMood = MOOD_IDLE;
      moodHoldMs = random(3000, 6000);
    } else {
      uint8_t roll = random(0, 100);
      if      (roll < 45) { currentMood = MOOD_IDLE;        moodHoldMs = random(3000, 6000); }
      else if (roll < 55) { currentMood = MOOD_HAPPY;       moodHoldMs = 1400; }
      else if (roll < 62) { currentMood = MOOD_SURPRISED;   moodHoldMs = 1100; }
      else if (roll < 70) { currentMood = MOOD_HEART;       moodHoldMs = 1600; }
      else if (roll < 78) { currentMood = MOOD_DANCE;       moodHoldMs = 1800; }
      else if (roll < 84) { currentMood = MOOD_WALK;        moodHoldMs = 1600; }
      else if (roll < 90) { currentMood = MOOD_EAT;         moodHoldMs = 1500; }
      else if (roll < 94) { currentMood = MOOD_EXCITED;     moodHoldMs = 1200; }
      else if (roll < 97) { currentMood = MOOD_LOOK_LEFT;   moodHoldMs = 900;  }
      else                { currentMood = MOOD_LOOK_RIGHT;  moodHoldMs = 900;  }
    }
  }

  // While the whole player is asleep/ended, force the sleep pose.
  if (playerState == STATE_ENDED) {
    currentMood = MOOD_SLEEP;
  }
}

// Returns which bitmap should be drawn for the character *this frame*,
// and writes the current bounce (y) offset into bounceYOut.
const unsigned char* getCharacterFrame(unsigned long now, int8_t &bounceYOut, int8_t &shiftXOut) {
  bounceYOut = 0;
  shiftXOut  = 0;

  // Idle breathing bounce: small continuous sine wave
  float t = now / 1000.0f;
  int8_t idleBounce = (int8_t)round(sin(t * 3.2f) * 1.5f);

  switch (currentMood) {
    case MOOD_IDLE:
      bounceYOut = idleBounce;
      if (blinkActive) return ANIM_BLINK[0];
      return ANIM_IDLE[idleFrameIndex];

    case MOOD_HAPPY:
      bounceYOut = (int8_t)round(sin(t * 6.0f) * 2.0f);
      return cat_happy;

    case MOOD_SURPRISED:
      bounceYOut = -2;   // a little "pop up"
      return cat_surprised;

    case MOOD_HEART:
      bounceYOut = (int8_t)round(sin(t * 5.0f) * 1.5f);
      return cat_heart_eyes;

    case MOOD_DANCE:
      bounceYOut = (int8_t)round(sin(t * 9.0f) * 3.0f);
      shiftXOut  = (multiFrameIndex == 0) ? -2 : 2;
      return ANIM_DANCE[multiFrameIndex];

    case MOOD_WALK:
      shiftXOut  = (int8_t)round(sin(t * 4.0f) * 6.0f);
      bounceYOut = (multiFrameIndex == 0) ? 0 : -1;
      return ANIM_WALK[multiFrameIndex];

    case MOOD_EAT:
      bounceYOut = 0;
      return ANIM_EAT[multiFrameIndex];

    case MOOD_SLEEP:
      bounceYOut = (int8_t)round(sin(t * 1.5f) * 1.0f);   // slow sleep breathing
      return cat_sleep;

    case MOOD_EXCITED:
      bounceYOut = (int8_t)round(sin(t * 10.0f) * 3.0f);
      return cat_excited;

    case MOOD_LOOK_LEFT:
      return cat_look_left;

    case MOOD_LOOK_RIGHT:
      return cat_look_right;

    default:
      return ANIM_IDLE[0];
  }
}

// ============================================================================
// ------------------------  LYRIC TIMELINE  ----------------------------------
// ============================================================================
void updateLyrics(unsigned long now) {
  const Song& song = songList[currentSongIndex];
  unsigned long elapsed = now - songStartMs;

  // Find which lyric line should be showing right now (the last one whose
  // timestamp has passed).
  int8_t target = activeLyricIdx;
  for (int8_t i = 0; i < (int8_t)song.lyricCount; i++) {
    uint32_t t = pgm_read_dword(&song.lyrics[i].time);
    if (elapsed >= t) target = i;
    else break;
  }

  if (target != activeLyricIdx) {
    lastLyricIdx   = activeLyricIdx;
    activeLyricIdx = target;
    lyricChangeMs  = now;
  }
}

// ============================================================================
// ------------------------  PARTICLE EFFECTS: INIT/UPDATE  -------------------
// ============================================================================
void initEffects() {
  for (uint8_t i = 0; i < NUM_SPARKLES; i++) {
    sparkles[i].active = true;
    sparkles[i].on = false;
    sparkles[i].nextToggleMs = millis() + random(300, 1500);
    // Corners / edges only, so they don't collide with the character or text
    switch (i % 4) {
      case 0: sparkles[i].x = random(2, 14);              sparkles[i].y = random(10, 18); break;
      case 1: sparkles[i].x = random(114, 124);            sparkles[i].y = random(10, 18); break;
      case 2: sparkles[i].x = random(2, 14);               sparkles[i].y = random(36, 50); break;
      case 3: sparkles[i].x = random(114, 124);            sparkles[i].y = random(36, 50); break;
    }
  }
  for (uint8_t i = 0; i < NUM_HEARTS; i++) {
    hearts[i].active = false;
  }
  nextHeartSpawnMs = millis() + random(1500, 3500);
}

void updateEffects(unsigned long now) {
  // ---- Sparkles: twinkle on/off at random intervals ----
  for (uint8_t i = 0; i < NUM_SPARKLES; i++) {
    if (now >= sparkles[i].nextToggleMs) {
      sparkles[i].on = !sparkles[i].on;
      sparkles[i].nextToggleMs = now + (sparkles[i].on ? random(200, 500) : random(600, 2000));
    }
  }

  // ---- Floating hearts: spawn occasionally, drift upward, fade (despawn) ----
  if (now >= nextHeartSpawnMs) {
    for (uint8_t i = 0; i < NUM_HEARTS; i++) {
      if (!hearts[i].active) {
        hearts[i].active = true;
        hearts[i].spawnMs = now;
        hearts[i].x = random(6, 122);
        hearts[i].y = 62;
        break;
      }
    }
    nextHeartSpawnMs = now + random(2000, 4500);
  }
  for (uint8_t i = 0; i < NUM_HEARTS; i++) {
    if (hearts[i].active) {
      unsigned long age = now - hearts[i].spawnMs;
      hearts[i].y = 62.0f - (age / 1000.0f) * 14.0f;     // drift upward
      hearts[i].x += sinf((now + i * 500) / 300.0f) * 0.15f; // gentle sway
      if (hearts[i].y < 10 || age > 4000) hearts[i].active = false;
    }
  }

  // ---- Bouncing music note phase ----
  noteBouncePhase = now / 1000.0f;
}

// ============================================================================
// ------------------------  RENDER (drawing only)  ---------------------------
// ============================================================================
void render(unsigned long now) {
  display.clearDisplay();

  switch (playerState) {
    case STATE_LOADING:
      drawLoadingScreen(now);
      break;

    case STATE_PLAYING:
      drawHeader();
      drawLyrics(now);
      drawCharacter(now);
      drawEffects(now);
      drawProgressBar(now);
      break;

    case STATE_ENDED:
      drawEndingScreen(now);
      drawCharacter(now);   // cat naps while the "ended" screen shows
      drawEffects(now);
      break;
  }

  display.display();
}

// ---- Header: song title, top of screen -------------------------------------
void drawHeader() {
  const Song& song = songList[currentSongIndex];
  display.setTextSize(1);
  display.setCursor(4, 1);
  display.print(song.title);
  display.drawFastHLine(0, 9, SCREEN_WIDTH, SSD1306_WHITE);
}

// ---- Lyrics: sliding transition between the previous and current line -----
// Uses the compact TomThumb font (smaller than the default 6x8 font) plus
// faux-bold (double-draw offset by 1px) and automatic word-wrap, so lines
// that don't fit on one row split cleanly onto two instead of being cut off
// or overflowing the copy buffer.
void drawLyrics(unsigned long now) {
  const Song& song = songList[currentSongIndex];
  if (activeLyricIdx < 0) return;

  unsigned long since = now - lyricChangeMs;
  float progress = since >= LYRIC_SLIDE_MS ? 1.0f : (float)since / LYRIC_SLIDE_MS;
  // ease-out for a smoother, less mechanical slide
  float eased = 1.0f - (1.0f - progress) * (1.0f - progress);

  char buf[LYRIC_BUF_SIZE];

  display.setFont(&TomThumb);   // switch to the compact font for lyrics only

  // Outgoing line slides up and out
  if (lastLyricIdx >= 0 && progress < 1.0f) {
    strcpy_P(buf, (char*)pgm_read_ptr(&song.lyrics[lastLyricIdx].text));
    int16_t y = LYRIC_Y - (int16_t)(eased * LYRIC_SLIDE_DIST);
    drawWrappedLyric(buf, y, true);
  }

  // Incoming line slides up into place from below
  {
    strcpy_P(buf, (char*)pgm_read_ptr(&song.lyrics[activeLyricIdx].text));
    int16_t y = LYRIC_Y + (int16_t)((1.0f - eased) * LYRIC_SLIDE_DIST);
    drawWrappedLyric(buf, y, true);
  }

  display.setFont(NULL);        // restore the default font for everything else
}

// Draws `text` centered horizontally at baseline y. If bold is true, draws
// it twice (offset 1px right) to fake a heavier weight — the stock GFX
// fonts have no real bold/italic variant, so this is the practical stand-in.
void drawTextCenteredAtY(const char* text, int16_t y, bool bold) {
  if (y < 8 || y > 36) return;   // stay within the lyric viewport, avoid overlap
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  int16_t x = (SCREEN_WIDTH - (int16_t)w) / 2;
  if (x < 0) x = 0;
  display.setCursor(x, y);
  display.print(text);
  if (bold) {
    display.setCursor(x + 1, y);
    display.print(text);
  }
}

// Centers `text` at centerY; if it's too wide for the screen, splits it on
// the nearest word boundary to the middle and draws it as two stacked rows
// instead of letting it run off the edge.
void drawWrappedLyric(const char* text, int16_t centerY, bool bold) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, centerY, &x1, &y1, &w, &h);
  const uint16_t maxW = SCREEN_WIDTH - 6;

  if (w <= maxW) {
    drawTextCenteredAtY(text, centerY, bold);
    return;
  }

  int len = strlen(text);
  int mid = len / 2;
  int splitAt = -1;
  for (int i = mid; i > 0; i--) {
    if (text[i] == ' ') { splitAt = i; break; }
  }
  if (splitAt < 0) {
    for (int i = mid; i < len; i++) {
      if (text[i] == ' ') { splitAt = i; break; }
    }
  }
  if (splitAt < 0) {             // single long word, nothing to split on
    drawTextCenteredAtY(text, centerY, bold);
    return;
  }

  char line1[LYRIC_BUF_SIZE], line2[LYRIC_BUF_SIZE];
  int n1 = splitAt < (int)sizeof(line1) - 1 ? splitAt : (int)sizeof(line1) - 1;
  memcpy(line1, text, n1);
  line1[n1] = '\0';
  strncpy(line2, text + splitAt + 1, sizeof(line2) - 1);
  line2[sizeof(line2) - 1] = '\0';

  drawTextCenteredAtY(line1, centerY - 4, bold);
  drawTextCenteredAtY(line2, centerY + 5, bold);
}

// Kept for the loading/ending screens, which still use the default font.
void drawCenteredText(const char* text, int16_t y) {
  if (y < 10 || y > 34) return;   // stay within the lyric viewport, avoid overlap
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextSize(1);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int16_t x = (SCREEN_WIDTH - w) / 2;
  if (x < 0) x = 0;
  display.setCursor(x, y);
  display.print(text);
}

// ---- Character: the animated cat, always drawn near the bottom-center -----
void drawCharacter(unsigned long now) {
  int8_t bounceY = 0, shiftX = 0;
  const unsigned char* frame = getCharacterFrame(now, bounceY, shiftX);
  int16_t x = CHAR_BASE_X + shiftX;
  int16_t y = CHAR_BASE_Y + bounceY;
  display.drawBitmap(x, y, frame, CAT_W, CAT_H, SSD1306_WHITE);

  // Little "zzz" drifting above the cat while asleep
  if (currentMood == MOOD_SLEEP) {
    float t = now / 1000.0f;
    int16_t zx = x + 22 + (int16_t)(sin(t * 1.2f) * 2);
    int16_t zy = y - 6 - (int16_t)(fmod(t * 4.0f, 10.0f));
    if (zy > y - 14) {
      display.drawBitmap(zx, zy, icon_zzz, ICON_W, ICON_H, SSD1306_WHITE);
    }
  }
}

// ---- Effects: corner sparkles, bouncing note, floating hearts -------------
void drawEffects(unsigned long now) {
  // Sparkles (twinkling corners)
  for (uint8_t i = 0; i < NUM_SPARKLES; i++) {
    if (sparkles[i].on) {
      display.drawBitmap(sparkles[i].x, sparkles[i].y, icon_sparkle, SPARKLE_W, SPARKLE_H, SSD1306_WHITE);
    }
  }

  // Bouncing music note, top-right corner
  int16_t noteY = 2 + (int16_t)round((sin(noteBouncePhase * 4.0f) + 1.0f) * 2.0f);
  display.drawBitmap(SCREEN_WIDTH - 12, noteY, icon_note, ICON_W, ICON_H, SSD1306_WHITE);

  // Floating hearts
  for (uint8_t i = 0; i < NUM_HEARTS; i++) {
    if (hearts[i].active) {
      display.drawBitmap((int16_t)hearts[i].x, (int16_t)hearts[i].y, icon_heart, ICON_W, ICON_H, SSD1306_WHITE);
    }
  }
}

// ---- Progress bar: very bottom of the screen -------------------------------
void drawProgressBar(unsigned long now) {
  const Song& song = songList[currentSongIndex];
  unsigned long elapsed = now - songStartMs;
  if (elapsed > song.durationMs) elapsed = song.durationMs;

  const int16_t barX = 4, barY = 60, barW = SCREEN_WIDTH - 8, barH = 3;
  display.drawRoundRect(barX, barY, barW, barH, 1, SSD1306_WHITE);

  int16_t fillW = (int32_t)(barW - 2) * elapsed / song.durationMs;
  if (fillW > 0) {
    display.fillRoundRect(barX + 1, barY + 1, fillW, barH - 2, 0, SSD1306_WHITE);
  }
}

// ---- Loading screen: shown briefly before "playback" starts ---------------
void drawLoadingScreen(unsigned long now) {
  float t = (now - stateStartMs) / 1000.0f;

  display.setTextSize(1);
  drawCenteredText("loading...", 26);

  // Three dots that pulse in sequence
  const int16_t cx = SCREEN_WIDTH / 2;
  for (int8_t i = -1; i <= 1; i++) {
    float phase = t * 4.0f - i * 0.6f;
    float bounce = (sin(phase) + 1.0f) / 2.0f;   // 0..1
    int16_t dotY = 40 - (int16_t)(bounce * 4.0f);
    display.fillCircle(cx + i * 10, dotY, 2, SSD1306_WHITE);
  }

  // spinning ring of small ticks around a center point (subtle "vinyl" feel)
  float ang = t * 3.0f;
  for (uint8_t k = 0; k < 4; k++) {
    float a = ang + k * (PI / 2.0f);
    int16_t px = cx + (int16_t)(cos(a) * 18);
    int16_t py = 14 + (int16_t)(sin(a) * 8);
    display.drawPixel(px, py, SSD1306_WHITE);
    display.drawPixel(px + 1, py, SSD1306_WHITE);
  }
}

// ---- Ending screen: shown briefly after the track finishes ----------------
void drawEndingScreen(unsigned long now) {
  float t = (now - stateStartMs) / 1000.0f;
  drawCenteredText("thanks for listening", 14);

  // Fading/floating hearts row as an outro flourish (uses drawEffects hearts too)
  float bob = sin(t * 3.0f) * 2.0f;
  drawCenteredTextF("<3  <3  <3", 24 + (int16_t)bob);
}

void drawCenteredTextF(const char* text, int16_t y) {
  int16_t x1, y1; uint16_t w, h;
  display.setTextSize(1);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, y);
  display.print(text);
}

/*
  ===========================================================================
  HOW TO ADD MORE ANIMATIONS / EXPRESSIONS
  ---------------------------------------------------------------------------
  1. Draw a new 32x32 monochrome frame (any tool that exports a 1-bit bitmap
     works — even the Python generator used to build this sketch's frames,
     see gen_bitmaps.py-style logic: PIL ImageDraw on a 1-bit canvas, then
     pack rows into bytes MSB-first, padded to a byte boundary).
  2. Add it as `const unsigned char cat_myNewPose[] PROGMEM = { ... };`
     next to the other bitmaps.
  3. Add a new value to `enum CharMood` (before MOOD_COUNT).
  4. Add a `case MOOD_MYNEWPOSE:` branch inside getCharacterFrame() that
     returns cat_myNewPose (and sets whatever bounce/shift feels right).
  5. Add a branch/weight for it inside the mood-transition roll in
     updateCharacter() so it actually gets picked sometimes.
  For a *multi-frame* animation (like dance/walk), instead define an array
  `const unsigned char* const ANIM_MYTHING[] = { frameA, frameB };`, add a
  length + ms constant like the existing ones, and let it hook into the
  `frameMs`/`frameLen` block near the top of updateCharacter().

  ===========================================================================
  HOW TO ADD MORE SONGS
  ---------------------------------------------------------------------------
  1. Make a new `const LyricLine SONG2_LYRICS[] PROGMEM = { ... };` array,
     following the exact same pattern as SONG1_LYRICS above (timestamps in
     ms from t=0 of that song).
  2. Add `const uint8_t SONG2_LYRICS_LEN = sizeof(SONG2_LYRICS)/sizeof(LyricLine);`
  3. Add a new entry to `songList[]`:
       { "My Second Song", SONG2_LYRICS, SONG2_LYRICS_LEN, 45000UL }
     (the last field is the total track length in ms — controls the
     progress bar fill speed).
  4. To auto-advance through the list instead of looping the same track,
     change the STATE_ENDED branch in updatePlayerState() to:
       currentSongIndex = (currentSongIndex + 1) % SONG_COUNT;
     before resetting playerState to STATE_LOADING.
  5. (Optional) Hook up a real "next track" trigger — e.g. a button on an
     interrupt pin, or serial/BLE command — that sets currentSongIndex and
     forces playerState = STATE_LOADING; stateStartMs = millis();

  ===========================================================================
  MEMORY USAGE NOTES
  ---------------------------------------------------------------------------
  - Every bitmap and every LyricLine array lives in PROGMEM (flash), not
    RAM, via the PROGMEM attribute and pgm_read_*()/strcpy_P() accessors.
    An ESP32 has ~4MB of flash and ~320KB of RAM; this sketch's whole
    graphics set (16 x 32x32 cat frames @ 128 bytes each + a handful of
    8x8/6x6 icons) totals well under 3KB of flash — negligible.
  - RAM usage is dominated by the Adafruit_SSD1306 frame buffer itself
    (128*64/8 = 1024 bytes) plus a few small structs (particles, state
    machine variables) — a tiny fraction of the ESP32's available RAM.
  - Because nothing uses String or dynamic allocation, there's no heap
    fragmentation risk even if this sketch runs for days.

  ===========================================================================
  COMPILE NOTES
  ---------------------------------------------------------------------------
  - Board: any "ESP32 Dev Module" (or your specific dev-kit variant) under
    Tools > Board once the "esp32" boards package (Espressif Systems) is
    installed via Boards Manager.
  - Libraries needed (Library Manager): "Adafruit GFX Library",
    "Adafruit SSD1306". Wire.h ships with the ESP32 core.
  - If your OLED uses I2C address 0x3D instead of 0x3C, change
    SCREEN_ADDRESS above.
  ===========================================================================
*/
