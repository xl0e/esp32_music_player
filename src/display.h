#pragma once

#include <TFT_eSPI.h> // Hardware-specific library

#include "pin_config.h"
// #include "logger.h"

#include "fonts/Seven_Segment10pt7b.h"
#include "fonts/Seven_Segment8pt7b.h"
#include "fonts/FreeSans7pt8b.h"
#include "fonts/FreeSansBold7pt7b.h"


#include "source.h"

#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define GRAY 0x5ac9

namespace Display
{
  namespace Colors
  {
    uint16_t BATTERY = GREEN;
    uint16_t BG = BLACK;
    uint16_t PROGRESS = CYAN;
    uint16_t TIME = CYAN;
    uint16_t TITLE = WHITE;
    uint16_t ERROR = RED;
    uint16_t ARTIST = WHITE;
  }

  namespace Position
  {
    const uint16_t TIME_Y = 91;
    const uint16_t BAR_Y = 95;
    const uint16_t TITLE_Y = 43;
    const uint16_t ARTIST_Y = 63;

    const uint16_t BAR_H = 8;
  }

  const uint16_t SCREEN_W = 160;
  const uint16_t SCREEN_H = 128;
  const uint16_t SCREEN_W2 = SCREEN_W / 2;
  const uint16_t SCREEN_H2 = SCREEN_H / 2;

  const String S0 = "0";
  const String SC = ":";
  const String S00C = "00:";
  const String S88C88C88 = "88:88:88";

  TFT_eSPI tft = TFT_eSPI();

  bool isOk;
  uint16_t bitrate;
  uint16_t duration;
  uint16_t position;
  uint16_t prevPosX;
  uint16_t volume;
  char name[10] = {""};
  char title[160];
  char artist[160];
  const uint16_t SCROLL_PERIOD_MS = 250; //
  const uint16_t SCROLL_PADDING_PX = 2;
  const uint16_t SCROLL_PAUSE_MS = 1000;
  const uint16_t SCROLL_STEP_PX = 3;
  uint16_t artistTextWidth = 0;
  uint16_t titleTextWidth = 0;
  int16_t artistLastX = 0;
  int16_t titleLastX = 0;
  uint32_t lastTitleScrollMs = 0;
  uint32_t lastArtistScrollMs = 0;

  uint8_t voltagePercent;

  void tftPrintRight(const int16_t x, const int16_t y, const String text, const uint16_t color)
  {
    tft.setTextColor(color);
    tft.setTextDatum(BR_DATUM);
    tft.drawString(text, x, y);
  }

  void tftPrintLeft(const int16_t x, const int16_t y, const String text, const uint16_t color)
  {
    tft.setTextColor(color);
    tft.setTextDatum(BL_DATUM);
    tft.drawString(text, x, y);
  }

  void tftPrintCenter(const int16_t x, const int16_t y, const String text, const uint16_t color)
  {
    tft.setTextColor(color);
    tft.setTextDatum(BC_DATUM);
    tft.drawString(text, x, y);
  }

  uint16_t calcPosX()
  {
    if (duration > 0)
    {
      return 1 + constrain(158 * position / duration, 0, 158);
    }
    return 1;
  }

  String addZero(const int16_t d)
  {
    if (d < 10)
    {
      return S0 + String(d);
    }
    return String(d);
  }

  String convertTime(int16_t d)
  {
    if (d < 60)
    {
      return S00C + addZero(d);
    }
    else if (d < 3600)
    {
      return addZero(d / 60) + SC + addZero(d % 60);
    }
    auto m = d % 3600;
    return addZero(d / 3600) + SC + addZero(m / 60) + SC + String(m % 60);
  }

  void drawTime(const int16_t x, const int16_t y, const String time, bool right = false)
  {
    tft.setFreeFont(&Seven_Segment8pt7b);
    if (right)
    {
      tftPrintRight(x, y, S88C88C88, Colors::BG);
      if (isOk)
        tftPrintRight(x, y, time, Colors::PROGRESS);
    }
    else
    {
      tftPrintLeft(x, y, S88C88C88, Colors::BG);
      if (isOk)
        tftPrintLeft(x, y, time, Colors::PROGRESS);
    }
  }

