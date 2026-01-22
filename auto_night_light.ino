/*
  Arduino Auto Night Light (Smooth Fade + LDR)
  - LED breathes when it is dark
  - LED is off when it is bright
  - Optional button toggles FORCE ON / AUTO

  Wiring:
    LED PWM: D9 -> 220Ω -> LED -> GND
    LDR divider: 5V -> LDR -> (A0 node) -> 10k -> GND
    Button (optional): D2 -> button -> GND (uses INPUT_PULLUP)
*/

const int LED_PIN = 9;        // Must be PWM-capable on most boards
const int LDR_PIN = A0;
const int BUTTON_PIN = 2;     // Optional

// Light threshold (0..1023). Lower typically means darker or brighter depending on divider placement.
// With wiring in README, lower reading usually means darker? It can vary by LDR.
// Use Serial output to confirm and adjust.
const int DARK_THRESHOLD = 450;

// Breathing fade configuration
const int FADE_MIN = 0;
const int FADE_MAX = 255;
const int FADE_STEP = 3;
const int FADE_DELAY_MS = 15;

// Button debounce
const unsigned long DEBOUNCE_MS = 40;

enum Mode {
  MODE_AUTO = 0,
  MODE_FORCE_ON = 1
};

Mode mode = MODE_AUTO;

int brightness = 0;
int direction = 1; // 1 = up, -1 = down

// Button state
bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;
unsigned long lastDebounceTime = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP); // optional button

  Serial.begin(9600);
  Serial.println("Auto Night Light started. Readings will print every ~500ms.");
}

bool isDark(int ldrValue) {
  // Depending on your LDR + resistor arrangement, you may need to invert this logic.
  // If covering the LDR makes the value go DOWN, then "dark" is ldrValue < DARK_THRESHOLD.
  // If covering the LDR makes the value go UP, then "dark" is ldrValue > DARK_THRESHOLD.
  //
  // With the wiring in README (5V -> LDR -> node -> 10k -> GND), many setups produce:
  // bright -> higher value, dark -> lower value
  return (ldrValue < DARK_THRESHOLD);
}

void updateButton() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
    if (reading != stableButtonState) {
      stableButtonState = reading;

      // Button pressed = LOW (because of INPUT_PULLUP)
      if (stableButtonState == LOW) {
        mode = (mode == MODE_AUTO) ? MODE_FORCE_ON : MODE_AUTO;
        Serial.print("Mode changed to: ");
        Serial.println(mode == MODE_AUTO ? "AUTO" : "FORCE ON");
      }
    }
  }

  lastButtonReading = reading;
}

void breatheStep() {
  brightness += direction * FADE_STEP;

  if (brightness >= FADE_MAX) {
    brightness = FADE_MAX;
    direction = -1;
  } else if (brightness <= FADE_MIN) {
    brightness = FADE_MIN;
    direction = 1;
  }

  analogWrite(LED_PIN, brightness);
  delay(FADE_DELAY_MS);
}

void loop() {
  updateButton();

  int ldrValue = analogRead(LDR_PIN);

  // Light logic
  bool darkNow = isDark(ldrValue);

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    lastPrint = millis();
    Serial.print("LDR: ");
    Serial.print(ldrValue);
    Serial.print(" | dark: ");
    Serial.print(darkNow ? "YES" : "NO");
    Serial.print(" | mode: ");
    Serial.println(mode == MODE_AUTO ? "AUTO" : "FORCE ON");
  }

  if (mode == MODE_FORCE_ON) {
    // Force on but still breathe (looks nicer than constant)
    breatheStep();
    return;
  }

  // AUTO mode
  if (darkNow) {
    breatheStep();
  } else {
    // Bright: ensure LED off and avoid flicker
    analogWrite(LED_PIN, 0);
    delay(50);
  }
}
