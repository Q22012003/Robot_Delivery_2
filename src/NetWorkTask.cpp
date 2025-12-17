// NetworkTask.cpp
#include "NetworkTask.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // [CẦN CÀI THƯ VIỆN NÀY]
#include "Config.h"
#include "Certificates.h"
#include "SharedData.h"

WiFiClientSecure net;
PubSubClient client(net);

// Hàm xử lý dữ liệu nhận từ AWS (Server gửi xuống)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.print("[MQTT] Received: ");
    Serial.println(message);

    // [THÊM] Parse JSON để lấy Target
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (!error) {
        const char* type = doc["type"];
        if (strcmp(type, "STEP") == 0) {
            const char* target = doc["target"];
            const char* dir = doc["direction"];
            
            // Lưu target vào biến chung để check
            strncpy(currentTarget, target, sizeof(currentTarget) - 1);
            currentTarget[sizeof(currentTarget) - 1] = '\0';
            
            Serial.printf(">>> UPDATED TARGET: %s | DIRECTION: %s\n", currentTarget, dir);
        }
        else if (strcmp(type, "STOP") == 0) {
             strcpy(currentTarget, ""); // Xóa target
             Serial.println(">>> STOP COMMAND RECEIVED");
        }
    } else {
        Serial.println("Error parsing JSON");
    }
}

void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;
    Serial.print("Connecting WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected");
}

void connectAWS() {
    net.setCACert(AWS_ROOT_CA);
    net.setCertificate(DEVICE_CERT);
    net.setPrivateKey(DEVICE_PRIVATE_KEY);

    client.setServer(AWS_IOT_ENDPOINT, 8883);
    client.setCallback(mqttCallback);

    Serial.print("Connecting AWS...");
    while (!client.connected()) {
        if (client.connect(MQTT_CLIENT_ID)) {
            Serial.println("AWS Connected!");
            client.subscribe(MQTT_TOPIC_SUBSCRIBE);
        } else {
            Serial.print(".");
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}

void TaskNetwork(void *pvParameters) {
    connectWiFi();
    connectAWS();

    ScannerMessage incomingMsg;

    for (;;) {
        if (WiFi.status() != WL_CONNECTED) connectWiFi();
        if (!client.connected()) connectAWS();
        client.loop();

        // Kiểm tra dữ liệu từ Scanner
        if (xQueueReceive(scannerQueue, &incomingMsg, pdMS_TO_TICKS(10)) == pdTRUE) {
            
            String scannedPos = String(incomingMsg.barcode);
            Serial.print("[SCANNER] Read: " + scannedPos);

            // [THÊM] Logic so sánh (Chỉ để in Log cho dễ debug)
            bool isCorrect = false;
            if (strcmp(currentTarget, "") != 0) { // Nếu đang có target
                if (scannedPos.equals(String(currentTarget))) {
                    Serial.println(" -> [MATCH] ĐÚNG VỊ TRÍ!");
                    isCorrect = true;
                } else {
                    Serial.printf(" -> [WRONG] SAI VỊ TRÍ! (Target: %s)\n", currentTarget);
                }
            } else {
                Serial.println(" -> [INFO] Chưa có lệnh di chuyển.");
            }

            // Gửi lên AWS (Vẫn gửi dù sai để Server biết xe đang ở đâu)
            // Có thể thêm trường status vào đây nếu muốn
            String statusStr = isCorrect ? "OK" : "WRONG_POS";
            
            String payload = "{\"device_id\":\"" + String(MQTT_CLIENT_ID) + 
                             "\",\"position\":\"" + scannedPos + 
                             "\",\"status\":\"" + statusStr + "\"}"; // Gửi kèm trạng thái
            
            if(client.publish(MQTT_TOPIC_PUBLISH, payload.c_str())){
                 Serial.println("[NETWORK] Uploaded to Cloud successfully.");
            } else {
                 Serial.println("[NETWORK] Upload Failed.");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}