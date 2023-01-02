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

#define FRAME_START_X 0
#define FRAME_START_Y 7

char buf[80];

void wait_draw()
{
  // u8g.drawStr(FRAME_START_X, FRAME_START_Y, "please wait... ");
  oled.clear(PAGE);
  printTitle("please wait...", 0);
  oled.display();
}

void receiver_draw(uint32_t channel)
{
  oled.clear(PAGE);
  // display voltage
  /*
  u8g.setFont(u8g_font_6x10);
  u8g.drawStr(FRAME_START_X, FRAME_START_Y, "vbatt: ");
  u8g.setPrintPos(FRAME_START_X + 40, FRAME_START_Y);
  u8g.print(volt);
  */
  // oled.setCursor(FRAME_START_X, FRAME_START_Y);
  // oled.print("vbatt: ");
  // oled.print(volt);

  // display current freq and the next strong one
  // u8g.setFont(u8g_font_8x13); // ?? TODO
  sprintf(buf, "curr [%x]%d:%d", pgm_read_byte_near(channelNames + channel), pgm_read_word_near(channelFreqTable + channel), rx5808.getRssi(channel)); // frequency:RSSI
  // u8g.drawStr(FRAME_START_X, FRAME_START_Y + 15, buf);
  oled.setCursor(FRAME_START_X, FRAME_START_Y);
  oled.print(buf);

  uint16_t next_chan = rx5808.getNext(channel);
  sprintf(buf, "next [%x]%d:%d", pgm_read_byte_near(channelNames + next_chan), pgm_read_word_near(channelFreqTable + next_chan), rx5808.getRssi(next_chan)); // frequency:RSSI
  // u8g.drawStr(FRAME_START_X, FRAME_START_Y + 30, buf);
  oled.setCursor(FRAME_START_X, FRAME_START_Y + 15);
  oled.print(buf);

  // histo below
  for (int i = CHANNEL_MIN; i < CHANNEL_MAX; i++)
  {
    uint8_t channelIndex = pgm_read_byte_near(channelList + i); // retrive the value based on the freq order
    uint16_t rssi_scaled = map(rx5808.getRssi(channelIndex), 1, BIN_H, 1, BIN_H / 2);
    // u8g.drawVLine(5 + 3 * i, 65 - rssi_scaled, rssi_scaled); // for bar plot, half size because of the small space
    oled.lineV(5 + 3 * i, 65 - rssi_scaled, rssi_scaled);
  }
  oled.display();
}

// draw all the channels of a certain band
void scanner_draw(uint8_t band)
{
  oled.clear(PAGE);

  int i, x = 0, y = 0, offset = 8 * band;
  // u8g.setFont(u8g_font_8x13B); // header

  switch (band)
  {
  case 0:
    // u8g.drawStr(0, 10, "BAND A");
    oled.setCursor(0, 10);
    oled.print("BAND A");
    break;
  case 1:
    // u8g.drawStr(0, 10, "BAND B");
    oled.setCursor(0, 10);
    oled.print("BAND B");
    break;
  case 2:
    // u8g.drawStr(0, 10, "BAND E");
    oled.setCursor(0, 10);
    oled.print("BAND E");
    break;
  case 3:
    // u8g.drawStr(0, 10, "FATSHARK");
    oled.setCursor(0, 10);
    oled.print("FATSHARK");
    break;
  case 4:
    // u8g.drawStr(0, 10, "RACEBAND");
    oled.setCursor(0, 10);
    oled.print("RACEBAND");
    break;
  case 5:
    spectrum_draw();
    return;
  case 6:
    summary_draw();
    return;
  }

  // u8g.setFont(u8g_font_6x10); // smaller for channels

  // make a small histo down left and print the rssi information of the 8 channels of the band
  for (i = 0; i < 8; i++)
  {
    // u8g.drawVLine(2 * i + 5, 20 + BIN_H - rx5808.getRssi(offset + i), rx5808.getRssi(offset + i));        // for bar plot
    oled.lineV(2 * i + 5, 20 + BIN_H - rx5808.getRssi(offset + i), rx5808.getRssi(offset + i));
    sprintf(buf, "%d:%d", pgm_read_word_near(channelFreqTable + offset + i), rx5808.getRssi(offset + i)); // frequency:RSSI
    // u8g.drawStr(30 + x, 30 + 10 * y++, buf);
    oled.setCursor(30 + x, 30 + 10 * y++);
    oled.print(buf);

    if (i == 3)
    {
      x = 45;
      y = 0;
    }
  }

  // computation of the max value
  uint16_t chan = rx5808.getMaxPosBand(band);
  sprintf(buf, "MAX %d", pgm_read_word_near(channelFreqTable + chan));
  // u8g.drawStr(75, 7, buf);
  oled.setCursor(75, 7);
  oled.print(buf);

  // computation of the min value
  chan = rx5808.getMinPosBand(band);
  sprintf(buf, "MIN %d", pgm_read_word_near(channelFreqTable + chan));
  // u8g.drawStr(75, 17, buf);
  oled.setCursor(75, 17);
  oled.print(buf);

  oled.display();
}

