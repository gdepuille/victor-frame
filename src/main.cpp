#include <Arduino.h>
#include <FastLED.h>
#include <LEDMatrix.h>
#include <LEDText.h>
#include <FontRobotron.h>
#include <OneButton.h>

#include "bitmaps/digdug.h"
#include "bitmaps/frogger.h"
#include "bitmaps/galaga.h"
#include "bitmaps/mario.h"
#include "bitmaps/pacman.h"
#include "bitmaps/qbert.h"

// Led screen definition
#define COLS 16
#define ROWS 16
#define NUM_LEDS (COLS * ROWS) // Matrix 16x16

#define FRAMES_PER_SECOND 120
#define TIME_TO_SLEEP_MINUTE 60 // 1 Heure

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define MATRIX_WIDTH   COLS
#define MATRIX_HEIGHT  ROWS
#define MATRIX_TYPE    HORIZONTAL_ZIGZAG_MATRIX

// I/O pins
#define DATA_PIN 6

// Input pin for the button
// Click           : Change manually the pattern / animation
// Double Click    : Change patterns family
// Triple Click    : Enable / Disable automatic change
// Quadruple Click : On / Off
// Long press      : Change the brightness
#define BTN 2

void addGlitter(fract8 chanceOfGlitter);
void drawBitmap(const long *bitmap);
void allLedsColor(CRGB color);
void showText();

void rainbow();
void rainbowWithGlitter();
void onlyGlitter();
void confetti();
void sinelon();
void juggle();
void bpm();

void toggleLights();
void nextPattern();
void multiClick();
void nextPatternFamily();
void toggleAuto();
void startChangeBrightness();
void stopChangeBrightness();
void modifyBrightness();

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
const SimplePatternList animationsPatterns = {rainbow, onlyGlitter, confetti, sinelon, juggle, rainbowWithGlitter, bpm};
const long *bitmapsPatterns[] = {
  SMBSpiny, SMBGoomba, SMBMario,
  Frogger, PacManPac, PacManBlinkyFull,
  PacManMsPac, Qbert
};
const uint8_t bitmapsPatternsSize[] = {
  SMBSpinySize, SMBGoombaSize, SMBMarioSize,
  FroggerSize, PacManPacSize, PacManBlinkyFullSize,
  PacManMsPacSize, QbertSize
};

uint8_t currentFamilyNumber = 1; // Index of the current pattern family
uint8_t currentPatternNumber = 0; // Index number of which pattern is current inside the current family of patterns
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
uint8_t bitmapAnimationNumber = 0; // Index of the current bitmap animation

bool modeChanged = false;
bool automaticChange = true;
bool changeBrightness = false;
bool incBrightness = true;
bool enabled = true;
uint8_t brightness = 10;
uint8_t elapsedRuntime = 0;

// Define the array of leds
CRGB leds[NUM_LEDS];
OneButton button;
cLEDMatrix<MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> ledMatrix;
cLEDText ledText;

const unsigned char txtIntro[] = {
  EFFECT_SCROLL_LEFT EFFECT_FRAME_RATE "\x04" "   VICTOR'S FRAME "
};

const unsigned char txtAuto[] = {
  EFFECT_SCROLL_LEFT EFFECT_RGB "\x00\xff\x00" "   AUTOMATIC " EFFECT_RGB "\xff\xff\xff"
};

const unsigned char txtManual[] = {
  EFFECT_SCROLL_LEFT EFFECT_RGB "\xff\x00\x00" "   MANUAL " EFFECT_RGB "\xff\xff\xff"
};

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
  delay(2000);
  Serial.println("Setup start :");
#endif

#ifdef DEBUG
  Serial.println(" * Input configuration ...");
#endif
  button = OneButton(BTN, true, true);
  button.attachClick(nextPattern);
  button.attachDoubleClick(nextPatternFamily);
  button.attachMultiClick(multiClick);
  button.attachLongPressStart(startChangeBrightness);
  button.attachLongPressStop(stopChangeBrightness);

#ifdef DEBUG
  Serial.println(" * Output configuration ...");
#endif
  pinMode(LED_BUILTIN, OUTPUT);

#ifdef DEBUG
  Serial.println(" * Disable LED_BUILTIN ...");
