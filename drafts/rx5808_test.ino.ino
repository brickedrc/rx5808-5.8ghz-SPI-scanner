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
    Modifications © 2023 Nick Brogna
  */

#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <SFE_MicroOLED.h> // Include the SFE_MicroOLED library
#include "U8glib.h"
#include "rx5808.h"
#include "const.h"

// devices with all constructor calls is here: http://code.google.com/p/u8glib/wiki/device
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_DEV_0 | U8G_I2C_OPT_NO_ACK | U8G_I2C_OPT_FAST); // Fast I2C / TWI

RX5808 rx5808(rssi_pin);

MicroOLED oled();

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

MicroOLED oled;

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
  oled.print("Hello");
  oled.display();
  oled.clear(PAGE); // Clear the buffer.

  // initialize SPI:
  Serial.println("Initializing SPI...");
  pinMode(SSP, OUTPUT);
  SPI.begin();
  SPI.setBitOrder(LSBFIRST);
  Serial.println("Initializing RX5808...");
  rx5808.init();

  // power on calibration if button pressed
  while (digitalRead(button_select) == LOW || digitalRead(button_mode) == LOW)
  {
    Serial.println("Calibrating RX...");
    rx5808.calibration();
  }

  // receiver init
  curr_channel = rx5808.getMaxPos();
  changing_freq = 1;
  changing_mode = 0;
  Serial.print("Current Channel = ");
  Serial.println(curr_channel);

  // rock&roll
  attachInterrupt(digitalPinToInterrupt(button_select), irq_select_handle, CHANGE);
  attachInterrupt(digitalPinToInterrupt(button_mode), irq_mode_handle, CHANGE);
}

void loop(void)
{
  oled.clear(PAGE); // Clear the screen
  // battery_measure();

  if (curr_status == SCANNER_MODE)
  {
    Serial.println("Scanning..."); // DEBUG
    rx5808.scan(1, BIN_H);

    // display Scanning mode
    oled.setCursor(0, 0);
    oled.print("Scanning...");
  }

  /*
  //GRAPHICS
  u8g.firstPage();
  do {
    if (changing_mode) { //if changing mode "please wait..."
      wait_draw();
    } else {
      if (curr_status == RECEIVER_MODE) {
        receiver_draw(curr_channel);
      }
      if (curr_status == SCANNER_MODE) {
        scanner_draw(curr_screen);
      }
    }
  } while ( u8g.nextPage() );
  */

  if (curr_status == RECEIVER_MODE && changing_freq || changing_mode)
  {
    changing_freq = changing_mode = 0;
    curr_freq = pgm_read_word_near(channelFreqTable + curr_channel);

    Serial.println("Switching to RX Mode...");
    Serial.print("Channel ");
    Serial.print(curr_channel);
    Serial.print(" - Changing frequency to ");
    Serial.print(curr_freq);
    Serial.println(" MHz");

    rx5808.scan(1, BIN_H);
    rx5808.setFreq(curr_freq);

    uint8_t channelIndex = pgm_read_byte_near(channelList); // retrive the value based on the freq order
    // Serial.print(curr_freq);
    // Serial.print(" MHz - RSSI: ");
    // Serial.println(rx5808.getRssi(channelIndex));
  }
  else if (curr_status == RECEIVER_MODE)
  {
    rx5808.scan(1, BIN_H);
    uint8_t channelIndex = pgm_read_byte_near(channelList); // retrive the value based on the freq order
    // Serial.print(curr_freq);
    // Serial.print(" MHz - RSSI: ");
    // Serial.println(rx5808.getRssi(channelIndex));

    oled.setCursor(0, 0);
    oled.print("[RX MODE]");
    oled.setCursor(0, 10);
    oled.print(curr_freq);
    oled.print(" MHz");
    oled.setCursor(0, 20);
    oled.print("RSSI: ");
    oled.print(rx5808.getRssi(channelIndex));
  }

  // delay(500);
  oled.display(); // draw display
}
