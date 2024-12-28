#include "Arduino.h"

#include "player.h"

#define LED 16

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
    }
    Serial.print("setup() running on core ");
    Serial.println(xPortGetCoreID());
    Player.init();
}

void loop()
{
    Player.loop();
}