#endif
  digitalWrite(LED_BUILTIN, LOW);

#ifdef DEBUG
  Serial.println(" * Configure FastLED ...");
#endif
  CFastLED::addLeds<NEOPIXEL, DATA_PIN>(leds, ledMatrix.Size());
  FastLED.setBrightness(brightness);

  ledMatrix.SetLEDArray(leds);

  ledText.SetFont(RobotronFontData);
  ledText.Init(&ledMatrix, ledMatrix.Width(), ledText.FontHeight() + 1, 0, 0);
  ledText.SetText((unsigned char *) txtIntro, sizeof(txtIntro) - 1);

  showText();
}

void loop() {
  button.tick();

  unsigned long delay = 1000 / FRAMES_PER_SECOND;
  if (enabled && modeChanged) {
    if (automaticChange) {
      ledText.SetText((unsigned char *) txtAuto, sizeof(txtAuto) - 1);
    } else {
      ledText.SetText((unsigned char *) txtManual, sizeof(txtAuto) - 1);
    }
    modeChanged = false;
    showText();

  } else if (enabled && !modeChanged) {
    // Call the current pattern function once, updating the 'leds' array
    if (currentFamilyNumber == 0) {
      animationsPatterns[currentPatternNumber]();
    } else {
      drawBitmap(bitmapsPatterns[currentPatternNumber][bitmapAnimationNumber]);
    }

  } else if (!enabled) {
    fadeToBlackBy(leds, NUM_LEDS, 20);
    elapsedRuntime = 0;
  }

  FastLED.show(); // send the 'leds' array out to the actual LED strip
  FastLED.delay(delay); // insert a delay to keep the framerate modest

  // do some periodic updates
  EVERY_N_MILLISECONDS(20) {
    // slowly cycle the "base color" through the rainbow
    gHue++;
  }

  EVERY_N_MILLISECONDS(100) {
    // change the brightness
    if (changeBrightness) {
      modifyBrightness();
    }
  }

  EVERY_N_MILLIS(200) {
    if (currentFamilyNumber == 1) {
      bitmapAnimationNumber++;
      if (bitmapAnimationNumber >= bitmapsPatternsSize[currentPatternNumber]) {
        bitmapAnimationNumber = 0;
      }
    } else {
      bitmapAnimationNumber = 0;
    }
  }

  EVERY_N_SECONDS(currentFamilyNumber == 0 ? 30 : 10) {
    // change patterns periodically if in automatic mode
    if (enabled && automaticChange) {
      nextPattern();
    }
  }

  EVERY_N_MINUTES(10) {
    // change patterns periodically if in automatic mode
    if (enabled && automaticChange) {
      nextPatternFamily();
    }
  }

  EVERY_N_MINUTES(1) {
    // increment time
    if (enabled) {
      elapsedRuntime++;
    }

#ifdef DEBUG
    Serial.println("Auto stop : ");
    Serial.print(" -> Elapsed (min)   : ");Serial.println(elapsedRuntime);
    Serial.print(" -> Stop time (min) : ");Serial.println(TIME_TO_SLEEP_MINUTE);
#endif

    if (elapsedRuntime >= TIME_TO_SLEEP_MINUTE) {
      enabled = false;
    }
  }
}

// ==================================================== //

void toggleLights() {
#ifdef DEBUG
  Serial.println("Toggle lights");
#endif

  enabled = !enabled;
}

void nextPattern() {
  if (!enabled) {
    enabled = true;
    return;
  }

#ifdef DEBUG
  Serial.println("Click -> Pattern change");
#endif

  // add one to the current pattern number, and wrap around at the end
  if (currentFamilyNumber == 0) {
    currentPatternNumber = (currentPatternNumber + 1) % ARRAY_SIZE(animationsPatterns);
  } else {
    bitmapAnimationNumber = 0;
    currentPatternNumber = (currentPatternNumber + 1) % ARRAY_SIZE(bitmapsPatterns);
  }

#ifdef DEBUG
  Serial.print(" * Family number   : ");
  Serial.println(currentFamilyNumber);
  Serial.print(" * Pattern courant : ");
  Serial.println(currentPatternNumber);
#endif
}

