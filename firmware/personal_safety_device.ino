/*
  Personal Safety Device Prototype

  Hardware:
  - ESP8266
  - L293D Driver
  - Cree LED
  - Piezo Buzzer
  - Li-Po Battery
  - MT3608 Boost Converter

  Features:
  - Arm/disarm system
  - Long-press activation
  - High-intensity LED strobe
  - Ultrasonic buzzer output
  - Automatic disarming after activation

  Author: Izabelle Kirongozi
*/

// -------------------- PIN ASSIGNMENTS --------------------

#define BTN         D3    // GPIO0  - Push button (INPUT_PULLUP)
#define LED_1A      D1    // GPIO5  - L293D Pin2 (1A)
#define LED_2A      D6    // GPIO12 - L293D Pin7 (2A)
#define LED_EN      D2    // GPIO4  - L293D Pin1 (PWM Enable)

#define BUZZ_EN     D4    // GPIO2  - L293D Pin9 (Enable)
#define BUZZ_SIG    D5    // GPIO14 - L293D Pin10 (Signal)

// -------------------- CONFIGURATION --------------------

const int PWM_MAX         = 255;
const int PWM_BLINK       = 120;

const int BURST_DURATION  = 2000;   // ms
const int STROBE_ON_MS    = 25;     // ms
const int STROBE_OFF_MS   = 5;      // ms

const int LONG_PRESS_MS   = 1500;
const int SHORT_PRESS_MS  = 500;

// -------------------- SYSTEM STATE --------------------

bool armed = false;
bool buttonPressed = false;

unsigned long pressStartTime = 0;

// -------------------- LED CONTROL --------------------

inline void ledOn() {
  analogWrite(LED_EN, PWM_MAX);
}

inline void ledOff() {
  analogWrite(LED_EN, 0);
}

inline void setLedBrightness(int pwmValue) {
  analogWrite(LED_EN, constrain(pwmValue, 0, 255));
}

// -------------------- SETUP --------------------

void setup() {

  Serial.begin(115200);
  delay(100);

  pinMode(BTN, INPUT_PULLUP);

  pinMode(LED_1A, OUTPUT);
  pinMode(LED_2A, OUTPUT);
  pinMode(LED_EN, OUTPUT);

  pinMode(BUZZ_EN, OUTPUT);
  pinMode(BUZZ_SIG, OUTPUT);

  // Configure LED channel

  digitalWrite(LED_1A, HIGH);
  digitalWrite(LED_2A, LOW);

  ledOff();

  // Configure buzzer channel

  digitalWrite(BUZZ_EN, HIGH);
  digitalWrite(BUZZ_SIG, LOW);

  Serial.println();
  Serial.println("Personal Safety Device");
  Serial.println("System Ready");
  Serial.println("Short press: Arm / Disarm");
  Serial.println("Long press: Activate");
}

// -------------------- ULTRASONIC BURST --------------------

void ultrasonicBurst(int durationMs) {

  unsigned long startTime = millis();
  unsigned int cycleCount = 0;

  while (millis() - startTime < (unsigned long)durationMs) {

    digitalWrite(BUZZ_SIG, HIGH);
    delayMicroseconds(12);

    digitalWrite(BUZZ_SIG, LOW);
    delayMicroseconds(13);

    if ((++cycleCount & 0x3FF) == 0) {
      yield();
    }
  }

  digitalWrite(BUZZ_SIG, LOW);
}

// -------------------- ACTIVATION SEQUENCE --------------------

void triggerSequence() {

  Serial.println("Activation sequence started");

  // Fast fade in

  for (int pwm = 0; pwm <= PWM_MAX; pwm += 17) {
    setLedBrightness(pwm);
    delay(5);
  }

  ledOn();

  unsigned long startTime = millis();
  int flashCount = 0;

  while (millis() - startTime < (unsigned long)BURST_DURATION) {

    ledOn();

    ultrasonicBurst(STROBE_ON_MS);

    ledOff();

    delay(STROBE_OFF_MS);

    flashCount++;

    yield();
  }

  // Fade out

  for (int pwm = PWM_MAX; pwm >= 0; pwm -= 17) {
    setLedBrightness(pwm);
    delay(4);
  }

  ledOff();

  digitalWrite(BUZZ_SIG, LOW);

  Serial.print("Sequence complete. Flashes emitted: ");
  Serial.println(flashCount);

  Serial.println("Device disarmed");

  armed = false;
}

// -------------------- STATUS INDICATOR --------------------

void blinkLed(int times, int pwmValue, int onMs, int offMs) {

  for (int i = 0; i < times; i++) {

    setLedBrightness(pwmValue);

    delay(onMs);

    ledOff();

    if (i < times - 1) {
      delay(offMs);
    }
  }

  delay(offMs);
}

// -------------------- MAIN LOOP --------------------

void loop() {

  bool buttonState = digitalRead(BTN);

  // Button pressed

  if (buttonState == LOW && !buttonPressed) {

    pressStartTime = millis();

    buttonPressed = true;
  }

  // Button released

  if (buttonState == HIGH && buttonPressed) {

    unsigned long pressDuration = millis() - pressStartTime;

    buttonPressed = false;

    Serial.print("Press duration: ");
    Serial.print(pressDuration);
    Serial.println(" ms");

    // Short press: arm/disarm

    if (pressDuration < (unsigned long)SHORT_PRESS_MS) {

      armed = !armed;

      if (armed) {

        Serial.println("System armed");

        blinkLed(3, PWM_BLINK, 200, 150);

      } else {

        Serial.println("System disarmed");

        blinkLed(1, PWM_BLINK, 400, 0);
      }

    }
    // Ignore medium press

    else if (pressDuration < (unsigned long)LONG_PRESS_MS) {

      Serial.println("Input ignored");

    }
    // Long press: activate

    else {

      if (armed) {

        triggerSequence();

      } else {

        Serial.println("Cannot activate. System not armed.");

        blinkLed(5, 60, 70, 70);
      }
    }
  }

  delay(10);
}
