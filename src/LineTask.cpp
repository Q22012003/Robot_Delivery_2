#include <Arduino.h>
#include "LineTask.h"
#include "Config.h"
#include "SharedData.h"
#include "DebuggerBLE.h"
#include <math.h>   // thêm dòng này nếu chưa có (để dùng fabs)
// Chỉ có 6 mắt
const int sensor_pins[NUM_SENSORS] = {SENSOR_0, SENSOR_1, SENSOR_2, SENSOR_3, SENSOR_4, SENSOR_5};

// Mảng lưu giá trị Calibration
int black_value[NUM_SENSORS];
int white_value[NUM_SENSORS];

// Biến PID
float lastPos = 0;
int servoPwm = 0;
float integral = 0;
// ===== FORWARD BLIND (GO STRAIGHT 1.5s) =====
bool forwardBlind = false;
unsigned long forwardBlindStartMs = 0;
const unsigned long FORWARD_BLIND_MS = 1500;

// ===== TURN APPROACH + ROTATE (no blind after turn) =====
const unsigned long TURN_STRAIGHT_MS = 750;   // đi thẳng trước khi quay
const unsigned long TURN_ROTATE_MS   = 1500;  // thời gian quay 90 độ (tùy robot)
const int TURN_STRAIGHT_SPEED = BASE_SPEED;   // tốc độ đi thẳng trước khi quay

bool forwardBlindDone = false;             // chỉ cho blind-forward chạy 1 lần cho mỗi "lần nhận lệnh FORWARD"
RobotCommand lastCmdSeen = CMD_STOP;       // để bắt cạnh (command change)

int readSensorAnalog(int index);
// ===== DEBUG BACK / REACQUIRE LOGGING =====
static inline const char* cmdName(RobotCommand c) {
  switch (c) {
    case CMD_STOP: return "STOP";
    case CMD_FORWARD: return "FORWARD";
    case CMD_TURN_LEFT: return "LEFT";
    case CMD_TURN_RIGHT: return "RIGHT";
    case CMD_BACK: return "BACK";
    default: return "UNKNOWN";
  }
}

static void logSensorsMapped(const char* tag) {
  // in giá trị mapped 0..1000 (đang dùng readSensorAnalog)
  Serial.printf("[%lu] %s S=", millis(), tag);
  for (int i = 0; i < NUM_SENSORS; i++) {
    int v = readSensorAnalog(i);
    Serial.printf("%d%s", v, (i == NUM_SENSORS - 1) ? "" : ",");
  }
  Serial.println();
}

void setupMotors() {
    pinMode(PIN_LDIR, OUTPUT); pinMode(PIN_IN4_LEFT, OUTPUT); pinMode(PIN_LPWM, OUTPUT);
    pinMode(PIN_RDIR, OUTPUT); pinMode(PIN_IN2_RIGHT, OUTPUT); pinMode(PIN_RPWM, OUTPUT);
    digitalWrite(PIN_LDIR, LOW); digitalWrite(PIN_IN4_LEFT, LOW);
    digitalWrite(PIN_RDIR, LOW); digitalWrite(PIN_IN2_RIGHT, LOW);
}

void setupSensors() {
    // Cấu hình ADC cho Analog thật
    analogReadResolution(10);       
    analogSetAttenuation(ADC_11db); 
    
    for (int i = 0; i < NUM_SENSORS; i++) {
        pinMode(sensor_pins[i], INPUT);
    }
}

void speed_run(int speedDC_left, int speedDC_right) {
    speedDC_left = constrain(speedDC_left, -MAX_SPEED, MAX_SPEED);
    speedDC_right = constrain(speedDC_right, -MAX_SPEED, MAX_SPEED);
    
    int pwm_left = abs(speedDC_left);
    int pwm_right = abs(speedDC_right);

    if (speedDC_left < 0) {
        digitalWrite(PIN_LDIR, LOW); digitalWrite(PIN_IN4_LEFT, HIGH); analogWrite(PIN_LPWM, pwm_left);
    } else {
        digitalWrite(PIN_LDIR, HIGH); digitalWrite(PIN_IN4_LEFT, LOW); analogWrite(PIN_LPWM, pwm_left);
    }

    if (speedDC_right < 0) {
        digitalWrite(PIN_RDIR, LOW); digitalWrite(PIN_IN2_RIGHT, HIGH); analogWrite(PIN_RPWM, pwm_right);
    } else {
        digitalWrite(PIN_RDIR, HIGH); digitalWrite(PIN_IN2_RIGHT, LOW); analogWrite(PIN_RPWM, pwm_right);
    }
}

