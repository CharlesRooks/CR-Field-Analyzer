#include <Arduino.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <lvgl.h>

LilyGo_Class amoled;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("SentinelOS - LilyGO AMOLED test");

    if (!amoled.begin()) {
        Serial.println("amoled.begin() failed");
        while (true) delay(1000);
    }

    Serial.print("Board: ");
    Serial.println(amoled.getName());

    beginLvglHelper(amoled);

    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "CR Field Analyzer\nSentinelOS");
    lv_obj_center(label);

    Serial.println("Display initialized.");
}

void loop()
{
    lv_task_handler();
    delay(5);
}