// draw based on the frequency order
void spectrum_draw()
{
  // u8g.setFont(u8g_font_8x13B);
  // u8g.drawStr(0, 10, "SPECTRUM");
  oled.setCursor(0, 10);
  oled.print("SPECTRUM");

  for (int i = CHANNEL_MIN; i < CHANNEL_MAX; i++)
  {
    uint8_t channelIndex = pgm_read_byte_near(channelList + i); // retrive the value based on the freq order
    // u8g.drawVLine(5 + 3 * i, 20 + BIN_H - rx5808.getRssi(channelIndex), rx5808.getRssi(channelIndex)); // for bar plot
    oled.lineV(5 + 3 * i, 20 + BIN_H - rx5808.getRssi(channelIndex), rx5808.getRssi(channelIndex));
  }

  // computation of the max value
  // u8g.setFont(u8g_font_6x10);
  uint16_t chan = rx5808.getMaxPos();
  sprintf(buf, "M %x:%d", pgm_read_byte_near(channelNames + chan), pgm_read_word_near(channelFreqTable + chan));
  // u8g.drawStr(75, 7, buf);
  oled.setCursor(75, 7);
  oled.print(buf);

  // computation of the min value
  chan = rx5808.getMinPos();
  sprintf(buf, "m %x:%d", pgm_read_byte_near(channelNames + chan), pgm_read_word_near(channelFreqTable + chan));
  // u8g.drawStr(75, 17, buf);
  oled.setCursor(75, 17);
  oled.print(buf);
}

