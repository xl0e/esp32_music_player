#pragma once

#include "Arduino.h"
#include "i2s_wrapper.h"

namespace Beeper
{
  I2SWrapper *output;
  SineWaveGenerator<int16_t> sineWave(1000);
  GeneratedSoundStream<int16_t> sound(sineWave);
  StreamCopy *copier;

  void init(I2SWrapper *i2s)
  {
    output = i2s;
    copier = new StreamCopy(*output->getI2s(), sound);
  }

  void beep(uint32_t ms, uint32_t tone = 1500)
  {
    sineWave.begin(output->getCfg(), tone);
    copier->copyMs(ms, output->getCfg());
    sineWave.end();
  }
}