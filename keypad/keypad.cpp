#include "keypad.h"
#include "esphome/core/log.h"
#include "esphome/core/gpio.h"

namespace esphome {
namespace keypad {

static const char *TAG = "keypad";

void Keypad::setup() {
  for (auto *pin : this->rows_)
    if (!has_diodes_)
      pin->pin_mode(gpio::FLAG_INPUT);
    else
      pin->digital_write(true);
  for (auto *pin : this->columns_)
    pin->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
}

void Keypad::loop() {
  static unsigned long active_start = 0;
  static int active_key = -1;
  unsigned long now = millis();
  int key = -1;
  bool error = false;
  int pos = 0, row, col;
  for (auto *row : this->rows_) {
    if (!has_diodes_)
      row->pin_mode(gpio::FLAG_OUTPUT);
    row->digital_write(false);
    for (auto *col : this->columns_) {
      if (!col->digital_read()) {
        if (key != -1)
          error = true;
        else
          key = pos;
      }
      pos++;
    }
    row->digital_write(true);
    if (!has_diodes_)
      row->pin_mode(gpio::FLAG_INPUT);
  }
  if (error)
    return;

  if (key != active_key) {
    if ((active_key != -1) && (this->pressed_key_ == active_key)) {
      row = this->pressed_key_ / this->columns_.size();
      col = this->pressed_key_ % this->columns_.size();
      ESP_LOGD(TAG, "key @ row %d, col %d released", row, col);
      for (auto &listener : this->listeners_)
        listener->button_released(row, col);
      if (this->keys_.size()) {
        unsigned char keycode = this->keys_[this->pressed_key_];
        ESP_LOGD(TAG, "key '%c' released", keycode);
        for (auto &listener : this->listeners_)
          listener->key_released(keycode);
      }
      this->pressed_key_ = -1;
    }

    active_key = key;
    if (key != -1)
      active_start = now;
    return;
  }

  if ((this->pressed_key_ == key) || (now - active_start < this->debounce_time_))
    return;

  row = key / this->columns_.size();
  col = key % this->columns_.size();
  ESP_LOGD(TAG, "key @ row %d, col %d pressed", row, col);
  for (auto &listener : this->listeners_)
    listener->button_pressed(row, col);
  if (this->keys_.size()) {
    unsigned char keycode = this->keys_[key];
    ESP_LOGD(TAG, "key '%c' pressed", keycode);
    for (auto &listener : this->listeners_)
      listener->key_pressed(keycode);
  }
  this->pressed_key_ = key;
}

void Keypad::dump_config() {
  ESP_LOGCONFIG(TAG, "Keypad:");
  ESP_LOGCONFIG(TAG, " Rows:");
  for (auto &pin : this->rows_)
    LOG_PIN("  Pin: ", pin);
  ESP_LOGCONFIG(TAG, " Cols:");
  for (auto &pin : this->columns_)
    LOG_PIN("  Pin: ", pin);
}

void Keypad::register_listener(KeypadListener *listener) {
  this->listeners_.push_back(listener);
}

}  // namespace keypad
}  // namespace esphome

