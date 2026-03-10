#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#ifdef __cplusplus
extern "C" {
#endif

void Ultrasonic_Init(void);
float Ultrasonic_ReadDistanceCm(void);
float Ultrasonic_ReadDistanceCmAvg(int samples);
void TaskUltrasonic(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif