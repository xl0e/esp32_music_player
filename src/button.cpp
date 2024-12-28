#include "button.h"

void _noop()
{
}

Button::Button(int pin, ButtonType type)
{
  _onClick = _noop;
  _onDblClick = _noop;
  _onHold = _noop;
  _onDown = _noop;
  _onUp = _noop;
  _pin = pin;
  if (type == ButtonType::ACTIVE_LOW)
  {
    pinMode(_pin, INPUT_PULLUP);
    _onValue = LOW;
  }
  else
  {
    pinMode(_pin, INPUT);
    _onValue = HIGH;
  }
}

void Button::tick()
{
  tick(millis());
}

void Button::tick(const unsigned long timeMs)
{
  auto v = digitalRead(_pin);
  if (_lastValue != v && (timeMs - _lastChangedMs > _debounceMs))
  {
    _active = _onValue == v;
    if (_active)
    {
      _holdStartMs = timeMs + _holdThresholdMs;
      _onDown();
    }
    else
    {
      _onUp();
      if (timeMs - _lastClickMs < _dblClickWindowMs)
      {
        _onDblClick();
        _lastClickMs = 0;
      }
      else if (timeMs - _lastChangedMs < _holdThresholdMs)
      {
        _lastClickMs = timeMs;
        _onClick();
      }
    }
    _lastChangedMs = timeMs;
    _lastValue = v;
  }
  else if (_onHoldSet && _active)
  {
    if (timeMs > _holdStartMs)
    {
      _onHold();
      if (_holdRepeat)
      {
        _holdStartMs = timeMs + _holdRepeatMs;
      }
      else
      {
        _active = false;
      }
    }
  }
}

void Button::setOnClick(ButtonHadler h)
{
  _onClick = h;
}

void Button::setOnDblClick(ButtonHadler h)
{
  _onDblClick = h;
}

void Button::setOnHold(ButtonHadler handler, bool repeat)
{
  _onHold = handler;
  _holdRepeat = repeat;
  _onHoldSet = true;
}

void Button::setOnDown(ButtonHadler handler)
{
  _onDown = handler;
}

void Button::setOnUp(ButtonHadler handler)
{
  _onUp = handler;
}
