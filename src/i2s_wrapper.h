#pragma once

#include "AudioTools.h"

class I2SWrapper
{
private:
  I2SStream *i2s;
  I2SConfig cfg;

public:
  void begin()
  {
    i2s->begin();
  }

  I2SStream* getI2s()
  {
    return i2s;
  }

  I2SConfig getCfg()
  {
    return cfg;
  }

  int getPortNo()
  {
    return cfg.port_no;
  }

  I2SWrapper(int bclk, int lrc, int dout)
  {
    i2s = new I2SStream();
    cfg = i2s->defaultConfig();
    cfg.pin_bck = bclk;
    cfg.pin_ws = lrc;
    cfg.pin_data = dout;
  }

  ~I2SWrapper()
  {
    if (i2s != nullptr)
    {
      log_d("Dispose I2S");
      i2s->end();
      delete i2s;
    }
  }
};