// LineTask.h
#ifndef LINE_TASK_H
#define LINE_TASK_H

void setupMotors();
void setupSensors();
void TaskLine(void *pvParameters);
void speed_run(int speedDC_left, int speedDC_right);

#endif