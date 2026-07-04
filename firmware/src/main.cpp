#include <Arduino.h>

uint32_t counter = 0;

void setup()
{
    Serial.begin(115200);
    delay(3000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("CR Field Analyzer - Loop Counter Test");
    Serial.println("========================================");
}

void loop()
{
    counter++;

    Serial.println();
    Serial.println("----------------------------------------");

    Serial.print("Loop #: ");
    Serial.println(counter);

    Serial.print("Uptime: ");
    Serial.print(millis());
    Serial.println(" ms");

    Serial.print("Chip Model: ");
    Serial.println(ESP.getChipModel());

    Serial.print("Flash: ");
    Serial.print(ESP.getFlashChipSize() / (1024 * 1024));
    Serial.println(" MB");

    Serial.print("PSRAM: ");
    Serial.print(ESP.getPsramSize() / (1024 * 1024));
    Serial.println(" MB");

    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());

    delay(3000);
}