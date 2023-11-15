#include "SevSegShift.h"

#define SHIFT_PIN_DS 8   /* Data input PIN */
#define SHIFT_PIN_STCP 7 /* Shift Register Storage PIN */
#define SHIFT_PIN_SHCP 6 /* Shift Register Shift PIN */

#define BUTTON_LEFT_PIN 14
#define BUTTON_RIGHT_PIN 15

int timeOffset = 0;

struct time
{
  unsigned long long milli;
  unsigned long long seconds;
  unsigned int minutes;
  unsigned int hours;

  void getTime()
  {
    unsigned long long currentTime = millis();
    long long adjustedMillis = currentTime % 86400000 + timeOffset * 60000;

    // Ensure that adjustedMillis is positive
    if (adjustedMillis < 0)
    {
      adjustedMillis += 86400000;
    }

    milli = adjustedMillis;
    seconds = milli / 1000;
    minutes = seconds / 60;
    hours = minutes / 60;
  }
};

time clock;

unsigned long buttonPressStartTime = 0;
const unsigned long holdThreshold = 100;        // Time in milliseconds to consider a button press as "held"
const unsigned long initialIncrementDelay = 50; // Initial delay between increments in milliseconds

enum ButtonState
{
  IDLE,
  PRESSED,
  HELD
};

ButtonState leftButtonState = IDLE;
ButtonState rightButtonState = IDLE;

// Instantiate a seven segment controller object (with Shift Register functionality)
SevSegShift sevseg(
    SHIFT_PIN_DS,
    SHIFT_PIN_SHCP,
    SHIFT_PIN_STCP,
    1,   /* number of shift registers there is only 1 Shiftregister
            used for all Segments (digits are on Controller)
            default value = 2 (see SevSegShift example)
            */
    true /* Digits are connected to Arduino directly
            default value = false (see SevSegShift example)
          */
);

void setup()
{
  byte numDigits = 4;
  byte digitPins[] = {5, 4, 3, 2};               // These are the PINS of the ** Arduino **
  byte segmentPins[] = {0, 1, 2, 3, 4, 5, 6, 7}; // these are the PINs of the ** Shift register **
  bool resistorsOnSegments = false;              // 'false' means resistors are on digit pins
  byte hardwareConfig = COMMON_ANODE;            // See README.md for options
  bool updateWithDelays = false;                 // Default 'false' is Recommended
  bool leadingZeros = true;                      // Use 'true' if you'd like to keep the leading zeros
  bool disableDecPoint = false;                  // Use 'true' if your decimal point doesn't exist or isn't connected. Then, you only need to specify 7 segmentPins[]

  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments,
               updateWithDelays, leadingZeros, disableDecPoint);
  sevseg.setBrightness(100);

  Serial.begin(9600);
}

void checkButton()
{
  int leftButtonStateNow = digitalRead(BUTTON_LEFT_PIN);
  int rightButtonStateNow = digitalRead(BUTTON_RIGHT_PIN);

  handleButton(BUTTON_LEFT_PIN, leftButtonStateNow, leftButtonState);
  handleButton(BUTTON_RIGHT_PIN, rightButtonStateNow, rightButtonState);
}

void handleButton(int buttonPin, int buttonStateNow, ButtonState &buttonState)
{
  if (buttonStateNow == HIGH)
  {
    if (buttonState == IDLE)
    {
      // Button pressed for the first time
      buttonState = PRESSED;
      buttonPressStartTime = millis();
      timeOffset += (buttonPin == BUTTON_LEFT_PIN) ? -1 : 1; // Adjust timeOffset
    }
    else if (buttonState == PRESSED)
    {
      // Button held down
      unsigned long elapsedTime = millis() - buttonPressStartTime;
      if (elapsedTime >= holdThreshold)
      {
        buttonState = HELD;
        buttonPressStartTime = millis();
      }
    }
    else if (buttonState == HELD)
    {
      // Increment timeOffset and adjust delay
      unsigned long elapsedTime = millis() - buttonPressStartTime;
      if (elapsedTime >= initialIncrementDelay)
      {
        timeOffset += (buttonPin == BUTTON_LEFT_PIN) ? -1 : 1;
        buttonPressStartTime = millis();
      }
    }
  }
  else
  {
    // Button released or not pressed
    if (buttonState == PRESSED)
    {
      // Reset button press start time when button is released after a short press
      buttonState = IDLE;
      buttonPressStartTime = 0;
    }
    else if (buttonState == HELD)
    {
      // Reset button state when button is released after being held
      buttonState = IDLE;
      buttonPressStartTime = 0;
      // Additional actions can be added here for the end of held state
    }
  }
}

void loop()
{
  checkButton();

  clock.getTime();

  int displayTime = ((clock.hours % 24) << 8) | (clock.minutes % 60);
  Serial.println(displayTime);
  sevseg.setNumber(displayTime, -1, true);
  sevseg.refreshDisplay();
}