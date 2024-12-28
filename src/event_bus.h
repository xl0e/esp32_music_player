#pragma once

#include <Arduino.h>

typedef std::function<void(const int16_t, const int16_t)> IntEventHandler;
typedef std::function<void(const int16_t, const char *)> StringEventHandler;

class EventProducer
{
public:
  virtual void sendEvent(const uint16_t eventId, int16_t value = 0) = 0;
  virtual void sendEvent(const uint16_t eventId, const char *value) = 0;
};

class EventConsumer
{
public:
  virtual void setIntEventHandler(IntEventHandler handler) = 0;
  virtual void setStringEventHandler(StringEventHandler handler) = 0;
  virtual void subscribe(EventConsumer &) = 0;
};


class EventBus : public EventConsumer, public EventProducer
{
private:
  IntEventHandler intEventHandler = [](const int16_t, const int16_t) {};
  StringEventHandler stringEventHandler = [](const int16_t, const char *) {};

public:
  void setIntEventHandler(IntEventHandler handler)
  {
    intEventHandler = handler;
  }

  void setStringEventHandler(StringEventHandler handler)
  {
    stringEventHandler = handler;
  }

  void sendEvent(const uint16_t eventId, int16_t value = 0)
  {
    intEventHandler(eventId, value);
  }

  void sendEvent(const uint16_t eventId, const char *value)
  {
    stringEventHandler(eventId, value);
  }

  void subscribe(EventConsumer &consumer)
  {

  }
};

EventBus NOOP_EVENT_BUS;