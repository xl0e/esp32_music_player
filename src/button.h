#pragma once

#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <Arduino.h>

typedef std::function<void(void)> ButtonHadler;

enum class ButtonType {
  ACTIVE_LOW,
  ACTIVE_HIGH
};


class Button
{
private:
  ButtonHadler _onClick;
  ButtonHadler _onDblClick;
  ButtonHadler _onHold;
  ButtonHadler _onDown;
  ButtonHadler _onUp;

  unsigned short _debounceMs = 50;
  unsigned short _dblClickWindowMs = 300;
  unsigned short _holdThresholdMs = 500;
  unsigned short _holdRepeatMs = 250;

  bool _onHoldSet = false;
  bool _holdRepeat = false;

  int _pin;
  int _onValue;
  int _lastValue;

  unsigned long _lastChangedMs;
  unsigned long _lastClickMs;
  unsigned long _holdStartMs = 0;

  bool _active;

public:
  Button(int pin, ButtonType type = ButtonType::ACTIVE_HIGH);

  void tick();
  void tick(const unsigned long);

  void setOnClick(ButtonHadler);
  void setOnDblClick(ButtonHadler);
  void setOnHold(ButtonHadler handler, bool repeat = false);
  void setOnDown(ButtonHadler);
  void setOnUp(ButtonHadler);
};

#endif //_BUTTON_H_