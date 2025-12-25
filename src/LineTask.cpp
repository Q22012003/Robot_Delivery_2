// LineTask.cpp
#include "LineTask.h"
#include "Config.h"
#include "SharedData.h"

// Biến PID & Sensor
float error = 0, previous_error = 0;
float integral = 0;
int mask = 0;
int currentSpeedL = 0;
int currentSpeedR = 0;

// Mapping sensor pins
const int sensor_pins[NUM_SENSORS] = {SENSOR_1, SENSOR_2, SENSOR_3, SENSOR_4, SENSOR_5};

void setupMotors() {
    // Cấu hình toàn bộ chân điều khiển motor là OUTPUT
    pinMode(MOTOR_R_IN1, OUTPUT);
    pinMode(MOTOR_R_IN2, OUTPUT);
    pinMode(MOTOR_R_PWM, OUTPUT);

    pinMode(MOTOR_L_IN1, OUTPUT);
    pinMode(MOTOR_L_IN2, OUTPUT);
    pinMode(MOTOR_L_PWM, OUTPUT);
}

void setupSensors() {
    for (int i = 0; i < NUM_SENSORS; i++) {
        pinMode(sensor_pins[i], INPUT);
    }
}

// Hàm điều khiển motor an toàn (Đã fix lỗi GPIO error)
void setMotorSpeed(int targetL, int targetR) {
    // Giới hạn target
    targetL = constrain(targetL, -255, 255);
    targetR = constrain(targetR, -255, 255);

    // --- LOGIC MỚI: TĂNG TỐC TỪ TỪ (RAMPING) ---
    // Thay vì set cái rụp, ta nhích từ từ mỗi lần gọi hàm (nếu thay đổi quá lớn)
    // Nếu chênh lệch > 20 pwm, chỉ tăng/giảm 20 đơn vị thôi
    
    if (targetL > currentSpeedL + 20) currentSpeedL += 20;
    else if (targetL < currentSpeedL - 20) currentSpeedL -= 20;
    else currentSpeedL = targetL; // Nếu gần tới rồi thì gán luôn

    if (targetR > currentSpeedR + 20) currentSpeedR += 20;
    else if (targetR < currentSpeedR - 20) currentSpeedR -= 20;
    else currentSpeedR = targetR;

    // --- GHI RA MOTOR (Dùng biến currentSpeed thay vì target) ---
    
    // Motor Trái
    if (currentSpeedL >= 0) {
        digitalWrite(MOTOR_L_IN1, LOW);
        digitalWrite(MOTOR_L_IN2, HIGH);
        analogWrite(MOTOR_L_PWM, currentSpeedL);
    } else {
        digitalWrite(MOTOR_L_IN1, HIGH);
        digitalWrite(MOTOR_L_IN2, LOW);
        analogWrite(MOTOR_L_PWM, -currentSpeedL);
    }

    // Motor Phải
    if (currentSpeedR >= 0) {
        digitalWrite(MOTOR_R_IN1, LOW);
        digitalWrite(MOTOR_R_IN2, HIGH);
        analogWrite(MOTOR_R_PWM, currentSpeedR);
    } else {
        digitalWrite(MOTOR_R_IN1, HIGH);
        digitalWrite(MOTOR_R_IN2, LOW);
        analogWrite(MOTOR_R_PWM, -currentSpeedR);
    }
}

float getSensor() {
    mask = 0;
    for (int i = 0; i < NUM_SENSORS; i++) {
        mask = (mask << 1) | digitalRead(sensor_pins[i]);
    }

    // Map bitmask sang error (Logic 0: Line, 1: Nền)
    switch (mask) {
        case 0b11110: error = -4; break; 
        case 0b11100: error = -3; break;
        case 0b11101: error = -2; break;
        case 0b11001: error = -1; break;
        case 0b11011: error = 0;  break; // Giữa
        case 0b10011: error = 1;  break;
        case 0b10111: error = 2;  break;
        case 0b00111: error = 3;  break;
        case 0b01111: error = 4;  break; 
        case 0b11111: 
            if (error > 0) error = 5; else error = -5; // Mất line
            break;
        case 0b00000: error = 0; break; // Ngã tư
        default: break; 
    }
    return error;
}

