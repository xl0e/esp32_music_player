#include "Arduino.h"

#include "i2s_wrapper.h"

#include "button.h"
#include "beeper.h"
#include "display.h"
#include "sd_card.h"

#include "bluetooth.h"
#include "events.h"
#include "event_bus.h"

#define BT_PLAY_PIN 35
#define BT_PREV_PIN 34
#define BT_NEXT_PIN 39
#define BT_MENU_PIN 36

#define MOSI 23
#define MISO 19
#define SCK 18
#define SS 5

#define I2S_LRC 26
#define I2S_DOUT 25
#define I2S_BCLK 27

#define SOURCE_SD 0
#define SOURCE_BT 1

#define MAX_FRAME_RATE 200 // ms

String currentFile = "/";


class PlayerClass : public EventBus
{
private:
  /*
    4.2v -> 2.1v -> 2605 = 100%
    3.4v -> 1.7v -> 2110 = 0%
    */
  static const uint16_t MAX_VOLTAGE = 2605;
  static const uint16_t MIN_VOLTAGE = 2110;
  const char *BT = "BT";
  const char *SD = "SD";
  const uint16_t VOLTAGE_READ_PERIOD_MS = 10000;

  I2SWrapper *i2s;
  // EventConsumer *sourceEvents = &NOOP_EVENT_BUS;

  Button btPlay = Button(BT_PLAY_PIN);
  Button btPrev = Button(BT_PREV_PIN);
  Button btNext = Button(BT_NEXT_PIN);
  Button btMenu = Button(BT_MENU_PIN);

  bool btPlayDown = false;

  int masterVolume = 42;

  uint32_t lastRedraw = 0;
  uint32_t lastVoltageReadMs = 0;

  int16_t realPos = 0;
  uint32_t realPosMs = 0;

  uint8_t voltagePercent = 75;

  int selectedSource;

public:
  void setupI2S()
  {
    if (i2s != nullptr)
    {
      log_d("Dispose I2S");
      delete i2s;
    }
    log_d("Create new I2S");
    i2s = new I2SWrapper(I2S_BCLK, I2S_LRC, I2S_DOUT);
    i2s->begin();
  }

  void changeVolume(int16_t d)
  {
    masterVolume = constrain(masterVolume + d, 0, 127);
  }

  void play()
  {
    log_i("Send PLAY");
    sendEvent(PlayerEvent::PLAY);
  }

  void playDown(bool isDown)
  {
    log_i("Bt Play is down = %d", isDown);
    btPlayDown = isDown;
  }

  void next(bool dbl)
  {
    if (btPlayDown)
    {
      changeVolume(10);
      sendEvent(PlayerEvent::SET_VOLUME, masterVolume);
      Display::drawVolume(masterVolume);
    }
    else
    {
      if (dbl)
      {
        log_i("Send NEXT_DIR");
        sendEvent(PlayerEvent::NEXT_DIR);
      }
      else
      {
        log_i("Send NEXT");
        sendEvent(PlayerEvent::NEXT);
      }
    }
  }

  void prev(bool dbl)
  {
    if (btPlayDown)
    {
      changeVolume(-10);
      sendEvent(PlayerEvent::SET_VOLUME, masterVolume);
      Display::drawVolume(masterVolume);
    }
    else
    {
      if (dbl)
      {
        log_i("Send PREVIOUS_DIR");
        sendEvent(PlayerEvent::PREVIOUS_DIR);
      }
      else
      {
        log_i("Send PREVIOUS");
        sendEvent(PlayerEvent::PREVIOUS);
      }
    }
  }

  void rewind()
  {
    if (btPlayDown)
    {
      changeVolume(-5);
      sendEvent(PlayerEvent::SET_VOLUME, masterVolume);
      Display::drawVolume(masterVolume);
    }
    else
    {
      log_i("Send REWIND");
      sendEvent(PlayerEvent::REWIND);
    }
  }

  void forward()
  {
    if (btPlayDown)
    {
      changeVolume(5);
      sendEvent(PlayerEvent::SET_VOLUME, masterVolume);
      Display::drawVolume(masterVolume);
    }
    else
    {
      log_i("Send FAST_FORWARD");
      sendEvent(PlayerEvent::FAST_FORWARD);
    }
  }