void toggleAuto() {
  if (!enabled) {
    enabled = true;
    return;
  }

#ifdef DEBUG
  Serial.println("Doubleclick -> Pattern automatic change");
#endif
  automaticChange = !automaticChange;
  modeChanged = true;

#ifdef DEBUG
  Serial.print(" * Enchainement en mode auto : ");
  Serial.println(automaticChange);
#endif
}

void multiClick() {
  if (!enabled) {
    enabled = true;
    return;
  }

  int nbClick = button.getNumberClicks();
#ifdef DEBUG
  Serial.print("Pattern multiclick = ");
  Serial.println(nbClick);
#endif

  if (nbClick == 3) {
    toggleAuto();
  } else if (nbClick == 4) {
    toggleLights();
  } else {
#ifdef DEBUG
    Serial.println(" * Inconnu !!");
#endif
  }
}

void nextPatternFamily() {
#ifdef DEBUG
  Serial.println("Next pattern family");
#endif
  currentFamilyNumber = (currentFamilyNumber + 1) % 2;
  currentPatternNumber = 0;

#ifdef DEBUG
  Serial.print(" * Family number   : ");
  Serial.println(currentFamilyNumber);
  Serial.print(" * Pattern courant : ");
  Serial.println(currentPatternNumber);
#endif
}

void startChangeBrightness() {
  if (!enabled) {
    return;
  }

#ifdef DEBUG
  Serial.println("Longpress start -> Brightness change");
#endif

  changeBrightness = true;
}

void stopChangeBrightness() {
  if (!enabled) {
    return;
  }
#ifdef DEBUG
  Serial.println("Longpress stop -> Brightness change");
#endif

  changeBrightness = false;
  incBrightness = !incBrightness;
}

void modifyBrightness() {
  if (!enabled) {
    return;
  }

  if (incBrightness && brightness <= 250) {
    brightness += 5;
  } else if (!incBrightness && brightness >= 10) {
    brightness -= 5;
  }

  FastLED.setBrightness(brightness);

#ifdef DEBUG
  Serial.print(" * Brightness : ");
  Serial.println(brightness);
#endif
}

// ==================================================== //

void addGlitter(fract8 chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += CRGB::White;
  }
}

void drawBitmap(const long *bitmap) {
  FastLED.clear();
  bool LTR = true;
  for (int i = 0; i < NUM_LEDS; i++) {
    if (LTR) {
      for (int j = 0; j < COLS; j++) {
        leds[i + j] = pgm_read_dword(&bitmap[i + j]);
      }
    } else {
      for (int j = COLS - 1; j > 0; j--) {
        leds[i + COLS - j] = pgm_read_dword(&bitmap[i + j - 1]);
      }
    }

    i = i + COLS - 1;
    LTR = !LTR;
  }
}

void allLedsColor(CRGB color) {
  for (auto & led : leds) {
    led = color;
  }
}

void showText() {
  while(ledText.UpdateText() != -1) {
    // Symetrise the text on the matrix
    CRGB first;
    CRGB last;
    for (int i = 0; i < MATRIX_WIDTH / 2; i++) {
      for (int j = 0; j < MATRIX_HEIGHT; j++) {
        first = ledMatrix(i, j);
        last = ledMatrix(MATRIX_WIDTH - i - 1, MATRIX_HEIGHT - j - 1);
        ledMatrix(i, j) = last;
        ledMatrix(MATRIX_WIDTH - i - 1, MATRIX_HEIGHT - j - 1) = first;
      }
    }

    FastLED.show();
    FastLED.delay(10);
    FastLED.clear();
  }
}

// ============= SIMPLE PATTERNS ============= //

void rainbow() {
  // FastLED's built-in rainbow generator
  fill_rainbow(leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void onlyGlitter() {
  // glitter, with fading, only
  fadeToBlackBy(leds, NUM_LEDS, 20);
  addGlitter(80);
}

void confetti() {
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV(gHue + random8(64), 200, 255);
}

void sinelon() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  leds[pos] += CHSV(gHue, 255, 192);
}

void bpm() {
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
  for (int i = 0; i < NUM_LEDS; i++) {
    //9948
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy(leds, NUM_LEDS, 20);
  byte dothue = 0;
  for (int i = 0; i < 8; i++) {
    leds[beatsin16(i + 7, 0, NUM_LEDS - 1)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}
