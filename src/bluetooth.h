#pragma once


#include "BluetoothA2DPSink.h"
#include "i2s_wrapper.h"


#include "pin_config.h"

namespace Bluetooth
{
  const BaseType_t CORE_ID = 0;
  const uint16_t POSITION_SEND_INTERVAL = 100;

  EventBus myEvents;

  BluetoothA2DPSink *a2dp_sink;

  bool playing = false;
  volatile uint32_t lastPositionSendMs = 0;
  volatile uint16_t lastPositionMs = 0;
  int16_t volume = -1;

  void bindCallbacks();
  void loop();
  void end();

  void avrc_rn_play_pos_callback(uint32_t pos)
  {
    lastPositionMs = pos;
    lastPositionSendMs = millis();
    myEvents.sendEvent(SourceEvent::TRACK_POSITION, lastPositionMs / 1000);
  }

  void avrc_rn_volumechange(int v)
  {
    log_i("==> AVRC volume changed: %d", v);
    myEvents.sendEvent(SourceEvent::VOLUME, v);
  }

  void avrc_metadata_callback(uint8_t id, const uint8_t *text)
  {
    log_i("==> AVRC metadata rsp: attribute id 0x%x, %s", id, text);
    if (id == ESP_AVRC_MD_ATTR_TITLE)
    {
      myEvents.sendEvent(SourceEvent::TRACK_TITLE, (const char *)text);
    }
    else if (id == ESP_AVRC_MD_ATTR_ARTIST)
    {
      myEvents.sendEvent(SourceEvent::TRACK_ARTIST, (const char *)text);
    }
    else if (id == ESP_AVRC_MD_ATTR_ALBUM)
    {
      myEvents.sendEvent(SourceEvent::TRACK_ALBUM, (const char *)text);
    }
    else if (id == ESP_AVRC_MD_ATTR_PLAYING_TIME)
    {
      auto playingTime = String((const char *)text).toInt() / 1000;
      myEvents.sendEvent(SourceEvent::TRACK_DURATION, playingTime);
      lastPositionMs = 0;
      lastPositionSendMs = millis();
      myEvents.sendEvent(SourceEvent::TRACK_CHANGED);
    }
  }

  void avrc_rn_playstatus_callback(esp_avrc_playback_stat_t playback)
  {
    switch (playback)
    {
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_STOPPED:
      playing = false;
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_PLAYING:
      playing = true;
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_PAUSED:
      playing = false;
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_FWD_SEEK:
      playing = true;
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_REV_SEEK:
      playing = true;
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_ERROR:
      playing = false;
      break;
    default:
      log_e("Got unknown playback status %d", playback);
    }
    myEvents.sendEvent(SourceEvent::PLABACK_STATUS, playing);
  }

  void avrc_connection_state_callback(bool connected)
  {
    log_i("Connection state is changed: connected=%d", connected);
    if (connected)
    {
      myEvents.sendEvent(SourceEvent::CONNECTED);
      if (volume >= 0)
      {
        // xTaskCreatePinnedToCore(set_volume, "set_volume", 4096, NULL, 2, NULL, CORE_ID);
        log_i("Call a2dp_sink->set_volume(%d) core: %d", volume, xPortGetCoreID());
        a2dp_sink->set_volume(volume);
      }
    }
    else
    {
      myEvents.sendEvent(SourceEvent::DISCONNECTED);
    }
  }

  void play()
  {
    if (playing)
    {
      a2dp_sink->pause();
    }
    else
    {
      a2dp_sink->play();
    }
  }

  void previous()
  {
    a2dp_sink->previous();
  }

  void next()
  {
    a2dp_sink->next();
  }

  void rewind()
  {
    a2dp_sink->rewind();
  }

  void fastForward()
  {
    a2dp_sink->fast_forward();
  }

  bool isOk()
  {
    return a2dp_sink->is_connected();
  }

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
    case PlayerEvent::PREVIOUS:
      previous();
      break;
    case PlayerEvent::REWIND:
      rewind();
      break;
    case PlayerEvent::FAST_FORWARD:
      fastForward();
      break;
    case PlayerEvent::SET_VOLUME:
    {
      log_i("Set volume %d", value);
      if (a2dp_sink->is_connected())
      {
        a2dp_sink->set_volume(value);
      }
      volume = value;
      break;
    }
    case PlayerEvent::CLOSE:
      end();
      break;
    case PlayerEvent::TICK:
      loop();
      break;
    }
  }

  void begin(EventConsumer *player, I2SWrapper* i2s)
  {
    log_i("Beginning BT");
    auto out = i2s->getI2s();
    a2dp_sink = new BluetoothA2DPSink(*out);
    
    log_i("A2DP sink is created");
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);

    bindCallbacks();

    log_i("Starting A2DP sink");
    // a2dp_sink->set_task_core(CORE_ID);
    a2dp_sink->start("ESP32-Music");
    player->subscribe(myEvents);
    myEvents.sendEvent(SourceEvent::DISCONNECTED);
    player->setIntEventHandler(onPlayerEvent);
    log_i("Beginning BT -- DONE");
  }

  void end()
  {
    log_i("Closing BT");
    myEvents.sendEvent(SourceEvent::DISCONNECTED);
    if (a2dp_sink != nullptr)
    {
      log_i("Disconnecting BT");
      a2dp_sink->disconnect();
      log_i("Stopping BT");
      a2dp_sink->end(true);
      delete a2dp_sink;
    }
    log_i("Done. Free heap %d", esp_get_free_heap_size());
  }

  void loop()
  {
    // update time
    if (playing)
    {
      auto ms = millis();
      auto deltaMs = ms - lastPositionSendMs;
      if (deltaMs > POSITION_SEND_INTERVAL)
      {
        lastPositionSendMs = ms;
        lastPositionMs += deltaMs;
        myEvents.sendEvent(SourceEvent::TRACK_POSITION, lastPositionMs / 1000);
      }
    }
  }



  void bindCallbacks()
  {
    log_i("Binding callbacks");
    log_i("Setting set_avrc_metadata_attribute_mask");
    a2dp_sink->set_avrc_metadata_attribute_mask(
        ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_PLAYING_TIME);
    log_i("Setting set_avrc_connection_state_callback");
    a2dp_sink->set_avrc_connection_state_callback(avrc_connection_state_callback);
    log_i("Setting set_avrc_metadata_callback");
    a2dp_sink->set_avrc_metadata_callback(avrc_metadata_callback);
    log_i("Setting set_avrc_rn_play_pos_callback");
    a2dp_sink->set_avrc_rn_play_pos_callback(avrc_rn_play_pos_callback, 2);
    log_i("Setting set_avrc_rn_volumechange");
    a2dp_sink->set_avrc_rn_volumechange(avrc_rn_volumechange);
    log_i("Setting set_avrc_rn_playstatus_callback");
    a2dp_sink->set_avrc_rn_playstatus_callback(avrc_rn_playstatus_callback);
    log_i("Setting a2dp_sink->set_auto_reconnect");
    a2dp_sink->set_auto_reconnect(true);
  }
}