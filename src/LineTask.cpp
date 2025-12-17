// LineTask.cpp
#include "LineTask.h"
#include "Config.h"
#include "SharedData.h"

// Biến cho PID
int lastError = 0;

void setupMotors() {
    pinMode(MOTOR_L_IN1, OUTPUT);
    pinMode(MOTOR_L_IN2, OUTPUT);
    pinMode(MOTOR_L_PWM, OUTPUT);
    pinMode(MOTOR_R_IN3, OUTPUT);
    pinMode(MOTOR_R_IN4, OUTPUT);
    pinMode(MOTOR_R_PWM, OUTPUT);
}

void setupSensors() {
    pinMode(SENSOR_1, INPUT);
    pinMode(SENSOR_2, INPUT);
    pinMode(SENSOR_3, INPUT);
    pinMode(SENSOR_4, INPUT);
    pinMode(SENSOR_5, INPUT);
}

// Hàm điều khiển motor cơ bản
void setMotorSpeed(int leftSpeed, int rightSpeed) {
    // Motor Trái
    if (leftSpeed >= 0) {
        digitalWrite(MOTOR_L_IN1, HIGH);
        digitalWrite(MOTOR_L_IN2, LOW);
    } else {
        digitalWrite(MOTOR_L_IN1, LOW);
        digitalWrite(MOTOR_L_IN2, HIGH);
        leftSpeed = -leftSpeed;
    }
    analogWrite(MOTOR_L_PWM, constrain(leftSpeed, 0, 255));

    // Motor Phải
    if (rightSpeed >= 0) {
        digitalWrite(MOTOR_R_IN3, HIGH);
        digitalWrite(MOTOR_R_IN4, LOW);
    } else {
        digitalWrite(MOTOR_R_IN3, LOW);
        digitalWrite(MOTOR_R_IN4, HIGH);
        rightSpeed = -rightSpeed;
    }
    analogWrite(MOTOR_R_PWM, constrain(rightSpeed, 0, 255));
}

// Đọc cảm biến và tính sai số (-2 đến 2)
int getError() {
    int s1 = digitalRead(SENSOR_1);
    int s2 = digitalRead(SENSOR_2);
    int s3 = digitalRead(SENSOR_3);
    int s4 = digitalRead(SENSOR_4);
    int s5 = digitalRead(SENSOR_5);

    // Giả sử Line là Mức 1 (HIGH). Nếu Line là mức 0 thì đảo ngược logic.
    // Logic trọng số: Trái < 0, Phải > 0, Giữa = 0
    if (s1) return -2; // Lệch trái nhiều
    if (s2) return -1; // Lệch trái ít
    if (s3) return 0;  // Giữa
    if (s4) return 1;  // Lệch phải ít
    if (s5) return 2;  // Lệch phải nhiều
    
    return lastError; // Mất line thì giữ sai số cũ
}

void TaskLine(void *pvParameters) {
    setupMotors();
    setupSensors();

    for (;;) {
        // Kiểm tra lệnh từ SharedData
        switch (currentCommand) {
            case CMD_STOP:
                setMotorSpeed(0, 0);
                break;

            case CMD_TURN_LEFT:
                Serial.println(">>> TURNING LEFT (90 Degree)...");
                // Quay tại chỗ: Trái lùi, Phải tiến
                setMotorSpeed(-TURN_SPEED, TURN_SPEED); 
                vTaskDelay(pdMS_TO_TICKS(TURN_90_TIME)); // Chờ quay xong
                
                // Quay xong thì dừng hoặc tự động chuyển sang dò line
                setMotorSpeed(0, 0);
                vTaskDelay(pdMS_TO_TICKS(100)); // Dừng nghỉ xíu cho đỡ trượt
                
                currentCommand = CMD_FORWARD; // [QUAN TRỌNG] Tự chuyển về chế độ dò line
                break;

            case CMD_TURN_RIGHT:
                Serial.println(">>> TURNING RIGHT (90 Degree)...");
                // Quay tại chỗ: Trái tiến, Phải lùi
                setMotorSpeed(TURN_SPEED, -TURN_SPEED);
                vTaskDelay(pdMS_TO_TICKS(TURN_90_TIME)); 

                setMotorSpeed(0, 0);
                vTaskDelay(pdMS_TO_TICKS(100));

                currentCommand = CMD_FORWARD; // Tự chuyển về chế độ dò line
                break;

            case CMD_FORWARD:
                // Logic PID Dò Line
                int error = getError();
                
                // Tính toán PID
                int P = error;
                int D = error - lastError;
                int correction = (KP * P) + (KD * D);
                
                lastError = error;

                int speedLeft = BASE_SPEED + correction;
                int speedRight = BASE_SPEED - correction;

                setMotorSpeed(speedLeft, speedRight);
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Chu kỳ PID (10ms là ổn)
    }
}