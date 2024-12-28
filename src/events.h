#pragma once

#include <Arduino.h>

enum PlayerEvent : uint16_t
{
  CLOSE,
  SET_VOLUME,
  PLAY,
  PAUSE,
  NEXT,
  NEXT_DIR,
  PREVIOUS,
  PREVIOUS_DIR,
  REWIND,
  FAST_FORWARD,
  TICK
};

enum SourceEvent : uint16_t
{
  ERROR,
  ERROR_2,
  VOLUME,
  TRACK_CHANGED,
  TRACK_TITLE,
  TRACK_ALBUM,
  TRACK_ARTIST,
  TRACK_DURATION,
  TRACK_POSITION,
  TRACK_BITRATE,
  CONNECTED,
  DISCONNECTED,
  PLABACK_STATUS
};