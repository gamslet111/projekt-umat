#pragma once

#include <Arduino.h>

// ESP32-C3-MINI-1 custom PCB pin map.
//
// GPIO9 is the BOOT strap button and GPIO20/GPIO21 are the USB-UART0 pins
// used by the CP210x for flashing/serial monitor, so they are intentionally
// not used by the application.
namespace BoardPins
{
constexpr uint8_t I2C_SDA = 1;
constexpr uint8_t I2C_SCL = 0;
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;
constexpr uint8_t PCF8574_I2C_ADDRESS = 0x20;

constexpr uint8_t DFPLAYER_RX = 18; // ESP32-C3 RX pin, connect to DFPlayer TX
constexpr uint8_t DFPLAYER_TX = 19; // ESP32-C3 TX pin, connect to DFPlayer RX

constexpr uint8_t ROTATE_MOTOR_1 = 4; // DRV8833 A_IN1, rotation header J4
constexpr uint8_t ROTATE_MOTOR_2 = 5; // DRV8833 A_IN2, rotation header J4
constexpr uint8_t CARD_MOTOR_1 = 7;   // DRV8833 B_IN1, card header J5
constexpr uint8_t CARD_MOTOR_2 = 6;   // DRV8833 B_IN2, card header J5
}
