#include "Ultrasonic.h"
#include <Arduino.h>
#include "Config.h"
#include "DebuggerBLE.h"

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS 1
#endif

static long Ultrasonic_ReadPulseUs(void) {
    // Đảm bảo TRIG ở mức thấp trước khi kích
    digitalWrite(ULTRASONIC_TRIG, LOW);
    delayMicroseconds(2);

    // Phát xung 10us
    digitalWrite(ULTRASONIC_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG, LOW);

    // Đo độ rộng xung ECHO, timeout 30ms ~ tối đa khoảng 5m
    long duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30000);

    return duration;
}

void Ultrasonic_Init(void) {
    pinMode(ULTRASONIC_TRIG, OUTPUT);
    pinMode(ULTRASONIC_ECHO, INPUT);

    digitalWrite(ULTRASONIC_TRIG, LOW);

    //debug_printf("[ULTRASONIC] Init done");
    debug_printf("[ULTRASONIC] Init done\n");
}

float Ultrasonic_ReadDistanceCm(void) {
    long duration = Ultrasonic_ReadPulseUs();

    if (duration <= 0) {
        return -1.0f; // timeout / không đọc được
    }

    // cm = us * 0.0343 / 2
    float distance = (duration * 0.0343f) / 2.0f;
    return distance;
}

float Ultrasonic_ReadDistanceCmAvg(int samples) {
    if (samples <= 0) {
        samples = 1;
    }

    float sum = 0.0f;
    int validCount = 0;

    for (int i = 0; i < samples; i++) {
        float d = Ultrasonic_ReadDistanceCm();
        if (d > 0.0f) {
            sum += d;
            validCount++;
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }

    if (validCount == 0) {
        return -1.0f;
    }

    return sum / validCount;
}

void TaskUltrasonicTest(void *pvParameters) {
    (void)pvParameters;

    Ultrasonic_Init();

    while (1) {
        float distance = Ultrasonic_ReadDistanceCmAvg(5);

        if (distance < 0) {
           //Serial.println("[ULTRASONIC] Read timeout");
            debug_printf("[ULTRASONIC] Read timeout\n");
        } else {
           // Serial.printf("[ULTRASONIC] Distance: %.2f cm\n", distance);
            debug_printf("[ULTRASONIC] Distance: %.2f cm\n", distance);
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}