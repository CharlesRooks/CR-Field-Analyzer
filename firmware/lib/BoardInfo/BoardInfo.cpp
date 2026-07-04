#include <Arduino.h>
#include "BoardInfo.h"

void BoardInfo::Print()
{
    Serial.println();
    Serial.println("========== Board Information ==========");

    Serial.println("Reached Print()");

    Serial.printf("Chip Model: %s\n", ESP.getChipModel());

    Serial.println("Done");
}