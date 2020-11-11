#include <Arduino.h>

#define KEY_THRESHOLD 70

#define KEY_DEBOUNCE_MS 700
#define KEY_REPEAT_MS 90

uint32_t ts_start; 
xQueueHandle xQueue;
TaskHandle_t hTouchTask;

// debounce and repeat
bool is_key(int key) {

    static int lastkey = 0;
    static int lastts = 0;
    static int state = 0;
    static int keyts = 0;

    uint32_t ts = millis();

    // reset on inactivity
    if ( key != lastkey || ( ts - lastts) > KEY_REPEAT_MS) {
        state = 0;
        keyts = 0;
    }

    lastkey = key;
    lastts  = ts;

    if (state == 0 ||
        (state == 2 && (ts - keyts > KEY_DEBOUNCE_MS))) {
        keyts = ts;
        state = 1;
        return true;
    } 
    if (state == 1 && (ts - keyts) > KEY_DEBOUNCE_MS) {
        keyts = ts;
        state = 2;
        return true;
    } 
    if (state == 2 && ((ts - keyts) > KEY_REPEAT_MS)) {
        keyts = ts;
        return true;
    }
    return false;
}

void inline handle_touchkey(int key) {
    xQueueSendFromISR(xQueue, &key, NULL);
}

void IRAM_ATTR T7handler(void) {
    handle_touchkey(7);
}

void IRAM_ATTR T8handler(void) {
    handle_touchkey(8);
}

void IRAM_ATTR T9handler(void) {
    handle_touchkey(9);
}

void TouchTask(void *p) {

    static bool keys_enabled = false;

    for(;;) {
        int key;
        BaseType_t xStatus = xQueueReceive( xQueue, &key, portMAX_DELAY );
        if (xStatus == 0) {
            Serial.println("failed to create xQueueReceive.");
            while (1) {}
        }
        if (!keys_enabled) {
            if (millis() - ts_start < 1000)
                continue;
            keys_enabled = true;
        }

        if (is_key(key)) {
            Serial.printf("key %u\n", key);
        }
    }
}

void setup() {

    Serial.begin(115200);

    ts_start = millis();

    xQueue = xQueueCreate(99, sizeof(int));
    if (xQueue == 0) {
        Serial.println("xQueue failed to create."); 
        while (1) {}
    }

    xTaskCreate(TouchTask,    /* Task function. */
            "TouchTask",   /* name of task. */
            10000,   /* Stack size of task */
            NULL,   /* parameter of the task */
            3,   /* priority of the task */
            &hTouchTask);   /* Task handle to keep track of created task */

    touchAttachInterrupt(T7, T7handler, KEY_THRESHOLD);
    touchAttachInterrupt(T8, T8handler, KEY_THRESHOLD);
    touchAttachInterrupt(T9, T9handler, KEY_THRESHOLD);
}

void loop() {
}