  void onSourceStringEvent(const uint16_t eventId, const char *message)
  {
    log_v("Source event[%d] - %s", eventId, message);
    switch (eventId)
    {
    case SourceEvent::ERROR:
      Display::drawError(message);
      break;
    case SourceEvent::TRACK_ARTIST:
      Display::setArtist(message);
      break;
    case SourceEvent::TRACK_TITLE:
      Display::setTitle(message);
      break;
    case SourceEvent::TRACK_ALBUM:
      Display::drawAlbum(message);
      break;
    case SourceEvent::TRACK_CHANGED:
    {
      Display::drawPosition(0);
      Display::drawDuration(0);
      Display::drawBitrate(0);
      break;
    }
    }
  }

  void onSourceIntEvent(const uint16_t eventId, uint16_t value)
  {
    log_v("Source event[%d] - %d", eventId, value);
    switch (eventId)
    {
    case SourceEvent::CONNECTED:
    {
      log_d("Source is connected");
      Display::drawConnected(true);
      Display::drawVolume(masterVolume);
      break;
    }
    case SourceEvent::DISCONNECTED:
    {
      log_d("Source is disconnected");
      Display::drawConnected(false);
      Display::drawVolume(masterVolume);
      break;
    }
    case SourceEvent::VOLUME:
    {
      masterVolume = value;
      Display::drawVolume(masterVolume);
      break;
    }
    case SourceEvent::TRACK_DURATION:
    {
      Display::drawDuration(value);
      break;
    }
    case SourceEvent::TRACK_BITRATE:
    {
      Display::drawBitrate(value);
      break;
    }
    case SourceEvent::TRACK_POSITION:
    {
      Display::drawPosition(value);
      break;
    }
    default:
      log_i("Unknown source event: %d", eventId);
    }
  }

  void setSd()
  {
    sendEvent(PlayerEvent::CLOSE);
    selectedSource = SOURCE_SD;
    Display::setName(SD);
    Display::drawConnected(false);
    setupI2S();
    SdCard::begin(this, i2s);
    sendEvent(PlayerEvent::SET_VOLUME, masterVolume);
  }

  void setBluetooth()
  {
    sendEvent(PlayerEvent::CLOSE);
    selectedSource = SOURCE_BT;
    Display::setName(BT);
    Display::drawConnected(false);
    setupI2S();
    Bluetooth::begin(this, i2s);
    log_d("Set source volume");
    sendEvent(PlayerEvent::SET_VOLUME, masterVolume);
  }

  void changeSource()
  {
    if (selectedSource == SOURCE_BT)
    {
      log_i("Change source to SD");
      setSd();
    }
    else if (selectedSource == SOURCE_SD)
    {
      log_i("Change source to Bluetooth");
      setBluetooth();
    }
  }

  void updateVoltage(uint32_t ms)
  {
    if (ms - lastVoltageReadMs > VOLTAGE_READ_PERIOD_MS)
    {
      /*
      Divider 1/2
      4.2v -> 2.1v -> 2605 = 100%
      3.4v -> 1.7v -> 2110 = 0%
      */
      auto vbat = analogRead(BAT_VCC);
      log_d("Bat voltage raw: %d", vbat);
      if (vbat > MIN_VOLTAGE)
      {
        voltagePercent = 100 * (vbat - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE);
      }
      else
      {
        voltagePercent = 0;
      }
      Display::drawBattery(voltagePercent);
      lastVoltageReadMs = ms;
    }
  }

  void init()
  {
    pinMode(BAT_VCC, INPUT);

    btPlay.setOnClick([this]
                      { play(); });
    btPlay.setOnDown([this]
                     { playDown(true); });
    btPlay.setOnUp([this]
                   { playDown(false); });

    btNext.setOnClick([this]
                      { next(false); });
    btPrev.setOnClick([this]
                      { prev(false); });

    btNext.setOnDblClick([this]
                         { next(true); });
    btPrev.setOnDblClick([this]
                         { prev(true); });

    btNext.setOnHold([this]
                     { forward(); }, true);
    btPrev.setOnHold([this]
                     { rewind(); }, true);

    btMenu.setOnHold([this]
                     { changeSource(); });

    Display::init();
    updateVoltage(1);
    if (SdCard::checkSd())
    {
      setSd();
    }
    else
    {
      setBluetooth();
    }
  }

  void loop()
  {
    uint32_t ms = millis();
    btPlay.tick();
    btPrev.tick();
    btNext.tick();
    btMenu.tick();

    sendEvent(PlayerEvent::TICK);
    Display::scroll(ms);
    updateVoltage(ms);
  }

  void subscribe(EventConsumer &source)
  {
    source.setStringEventHandler([this](const uint16_t e, const char *v)
                                 { onSourceStringEvent(e, v); });
    source.setIntEventHandler([this](const uint16_t e, const uint16_t v)
                              { onSourceIntEvent(e, v); });
  }
};

PlayerClass Player;