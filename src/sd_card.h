#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "Audio.h"
#include "i2s_wrapper.h"

#include "pin_config.h"
#include "events.h"
#include "event_bus.h"

#include "sd_nav.h"

// #include "logger.h"

namespace SdCard
{
  const uint16_t POSITION_SEND_INTERVAL = 100;
  EventBus myEvents;
  Audio *audio;
  SDNav *sdNav;
  SPIClass hSPI(HSPI);
  bool ok = false;
  bool playing;
  bool durationSend = false;

  uint32_t lastPositionSend;

  void bindCallbacks();
  void loop();
  void end();
  bool saveCurrentFile(const char *fileName);

  void play()
  {
    audio->pauseResume();
  }

  bool checkSd()
  {
    log_d("Checking SD card...");
    hSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_SS);
    if (!SD.begin(SD_SS, hSPI))
    {
      log_w("No SD card");
      return false;
    }
    log_i("SD card - OK");
    return true;
  }

  void playFile(const String &f)
  {
    audio->stopSong();
    log_i("Playing : %s", f.c_str());
    saveCurrentFile(f.c_str());
    auto fs = sdNav->getFs();
    if (audio->connecttoFS(fs, f.c_str()))
    {
      durationSend = false;
      myEvents.sendEvent(SourceEvent::TRACK_CHANGED, f.c_str());
      auto fileName = Path::getTitle(f.c_str());
      log_i("File name: %s", fileName.c_str());
      myEvents.sendEvent(SourceEvent::TRACK_TITLE, fileName.c_str());
      auto dirName = sdNav->getCurrentDirName();
      log_i("Dir name: %s", dirName.c_str());
      myEvents.sendEvent(SourceEvent::TRACK_ARTIST, dirName.c_str());
    }
    else
    {
      myEvents.sendEvent(SourceEvent::ERROR, "Can't play");
      myEvents.sendEvent(SourceEvent::TRACK_ALBUM, f.c_str());
    }
  }

  void previous()
  {
    log_i("Get previous file");
    String f = sdNav->getPrevFile();
    if (f.length() > 0)
    {
      playFile(f);
    }
  }

  void previousDir()
  {
    log_i("Get previous dir file");
    String f = sdNav->getPrevDirFile();
    if (f.length() > 0)
    {
      playFile(f);
    }
  }

  void next()
  {
    audio->stopSong();
    log_i("Get next file");
    String f = sdNav->getNextFile();
    if (f.length() > 1)
    {
      playFile(f);
    }
  }

  void nextDir()
  {
    audio->stopSong();
    log_i("Get next dir");
    String f = sdNav->getNextDirFile();
    if (f.length() > 1)
    {
      playFile(f);
    }
  }

  void rewind(int step)
  {
    auto c = audio->getAudioCurrentTime();
    audio->setAudioPlayPosition(c + step);
  }

  bool saveCurrentFile(const char *fileName)
  {
    if (sdNav == nullptr)
    {
      log_e("sdNav is null");
      return false;
    }

    auto fs = sdNav->getFs();
    log_d("Openning .current");
    auto f = fs.open("/.current", "w", true);
    if (f)
    {
      log_d("Saving %s to .current", fileName);
      auto len = strlen(fileName);
      f.write((uint8_t *)fileName, len + 1);
      f.flush();
      f.close();
    }
    return true;
  }

  char *restoreCurrentFile()
  {
    if (sdNav == nullptr)
    {
      log_e("sdNav is null");
    }

    auto fs = sdNav->getFs();
    log_d("Openning .current");
    auto f = fs.open("/.current");
    if (f)
    {
      log_d("Restoring from .current");
      char *file = new char[256];
      auto len = f.readBytesUntil('\0', file, 256);
      f.close();
      file[len] = '\0';
      log_d("Got file: %s", file);
      sdNav->restoreStateFromPath(file);
      char *res = strdup(file);
      free(file);
      return res;
    }
    return nullptr;
  }

  void end();

  void onPlayerEvent(uint16_t event, uint16_t value)
  {
    switch (event)
    {
    case PlayerEvent::PAUSE:
    case PlayerEvent::PLAY:
      play();
      break;
    case PlayerEvent::NEXT:
      next();
      break;
    case PlayerEvent::NEXT_DIR:
      nextDir();
      break;
    case PlayerEvent::PREVIOUS:
      previous();
      break;
    case PlayerEvent::PREVIOUS_DIR:
      previousDir();
      break;
    case PlayerEvent::REWIND:
      rewind(-3);
      break;
    case PlayerEvent::FAST_FORWARD:
      rewind(3);
      break;
    case PlayerEvent::SET_VOLUME:
      audio->setVolume(value);
      break;
    case PlayerEvent::CLOSE:
      end();
      break;
    case PlayerEvent::TICK:
      loop();
      break;
    default:
      break;
    }
  }

  void begin(EventConsumer *player, I2SWrapper *i2s)
  {
    log_i("Initing SD source...");
    // Subscribe player to my events
    player->subscribe(myEvents);

    if (!checkSd())
    {
      ok = false;
      myEvents.sendEvent(SourceEvent::ERROR, "No SD Card");
      end();
      return;
    }
    log_i("SD Card is mounted");
    log_d("Create SD nav...");
    sdNav = new SDNav(SD);

    log_d("Create Audio instance with i2s with port [%d]", i2s->getPortNo());
    audio = new Audio(false, 3, i2s->getPortNo());
    audio->setVolumeSteps(127);
    ok = true;

    bindCallbacks();
    // Bind to player events
    myEvents.sendEvent(SourceEvent::CONNECTED, 1);
    player->setIntEventHandler(onPlayerEvent);
    char *saved = restoreCurrentFile();
    if (saved != nullptr)
    {
      playFile(String(saved));
      free(saved);
    }
    else
    {
      next();
    }
  }

  void loop()
  {
    if (ok)
    {
      auto ms = millis();
      if (ms - lastPositionSend > POSITION_SEND_INTERVAL)
      {
        // send every 200-250 ms
        myEvents.sendEvent(SourceEvent::TRACK_POSITION, audio->getAudioCurrentTime());
        lastPositionSend = ms;
      }
      if (!durationSend && audio->getAudioFileDuration() > 1)
      {
        myEvents.sendEvent(SourceEvent::TRACK_DURATION, audio->getAudioFileDuration());
        myEvents.sendEvent(SourceEvent::TRACK_BITRATE, audio->getBitRate() / 1000);
        log_v("Track bitrate %d", audio->getBitRate());
        durationSend = true;
      }

      audio->loop();
      vTaskDelay(1); // Audio is distoreted without this
    }
  }

  void end()
  {
    log_i("Closing SD source");
    ok = false;
    myEvents.sendEvent(SourceEvent::DISCONNECTED);
    if (audio != nullptr)
    {
      log_i("Stopping audio");
      audio->stopSong();
      delay(200);
      delete audio;
    }
    if (sdNav != nullptr)
    {
      log_i("Deleting sd nav");
      delete sdNav;
    }

    SD.end();
    log_i("Done. Free heap %d", esp_get_free_heap_size());
  }

  void handleTag(uint16_t tag, const char *data)
  {
    switch (tag)
    {
    case ID3Tag::TIT1:
    case ID3Tag::TIT2:
    case ID3Tag::TIT3:
    {
      log_i("ID3 Title: %s", data);
      myEvents.sendEvent(TRACK_TITLE, data);
      break;
    }
    case ID3Tag::TPE1:
    case ID3Tag::TPE2:
    case ID3Tag::TPE3:
    case ID3Tag::TPE4:
    {
      log_i("ID3 Artist: %s", data);
      myEvents.sendEvent(TRACK_ARTIST, data);
      break;
    }
    case ID3Tag::TAL:
    case ID3Tag::TALB:
    {
      log_i("ID3 Album: %s", data);
      myEvents.sendEvent(TRACK_ALBUM, data);
      break;
    }
    case ID3Tag::TCON:
      log_i("ID3 Genre: %s", data);
      break;
    case ID3Tag::TYE:
    case ID3Tag::TYER:
      log_i("ID3 Year: %s", data);
      break;
    case ID3Tag::TRCK:
      log_i("ID3 Track: %s", data);
      break;
    case ID3Tag::COM:
    case ID3Tag::COMM:
      log_d("ID3 Comments: %s", data);
      break;
    default:
      log_d("ID3 callback [%d] %s", tag, data);
      break;
    }
  }

  void bindCallbacks()
  {
    log_i("Binding callbacks");
    audio->setAudioEofCallback([] { log_i("Next callback"); next(); });

    audio->setAudioId3DataCallback([](uint16_t tag, const char *data)
                                   { handleTag(tag, data); });
  }

};