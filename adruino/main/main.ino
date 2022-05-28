#include <Arduino.h>
#include <FunctionalInterrupt.h>
#include <BleKeyboard.h>
#include "esp_adc_cal.h"
#include "soc/adc_channel.h"

#define KEY_SW1 4
#define KEY_SW2 6
#define KEY_SW3 7

const bool serial_enabled = true;
const unsigned long bat_update_interval = 30000;

BleKeyboard bleKeyboard("3key-ble-v2", "warrenwjk", 100);
float battery_percentage(float v);
void battery_adc_init();
float get_battery_percentage();

class Button
{
public:
  Button(uint8_t reqPin, char c) : PIN(reqPin), to_send(c){
    state = true; // up
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
        if (newState == false && bleKeyboard.isConnected())
        {
          bleKeyboard.write(to_send);
        }
        state = newState;
      }
      pressed = false;
    }
  }

private:
  const uint8_t PIN;
    volatile bool pressed;
    volatile bool state; // 0 for down(pressed), 1 for up
    const char to_send;
};

Button key1(KEY_SW1, '2'); // chage key here
Button key2(KEY_SW2, '1');
Button key3(KEY_SW3, KEY_RETURN);

void setup() {
  if (serial_enabled) {
    Serial.begin(115200);
    #if defined(USE_NIMBLE)
      Serial.println("NimBLE enabled.");
    #endif
    Serial.println("BLE started.");
  }
  battery_adc_init();
  bleKeyboard.setName("github:am009/3key-ble-v2");
  bleKeyboard.setBatteryLevel(roundf(get_battery_percentage()));
  bleKeyboard.begin();
}

unsigned long prev_measure_bat = 0;

void loop() {
  unsigned long current_time = millis();
  if (current_time - prev_measure_bat > bat_update_interval) {
    bleKeyboard.setBatteryLevel(roundf(get_battery_percentage()));
    prev_measure_bat = current_time;
  }
  key1.checkPressed();
  key2.checkPressed();
  key3.checkPressed();
  delay(5);
}

// see https://github.com/am009/my-3key-ble-keyboard/blob/main/li-ion-vol2percentage.ggb
// https://github.com/am009/my-3key-ble-keyboard/blob/main/note.md#%E7%94%B5%E6%B1%A0%E7%AE%A1%E7%90%86
float battery_percentage(float v)
{
  float square1;
  if (v >= 4.2f)
  {
    return 100.0f;
  }
  else if (v > 3.79f)
  {
    square1 = (v - 4.25f);
    return (square1 * square1 * -28.05f + 10.0f) * 10.0f;
  }
  else if (v > 3.68f)
  {
    square1 = (v - 3.673f);
    return (square1 * square1 * 217.75f + 1.0f) * 10.0f;
  }
  else if (v > 3.0f)
  {
    square1 = (v - 2.87f);
    return (square1 * square1 * 1.563f - 0.03f) * 10.0f;
  }
  else
  {
    return 0.0f;
  }
}

// see https://github.com/espressif/esp-idf/blob/master/examples/peripherals/adc/single_read/single_read/main/single_read.c
esp_adc_cal_characteristics_t adc_chars;

void battery_adc_init()
{
  esp_err_t ret;
  bool cali_enable = false;
  esp_adc_cal_value_t val_type;
  ret = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP);
  if (ret == ESP_ERR_NOT_SUPPORTED) {
      Serial.println("Calibration scheme not supported, skip software calibration");
  } else if (ret == ESP_ERR_INVALID_VERSION) {
      Serial.println("eFuse not burnt, skip software calibration");
  } else if (ret == ESP_OK) {
      cali_enable = true;
  } else {
      Serial.println("Invalid arg");
  }

  val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc_chars);
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
  {
    Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);// Two Point --> coeff_a:47304mV coeff_b:0mV
  }
  else
  {
    Serial.println("Unknown Calibration");
  }

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_11); // ADC1_CHANNEL_2 => GPIO2
}

float get_battery_percentage() {
  int raw = adc1_get_raw(ADC1_CHANNEL_2); // GPIO2
  uint32_t volt = esp_adc_cal_raw_to_voltage(raw, &adc_chars); // mVolt
  float vbat = volt * 2.0f / 1000.0f;
  float batp = battery_percentage(vbat);
  if (serial_enabled) {
    Serial.printf("Volt: %d mV, %f%%.\n", volt * 2, batp);
  }
  return batp;
}