// only one screen to show all the channels
void summary_draw()
{

  uint8_t i;
  // u8g.setFont(u8g_font_6x10);
  // u8g.drawStr(FRAME_START_X, FRAME_START_Y, "BAND");
  oled.setCursor(FRAME_START_X, FRAME_START_Y);
  oled.print("BAND");
  // u8g.drawStr(FRAME_START_X + 80, FRAME_START_Y, "FREE CH");
  oled.setCursor(FRAME_START_X + 80, FRAME_START_Y);
  oled.print("FREE CH");

  // u8g.setPrintPos(FRAME_START_X + 35, FRAME_START_Y);
  // u8g.print(volt);
  // u8g.drawStr(FRAME_START_X + 65, FRAME_START_Y, "v");
  oled.setCursor(FRAME_START_X + 35, FRAME_START_Y);
  oled.print(volt);
  oled.print("v");

  // u8g.drawStr(FRAME_START_X + 15, FRAME_START_Y + 12, "A");
  oled.setCursor(FRAME_START_X + 15, FRAME_START_Y + 12);
  oled.print("A");
  // u8g.drawStr(FRAME_START_X + 15, FRAME_START_Y + 22, "B");
  oled.setCursor(FRAME_START_X + 15, FRAME_START_Y + 22);
  oled.print("B");
  // u8g.drawStr(FRAME_START_X + 15, FRAME_START_Y + 32, "E");
  oled.setCursor(FRAME_START_X + 15, FRAME_START_Y + 32);
  oled.print("E");
  // u8g.drawStr(FRAME_START_X + 15, FRAME_START_Y + 42, "F");
  oled.setCursor(FRAME_START_X + 15, FRAME_START_Y + 42);
  oled.print("F");
  // u8g.drawStr(FRAME_START_X + 15, FRAME_START_Y + 52, "R");
  oled.setCursor(FRAME_START_X + 15, FRAME_START_Y + 52);
  oled.print("R");

  // u8g.drawLine(25, 0, 25, 60); // start
  oled.line(25, 0, 25, 60);
  // u8g.drawLine(76, 0, 76, 60); // end
  oled.line(76, 0, 76, 60);

#define START_BIN FRAME_START_X + 29

#define BIN_H_LITTLE 9
#define START_BIN_Y 13

  // computation of the min value
  for (i = 0; i < 5; i++)
  {
    uint16_t chan = rx5808.getMinPosBand(i);
    sprintf(buf, "%x %d", pgm_read_byte_near(channelNames + chan), pgm_read_word_near(channelFreqTable + chan));
    // u8g.drawStr(FRAME_START_X + 80, FRAME_START_Y + 10 * i + 12, buf);
    oled.setCursor(FRAME_START_X + 80, FRAME_START_Y + 10 * i + 12);
    oled.print(buf);
  }

  for (i = 0; i < 8; i++)
  {
    uint8_t bin = rx5808.getVal(0, i, BIN_H_LITTLE);
    // u8g.drawBox(START_BIN + i * 6, FRAME_START_Y + START_BIN_Y - bin, 2, bin);
    oled.rect(START_BIN + i * 6, FRAME_START_Y + START_BIN_Y - bin, 2, bin);

    bin = rx5808.getVal(1, i, BIN_H_LITTLE);
    // u8g.drawBox(START_BIN + i * 6, FRAME_START_Y + START_BIN_Y - bin + 10, 2, bin);
    oled.rect(START_BIN + i * 6, FRAME_START_Y + START_BIN_Y - bin + 10, 2, bin);

    bin = rx5808.getVal(2, i, BIN_H_LITTLE);
    // u8g.drawBox(START_BIN + i * 6, FRAME_START_Y + START_BIN_Y - bin + 20, 2, bin);
    oled.rect(START_BIN + i * 6, FRAME_START_Y + START_BIN_Y - bin + 20, 2, bin);

    bin = rx5808.getVal(3, i, BIN_H_LITTLE);
    // u8g.drawBox(START_BIN + i * 6, FRAME_START_Y + START_BIN_Y - bin + 30, 2, bin);
    oled.rect(START_BIN + i * 6, FRAME_START_Y + START_BIN_Y - bin + 30, 2, bin);

    bin = rx5808.getVal(4, i, BIN_H_LITTLE);
    // u8g.drawBox(START_BIN + i * 6, FRAME_START_Y + START_BIN_Y - bin + 40, 2, bin);
    oled.rect(START_BIN + i * 6, FRAME_START_Y + START_BIN_Y - bin + 40, 2, bin);
  }
}

// initial spash screen
void splashScr()
{
  // u8g.setFontPosTop();
  // u8g.setFont(u8g_font_6x10);
  // u8g.firstPage();
  // do
  //{
  oled.clear(PAGE);

  // u8g.drawStr(5, 10, "oled 5.8ghz scanner");
  oled.setCursor(5, 10);
  oled.print("oled 5.8ghz scanner");

  // u8g.drawStr(10, 35, "by mikym0use");
  oled.setCursor(10, 35);
  oled.print("by [BBL]");

  oled.display();
  delay(500);

  //} while (u8g.nextPage());
}

// Center and print a small title
// This function is quick and dirty. Only works for titles one
// line long.
void printTitle(String title, int font)
{
  int middleX = oled.getLCDWidth() / 2;
  int middleY = oled.getLCDHeight() / 2;

  oled.clear(PAGE);
  oled.setFontType(font);
  // Try to set the cursor in the middle of the screen
  oled.setCursor(middleX - (oled.getFontWidth() * (title.length() / 2)),
                 middleY - (oled.getFontHeight() / 2));
  // Print the title:
  oled.print(title);
}