// Hàm đọc Analog (Không cần Hybrid nữa vì cả 6 chân đều an toàn)
int readSensorAnalog(int i) {
    int val = 1023 - analogRead(sensor_pins[i]); // Đảo ngược giá trị
    val = constrain(val, black_value[i], white_value[i]);
    return map(val, black_value[i], white_value[i], 0, 1000);
}

void calibrateSensors() {
    // [SỬA] Đổi thông báo thành 10s
    debug_println(">>> CALIBRATING 6 EYES (10s) - Do not connect WiFi yet...");
    
    for(int i=0; i<NUM_SENSORS; i++) {
        black_value[i] = 1023; white_value[i] = 0;   
    }
    
    unsigned long start = millis();
    // [SỬA] Tăng thời gian lên 10000ms (10 giây)
    while (millis() - start < 10000) {
        for (int i = 0; i < NUM_SENSORS; i++) {
            int val = 1023 - analogRead(sensor_pins[i]);
            if (val < black_value[i]) black_value[i] = val;
            if (val > white_value[i]) white_value[i] = val;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    debug_println(">>> CALIBRATION DONE. Allowing WiFi connection now.");
}

void calculatePID() {
    long avg = 0;
    long sum = 0;
    int val;
    bool onLine = false;

    for (int i = 0; i < NUM_SENSORS; i++) {
        val = readSensorAnalog(i);
        if (val > 200) onLine = true; 

        // Trọng số vị trí: 0, 1000, 2000, 3000, 4000, 5000
        avg += (long)val * (i * 1000);
        sum += val;
    }

    float position;
    if (!onLine) {
        // Mất line: giữ hướng cũ
        // Vì max là 5000, nên nếu lệch phải thì gán 5000, trái gán 0
        if (lastPos < 0) position = 0; 
        else position = 5000;
    } else {
        position = (float)avg / sum;
    }

    // TÂM XE MỚI: (0 + 5000) / 2 = 2500
    float error = position - 2500;

    integral += error;
    integral = constrain(integral, -3000, 3000);

    float P = error * PID_KP;
    float I = integral * PID_KI;
    float D = (error - lastPos) * PID_KD;

    float out = P + I + D;
    out = constrain(out, -4000, 4000); 
    
    // Scale về PWM: Chia 20 để mượt mà
    servoPwm = (int)(out / 20); 

    lastPos = error;
}

// ===== [ADD] Read line error without PID (for REACQUIRE) =====
static bool readLineError(float &errorOut) {
    long avg = 0;
    long sum = 0;
    int val;
    bool onLine = false;

    for (int i = 0; i < NUM_SENSORS; i++) {
        val = readSensorAnalog(i);
        if (val > 200) onLine = true;

        avg += (long)val * (i * 1000);
        sum += val;
    }

    float position;
    if (!onLine || sum == 0) {
        // fallback theo hướng cũ
        position = (lastPos < 0) ? 0 : 5000;
    } else {
        position = (float)avg / (float)sum;
    }

    errorOut = position - 2500; // tâm 6 mắt = 2500
    return onLine;
}

void TaskLine(void *pvParameters) {
    setupMotors();
    setupSensors();
    debug_println("[LineTask] Started 6-EYES ANALOG Mode.");

    calibrateSensors();
    debug_println("[LineTask] Ready.");

    // báo cho NetworkTask biết đã calib xong
    isCalibrated = true;
    debug_println("[LineTask] Ready. Waiting for command...");

    // BACK "tà đạo"
    const int BACK_SPIN_MS = 2000;   // quay tại chỗ 2s
    const int BACK_STOP_MS = 120;    // dừng cho hết trớn

    for (;;) {
         // ===== EDGE DETECT COMMAND CHANGE =====
        if (currentCommand != lastCmdSeen) {
            RobotCommand prev = lastCmdSeen;

            // rời khỏi FORWARD -> lần sau quay lại FORWARD sẽ được phép blind lại 1 lần
            if (prev == CMD_FORWARD && currentCommand != CMD_FORWARD) {
                forwardBlindDone = false;
            }

            // vừa nhận FORWARD (cạnh lên):
            // - bình thường: cho phép blind 1 lần
            // - nếu vừa từ TURN/BACK chuyển sang FORWARD: bỏ qua blind, vào PID ngay
            if (currentCommand == CMD_FORWARD) {
                forwardBlindDone = (prev == CMD_TURN_LEFT || prev == CMD_TURN_RIGHT || prev == CMD_BACK);
            }

            lastCmdSeen = currentCommand;
        }
        // STOP
        if (currentCommand == CMD_STOP) {
            speed_run(0, 0);

            // nếu có cơ chế align heading thì giữ nguyên
            if (needHeadingAlign) {
                while (currentHeading != desiredHeading) {
                    speed_run(TURN_SPEED, -TURN_SPEED);
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    speed_run(0, 0);
                    vTaskDelay(pdMS_TO_TICKS(80));
                    currentHeading = (RobotHeading)((currentHeading + 1) % 4);
                }
                needHeadingAlign = false;
            }

            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

                 // ===== KHI VỪA NHẬN CMD_FORWARD -> chạy thẳng 1.5s =====
         if (currentCommand == CMD_FORWARD && !forwardBlind && !runBlind && !forwardBlindDone) {
            forwardBlind = true;
            forwardBlindStartMs = millis();
            forwardBlindDone = true;

             // reset PID để tránh giật
              lastPos = 0;
              integral = 0;
              servoPwm = 0;

              Serial.printf("[%lu] FORWARD: blind straight %lu ms\n",
                   millis(), FORWARD_BLIND_MS);
         }


        // ===== BLIND ACTIONS (LEFT/RIGHT/BACK) =====
        if (runBlind) {

            if (currentCommand == CMD_TURN_LEFT) {
                // đi thẳng 0.5s rồi mới quay 90 độ -> giúp xe vào đúng line/ngã rẽ
                speed_run(BASE_SPEED_BLIND, BASE_SPEED_BLIND);
                vTaskDelay(pdMS_TO_TICKS(TURN_STRAIGHT_MS));

                // quay 90 độ (delay)
                speed_run(-TURN_SPEED, TURN_SPEED);
                vTaskDelay(pdMS_TO_TICKS(TURN_ROTATE_MS));

                speed_run(0, 0);
                vTaskDelay(pdMS_TO_TICKS(80));

                runBlind = false;
                currentCommand = CMD_FORWARD;     // vào PID ngay, KHÔNG đi mù
                lastPos = 0; integral = 0; servoPwm = 0;
            }
            else if (currentCommand == CMD_TURN_RIGHT) {
                // đi thẳng 0.5s rồi mới quay 90 độ -> giúp xe vào đúng line/ngã rẽ
                speed_run(BASE_SPEED_BLIND, BASE_SPEED_BLIND);
                vTaskDelay(pdMS_TO_TICKS(TURN_STRAIGHT_MS));

                // quay 90 độ (delay)
                speed_run(TURN_SPEED, -TURN_SPEED);
                vTaskDelay(pdMS_TO_TICKS(TURN_ROTATE_MS));

                speed_run(0, 0);
                vTaskDelay(pdMS_TO_TICKS(80));

                runBlind = false;
                currentCommand = CMD_FORWARD;     // vào PID ngay, KHÔNG đi mù
                lastPos = 0; integral = 0; servoPwm = 0;
            }
            else if (currentCommand == CMD_BACK) {
                // "tà đạo": quay tại chỗ 2s, không REACQUIRE, không lineHoldOff
                Serial.printf("[%lu] BACK: spin %d ms then resume PID\n", millis(), BACK_SPIN_MS);
                logSensorsMapped("BACK_ENTER_SENS");

                speed_run(80, -80);
                vTaskDelay(pdMS_TO_TICKS(BACK_SPIN_MS));

                speed_run(0, 0);
                vTaskDelay(pdMS_TO_TICKS(BACK_STOP_MS));

                logSensorsMapped("BACK_AFTER_SPIN_SENS");

                runBlind = false;
                currentCommand = CMD_FORWARD;

                lastPos = 0; integral = 0; servoPwm = 0;
            }
            else if (currentCommand == CMD_FORWARD) {
                // bạn đã muốn bỏ blind-forward -> vào dò line ngay
                debug_println("(currentCommand == CMD_FORWARD 2s foward");
                speed_run(70, 70);
                vTaskDelay(pdMS_TO_TICKS(BACK_SPIN_MS));
                debug_println("(currentCommand == CMD_FORWARD speed_run(0,0)");
                speed_run(0, 0);
                runBlind = false;
                lastPos = 0; integral = 0; servoPwm = 0;
            }
            else {
                speed_run(0, 0);
                runBlind = false;
            }

            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
if (forwardBlind) {
    speed_run(BASE_SPEED, BASE_SPEED);

    if (millis() - forwardBlindStartMs >= FORWARD_BLIND_MS) {
        forwardBlind = false;

        lastPos = 0;
        integral = 0;
        servoPwm = 0;

        Serial.printf("[%lu] FORWARD: blind done -> enable line follow\n", millis());
    }

    vTaskDelay(pdMS_TO_TICKS(10));
    continue;
}
        // ===== NORMAL LINE FOLLOW (PID) =====
        if (currentCommand == CMD_FORWARD) {
            calculatePID();
            int speedLeft  = BASE_SPEED + servoPwm;
            int speedRight = BASE_SPEED - servoPwm;
            speed_run(speedLeft, speedRight);
        } else {
            // lệnh khác mà không runBlind -> dừng an toàn
            speed_run(0, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