  void drawTrack(const char *title)
  {
  }

  void drawAlbum(const char *title)
  {
  }

  void drawArtist()
  {
    tft.setTextWrap(false);
    auto y0 = Position::ARTIST_Y;
    tft.setFreeFont(&FreeSans7pt8b);
    tft.fillRect(0, y0 - 15, SCREEN_W, 15, Colors::BG);
    if (artistTextWidth < SCREEN_W)
    {
      tftPrintCenter(SCREEN_W2, y0, artist, Colors::ARTIST);
    }
    else
    {
      tftPrintLeft(artistLastX, y0, artist, Colors::ARTIST);
    }
  }

  void setArtist(const char *str)
  {
    strcpy(artist, str);
    lastArtistScrollMs = 0;
    artistLastX = SCROLL_PADDING_PX;
    artistTextWidth = tft.textWidth(artist) + SCROLL_PADDING_PX;
    log_d("Set artist [%s], %d", artist, artistTextWidth);
    drawArtist();
  }

  void drawTitle()
  {
    tft.setTextWrap(false);
    auto y0 = Position::TITLE_Y;
    tft.setFreeFont(&FreeSans7pt8b);
    tft.fillRect(0, y0 - 15, SCREEN_W, 15, Colors::BG);
    if (titleTextWidth <= SCREEN_W)
    {
      tftPrintCenter(SCREEN_W2, y0, title, Colors::TITLE);
    }
    else
    {
      tftPrintLeft(titleLastX, y0, title, Colors::TITLE);
    }
  }

  void setTitle(const char *str)
  {
    strcpy(title, str);
    lastTitleScrollMs = 0;
    titleLastX = SCROLL_PADDING_PX;
    titleTextWidth = tft.textWidth(title) + SCROLL_PADDING_PX;
    log_d("Set title [%s], %d", title, titleTextWidth);
    drawTitle();
  }

  void drawRssi(int16_t x, int16_t y, int16_t value, uint16_t color)
  {
    tft.fillRect(x, y + 12, 3, 3, value > 0 ? color : GRAY);
    tft.fillRect(x + 5, y + 9, 3, 6, value > 30 ? color : GRAY);
    tft.fillRect(x + 10, y + 6, 3, 9, value > 50 ? color : GRAY);
    tft.fillRect(x + 15, y + 3, 3, 12, value > 70 ? color : GRAY);
    tft.fillRect(x + 20, y, 3, 15, value > 90 ? color : GRAY);
  }

  void drawBattery(const uint8_t value)
  {
    if (value == voltagePercent)
    {
      return;
    }
    log_d("Draw battery");
    auto x = SCREEN_W - 28;
    tft.drawRect(x, 0, 28, 15, WHITE);
    tft.drawFastVLine(x, 5, 5, Colors::BG);
    tft.drawFastVLine(x - 2, 5, 5, WHITE);
    tft.drawFastHLine(x - 2, 5, 2, WHITE);
    tft.drawFastHLine(x - 2, 10, 2, WHITE);

    tft.fillRect(x + 2,  2, 4, 11, value > 90 ? GREEN : GRAY);
    tft.fillRect(x + 7,  2, 4, 11, value > 75 ? GREEN : GRAY);
    tft.fillRect(x + 12, 2, 4, 11, value > 60 ? GREEN : GRAY);
    tft.fillRect(x + 17, 2, 4, 11, value > 35 ? GREEN : GRAY);
    tft.fillRect(x + 22, 2, 4, 11, value > 15 ? GREEN : GRAY);
  }

  void drawVolume(uint16_t v)
  {
    volume = v;
    if (isOk)
    {
      drawRssi(3, 0, v, YELLOW);
    }
    else
    {
      drawRssi(3, 0, 0, YELLOW);
    }
  }

  void drawDuration(uint16_t v)
  {
    if (v != duration)
    {
      duration = v;
      drawTime(160, Position::TIME_Y, convertTime(v), true);
    }
  }

