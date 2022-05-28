#include <Arduino.h>
#include <FunctionalInterrupt.h>

#define KEY_SW1 4
#define KEY_SW2 6
#define KEY_SW3 7

const bool serial_enabled = true;

class Button
{
public:
  Button(uint8_t reqPin) : PIN(reqPin){
    state = 1;
    pinMode(PIN, INPUT);
    attachInterrupt(PIN, std::bind(&Button::isr,this), CHANGE);
  };
  ~Button() {
    detachInterrupt(PIN);
  }

  void ARDUINO_ISR_ATTR isr() {
    pressed = true;
  }

  void checkPressed() {
    if (pressed) {
      delay(30);
      bool newState = digitalRead(PIN);
      if (newState != state) {
        if (serial_enabled)
          Serial.printf("Key on pin %u is %s\n", PIN, newState ? "up":"down");
        state = newState;
      }
      pressed = false;
    }
  }

private:
  const uint8_t PIN;
    volatile bool pressed;
    volatile bool state; // 0 for down(pressed), 1 for up
};

Button key1(KEY_SW1);
Button key2(KEY_SW2);
Button key3(KEY_SW3);


void setup() {
    if (serial_enabled)
      Serial.begin(115200);
}

void loop() {
  key1.checkPressed();
  key2.checkPressed();
  key3.checkPressed();
  delay(5);
}