float calculatePID() {
    integral += error;
    integral = constrain(integral, -100, 100); 
    float pid = (PID_KP * error) + (PID_KD * (error - previous_error)) + (PID_KI * integral);
    previous_error = error;
    return pid;
}

void turnLeftSmart() {
    Serial.println(">>> SMART TURN LEFT");
    setMotorSpeed(0, 0);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Đi thẳng chút xíu để lọt tâm xe
    setMotorSpeed(100, 100); 
    vTaskDelay(pdMS_TO_TICKS(200)); 

    // Quay trái tại chỗ
    setMotorSpeed(-TURN_SPEED_HIGH, TURN_SPEED_HIGH);
    vTaskDelay(pdMS_TO_TICKS(150)); // Thoát line cũ

    long startTime = millis();
    while (true) {
        if (digitalRead(SENSOR_3) == 0) { // Mắt giữa gặp line
            setMotorSpeed(-50, 50); // Phanh nghịch
            vTaskDelay(pdMS_TO_TICKS(50));
            setMotorSpeed(0, 0);
            break;
        }
        if (millis() - startTime > 2000) break;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    error = 0; integral = 0;
}

void turnRightSmart() {
    Serial.println(">>> SMART TURN RIGHT");
    setMotorSpeed(0, 0);
    vTaskDelay(pdMS_TO_TICKS(100));

    setMotorSpeed(100, 100);
    vTaskDelay(pdMS_TO_TICKS(200));

    // Quay phải tại chỗ
    setMotorSpeed(TURN_SPEED_HIGH, -TURN_SPEED_HIGH);
    vTaskDelay(pdMS_TO_TICKS(150)); 

    long startTime = millis();
    while (true) {
        if (digitalRead(SENSOR_3) == 0) {
            setMotorSpeed(50, -50); 
            vTaskDelay(pdMS_TO_TICKS(50));
            setMotorSpeed(0, 0);
            break;
        }
        if (millis() - startTime > 2000) break;
        vTaskDelay(pdMS_TO_TICKS(5)); 
    }
    error = 0; integral = 0;
}

void TaskLine(void *pvParameters) {
    setupMotors();
    setupSensors();
    
    // Log xác nhận khởi động
    Serial.println("[LineTask] Started. Waiting for command...");
    vTaskDelay(pdMS_TO_TICKS(1000));

    for (;;) {
        // --- [DEBUG SENSOR] ---
        // In ra giá trị 5 cảm biến mỗi 200ms
        // Format: [1] [2] [3] [4] [5] (VD: 1 1 0 1 1)
        // static unsigned long lastDebugTime = 0;
        // if (millis() - lastDebugTime > 200) { 
        //     lastDebugTime = millis();
        //     Serial.printf("Sensors (1-5): %d %d %d %d %d\n", 
        //         digitalRead(SENSOR_1), 
        //         digitalRead(SENSOR_2), 
        //         digitalRead(SENSOR_3), 
        //         digitalRead(SENSOR_4), 
        //         digitalRead(SENSOR_5));
        // }
        // 1. Dừng
        if (currentCommand == CMD_STOP) {
            setMotorSpeed(0, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // 2. Quay
        if (currentCommand == CMD_TURN_LEFT) {
            turnLeftSmart();
            currentCommand = CMD_FORWARD; 
        }
        else if (currentCommand == CMD_TURN_RIGHT) {
            turnRightSmart();
            currentCommand = CMD_FORWARD; 
        }
        
        // 3. Chạy thẳng PID
        if (currentCommand == CMD_FORWARD) {
            getSensor(); 
            float pid_value = calculatePID();
            int leftSpeed = START_SPEED - pid_value; 
           int rightSpeed = START_SPEED + pid_value;
            leftSpeed = constrain(leftSpeed, 0, MAX_SPEED);
            rightSpeed = constrain(rightSpeed, 0, MAX_SPEED);
            
            setMotorSpeed(leftSpeed, rightSpeed);

            // Debug tạm thời (Xóa sau khi xe chạy ok)
            // Serial.printf("Cmd:Fwd | L:%d R:%d | Err:%.1f\n", leftSpeed, rightSpeed, error);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}