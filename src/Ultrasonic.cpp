#include "Ultrasonic.h"
#include <Arduino.h>
#include "Config.h"
#include "SharedData.h"
#include "DebuggerBLE.h"

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS 1
#endif

static long Ultrasonic_ReadPulseUs(void) {
    digitalWrite(ULTRASONIC_TRIG, LOW);
    delayMicroseconds(2);

    digitalWrite(ULTRASONIC_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG, LOW);

    long duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30000);
    return duration;
}

static bool Ultrasonic_ShouldMonitor(void) {
    if (holdActive) return false;
    if (obstacleReportPending || obstacleBacktrackActive || obstacleAwaitingRoute) return false;
    if (runBlind) return false;
    return (currentCommand == CMD_FORWARD);
}

static void Ultrasonic_TriggerObstacle(float distanceCm) {
    obstacleDistanceCm = distanceCm;
    obstacleReportPending = true;
    obstacleBacktrackActive = true;
    obstacleAwaitingRoute = false;

    scannerArmed = false;
    scannerUnlockAtMs = (uint32_t)(millis() + OBSTACLE_BACK_ARM_DELAY_MS);

    currentCommand = CMD_BACK;
    runBlind = true;
    currentHeading = (RobotHeading)((currentHeading + 2) % 4);

    debug_printf("[OBSTACLE] %.2f cm ahead while moving to %s from %s -> publish + BACKTRACK\n",
                 distanceCm,
                 (strlen(currentTarget) > 0 ? currentTarget : "UNKNOWN"),
                 (strlen(lastKnownPos) > 0 ? lastKnownPos : "UNKNOWN"));
}

void Ultrasonic_Init(void) {
    pinMode(ULTRASONIC_TRIG, OUTPUT);
    pinMode(ULTRASONIC_ECHO, INPUT);

    digitalWrite(ULTRASONIC_TRIG, LOW);
    debug_printf("[ULTRASONIC] Init done\n");
}

float Ultrasonic_ReadDistanceCm(void) {
    long duration = Ultrasonic_ReadPulseUs();

    if (duration <= 0) {
        return -1.0f;
    }

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

void TaskUltrasonic(void *pvParameters) {
    (void)pvParameters;

    Ultrasonic_Init();

    uint8_t consecutiveHits = 0;
    uint32_t cooldownUntil = 0;

    while (1) {
        uint32_t now = millis();

        if ((int32_t)(now - cooldownUntil) < 0) {
            vTaskDelay(OBSTACLE_POLL_MS / portTICK_PERIOD_MS);
            continue;
        }

        if (!Ultrasonic_ShouldMonitor()) {
            consecutiveHits = 0;
            vTaskDelay(OBSTACLE_POLL_MS / portTICK_PERIOD_MS);
            continue;
        }

        float distance = Ultrasonic_ReadDistanceCmAvg(OBSTACLE_SAMPLES);

        if (distance > 0.0f && distance <= OBSTACLE_DISTANCE_CM) {
            consecutiveHits++;
            debug_printf("[ULTRASONIC] Near obstacle hit %u/%u: %.2f cm\n",
                         consecutiveHits, OBSTACLE_CONFIRM_HITS, distance);

            if (consecutiveHits >= OBSTACLE_CONFIRM_HITS) {
                Ultrasonic_TriggerObstacle(distance);
                consecutiveHits = 0;
                cooldownUntil = millis() + OBSTACLE_COOLDOWN_MS;
            }
        } else {
            consecutiveHits = 0;
        }

        vTaskDelay(OBSTACLE_POLL_MS / portTICK_PERIOD_MS);
    }
}