  void drawBitrate(uint16_t v)
  {
    if (v != bitrate)
    {
      bitrate = v;
      tft.fillRect(50, Position::TIME_Y - 16, 60, 16, Colors::BG);
      if (0 != v)
      {
        tft.setFreeFont(&Seven_Segment8pt7b);
        auto str = String(v);
        str.concat(" kbps");
        tftPrintCenter(80, Position::TIME_Y, str, Colors::TIME);
      }
    }
  }

  void drawPosition(const uint16_t v)
  {
    if (v == position)
    {
      return;
    }
    position = v;

    drawTime(0, Position::TIME_Y, convertTime(v));

    // Play bar
    int16_t y0 = Position::BAR_Y;
    auto x = calcPosX();
    tft.drawRect(0, y0, 160, Position::BAR_H, Colors::PROGRESS);
    if (x > prevPosX)
    {
      tft.fillRect(prevPosX, y0 + 1, x - prevPosX, Position::BAR_H - 2, Colors::PROGRESS);
    }
    else
    {
      tft.fillRect(x, y0 + 1, prevPosX - x, Position::BAR_H - 2, Colors::BG);
    }
    prevPosX = x;
  }

  void drawError(const char *error)
  {
    strcpy(title, error);
    tft.setTextWrap(false);
    auto y0 = Position::TITLE_Y;
    tft.setFreeFont(&FreeSans7pt8b);
    tft.fillRect(0, y0 - 15, SCREEN_W, 15, Colors::BG);
    if (titleTextWidth <= SCREEN_W)
    {
      tftPrintCenter(SCREEN_W2, y0, error, Colors::ERROR);
    }
    else
    {
      tftPrintLeft(titleLastX, y0, error, Colors::ERROR);
    }
  }

  void drawHeader()
  {
    log_d("Draw header");
    tft.setFreeFont(&FreeSans7pt8b);
    tft.fillRect(30, 0, 100, 16, Colors::BG);
    auto w = tft.textWidth(name);
    tft.fillRect(80 - w, 0, 2 * w, 16, isOk ? BLUE : RED);
    tftPrintCenter(80, 16, name, WHITE);
  }

  void setName(const char *s)
  {
    strcpy(name, s);
  }

  void drawConnected(const bool value)
  {
    isOk = value;
    drawHeader();
    drawPosition(0);
    drawDuration(0);
    drawBitrate(0);
    setTitle("");
    setArtist("");
  }

  void scrollTitle(const uint32_t timeMs)
  {
    if (titleTextWidth < SCREEN_W || timeMs < lastTitleScrollMs)
    {
      return;
    }

    if (timeMs - lastTitleScrollMs > SCROLL_PERIOD_MS)
    {
      drawTitle();
      lastTitleScrollMs = timeMs;
      if (titleLastX == SCROLL_PADDING_PX)
      {
        lastTitleScrollMs += SCROLL_PAUSE_MS;
      }
      if (titleLastX < SCREEN_W - titleTextWidth)
      {
        titleLastX = SCROLL_PADDING_PX;
        lastTitleScrollMs += SCROLL_PAUSE_MS;
      }
      else
      {
        titleLastX -= SCROLL_STEP_PX;
      }
    }
  }

  void scrollArtist(const uint32_t timeMs)
  {
    if (artistTextWidth < SCREEN_W || timeMs < lastArtistScrollMs)
    {
      return;
    }
    if (timeMs - lastArtistScrollMs > SCROLL_PERIOD_MS)
    {
      drawArtist();
      lastArtistScrollMs = timeMs;
      if (artistLastX == SCROLL_PADDING_PX)
      {
        lastArtistScrollMs += SCROLL_PAUSE_MS;
      }
      if (artistLastX < SCREEN_W - artistTextWidth)
      {
        artistLastX = SCROLL_PADDING_PX;
        lastArtistScrollMs += SCROLL_PAUSE_MS;
      }
      else
      {
        artistLastX -= SCROLL_STEP_PX;
      }
    }
  }

  void scroll(const uint32_t timeMs)
  {
    scrollTitle(timeMs);
    scrollArtist(timeMs);
  }

  void init()
  {
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(Colors::BG);
    drawHeader();
    volume = 1;
    drawVolume(0);
    duration = 1;
    drawDuration(0);
    position = 1;
    drawPosition(0);
  }
}
