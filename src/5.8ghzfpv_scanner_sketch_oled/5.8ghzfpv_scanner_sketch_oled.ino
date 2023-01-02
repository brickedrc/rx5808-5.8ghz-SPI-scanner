/*
This file is part of OLED 5.8ghz Scanner project.

    OLED 5.8ghz Scanner is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OLED 5.8ghz Scanner is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>.

    Copyright © 2016 Michele Martinelli
  */

#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <SFE_MicroOLED.h> // Include the SFE_MicroOLED library
#include "rx5808.h"
#include "const.h"

RX5808 rx5808(rssi_pin);

byte data0 = 0;
byte data1 = 0;
byte data2 = 0;
byte data3 = 0;

#define SCANNER_MODE 0
#define RECEIVER_MODE 1

uint8_t curr_status = SCANNER_MODE;
uint8_t curr_screen = 6;
uint8_t curr_channel = 0;
uint32_t curr_freq;

// battery monitor
float volt;
uint16_t VoltDivider = 10.5;

uint32_t last_irq;
uint8_t changing_freq, changing_mode;

// default values used for calibration
uint16_t rssi_min = 1024;
uint16_t rssi_max = 0;

#define PIN_RESET 9 // Optional - Connect RST on display to pin 9 on Arduino
MicroOLED oled(PIN_RESET);

void irq_select_handle()
{
  if (millis() - last_irq < 500) // debounce
    return;

  if (curr_status == SCANNER_MODE && digitalRead(button_select) == LOW)
  { // simply switch to the next screen
    curr_screen = (curr_screen + 1) % 7;
  }

  if (curr_status == RECEIVER_MODE)
  {
    if (digitalRead(button_select) == HIGH && millis() - last_irq > 800) // long press to reach the next strong cannel
      curr_channel = rx5808.getNext(curr_channel);
    else
      curr_channel = (curr_channel + 1) % CHANNEL_MAX;

    changing_freq = 1;
  }

  rx5808.abortScan();
  last_irq = millis();
}

void irq_mode_handle()
{
  if (millis() - last_irq < 500) // debounce
    return;

  if (digitalRead(button_mode) == LOW)
  {
    rx5808.abortScan();
    changing_mode = 1;

    if (curr_status == RECEIVER_MODE)
    {
      curr_status = SCANNER_MODE;
    }
    else
    {
      curr_status = RECEIVER_MODE;
      curr_channel = rx5808.getMaxPos(); // next channel is the strongest
    }
  }
  last_irq = millis();
}

void setup()
{
  Serial.begin(115200);

  delay(100);
  Wire.begin();
  oled.begin(0x3D, Wire);

  // button init
  pinMode(button_select, INPUT);
  digitalWrite(button_select, HIGH);
  pinMode(button_mode, INPUT);
  digitalWrite(button_mode, HIGH);

  // display init
  oled.clear(ALL);  // Clear the display's internal memory
  oled.display();   // Display what's in the buffer (splashscreen)
  delay(1000);      // Delay 1000 ms
  oled.clear(PAGE); // Clear the buffer.

  // initialize SPI:
  oled.print("Starting RX");
  oled.display();
  oled.clear(PAGE); // Clear the buffer.
  pinMode(SSP, OUTPUT);
  SPI.begin();
  SPI.setBitOrder(LSBFIRST);
  rx5808.init();

  // power on calibration if button pressed
  while (digitalRead(button_select) == LOW || digitalRead(button_mode) == LOW)
  {
    oled.print("Calibrating...");
    oled.display();
    oled.clear(PAGE); // Clear the buffer.
    rx5808.calibration();
  }

  // receiver init
  curr_channel = rx5808.getMaxPos();
  changing_freq = 1;
  changing_mode = 0;

  // rock&roll
  attachInterrupt(digitalPinToInterrupt(button_select), irq_select_handle, CHANGE);
  attachInterrupt(digitalPinToInterrupt(button_mode), irq_mode_handle, CHANGE);
}

void loop(void)
{
  // battery_measure();

  if (curr_status == SCANNER_MODE)
  {
    rx5808.scan(1, BIN_H);
  }

  if (changing_mode)
  { // if changing mode "please wait..."
    wait_draw();
  }
  else
  {
    if (curr_status == RECEIVER_MODE)
    {
      receiver_draw(curr_channel);
    }
    if (curr_status == SCANNER_MODE)
    {
      scanner_draw(curr_screen);
    }
  }

  if (curr_status == RECEIVER_MODE && changing_freq || changing_mode)
  {
    changing_freq = changing_mode = 0;
    rx5808.scan(1, BIN_H);
    curr_freq = pgm_read_word_near(channelFreqTable + curr_channel);
    rx5808.setFreq(curr_freq);
  }
}
