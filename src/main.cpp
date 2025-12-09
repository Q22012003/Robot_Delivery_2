#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 

#include "Certificates.h"
#include "Config.h"

// --- Khởi tạo đối tượng ---
WiFiClientSecure net;
PubSubClient client(net);
HardwareSerial SerialGM65(2);
long lastReconnectAttempt = 0;

// --- BIẾN TOÀN CỤC CHO LOGIC HANDSHAKE ---
String currentTarget = ""; // Lưu điểm đích backend gửi (VD: "1,2")
bool isMoving = false;     // Trạng thái đang thực hiện lệnh đi tới điểm

// ==========================================================
// HÀM: GỬI TRẠNG THÁI OK
// ==========================================================
void publishOk(String pos) {
    // JSON: { "status": "OK", ... } để Backend biết đã đến nơi
    String payload = "{\"device_id\":\"" + String(MQTT_CLIENT_ID) + "\",\"position\":\"" + pos + "\",\"status\":\"OK\"}";
    
    // Gửi lên topic publish (matrix_position)
    client.publish(MQTT_TOPIC_PUBLISH, payload.c_str());
}

// ==========================================================
// HÀM: CALLBACK (NHẬN LỆNH TỪ AWS)
// ==========================================================
void callback(char* topic, byte* payload, unsigned int length) {
  // Serial.print("\n[MQTT] Msg: "); Serial.println(topic); // Debug nếu cần

  StaticJsonDocument<1024> doc; 
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  const char* type = doc["type"]; 

  // --- XỬ LÝ LỆNH TỪNG BƯỚC (STEP) ---
  if (strcmp(type, "STEP") == 0) {
    const char* target = doc["target"];
    currentTarget = String(target);
    isMoving = true;

    // --- LOG YÊU CẦU ---
    Serial.print("da nhan duoc vi tri tu AWS: ");
    Serial.println(currentTarget);
    Serial.println("dang trong qua trinh di toi diem do ...");
    
    // TODO: Viết code điều khiển motor chạy theo Line/PID tại đây
    
  } else if (strcmp(type, "CONTROL") == 0) {
    const char* cmd = doc["value"];
    Serial.print("Lệnh điều khiển: "); Serial.println(cmd);
  }
}

// ==========================================================
// HÀM: KẾT NỐI WIFI & AWS (GIỮ NGUYÊN)
// ==========================================================
void connectWiFi() {
  Serial.print("Dang ket noi WiFi: ");
  Serial.println(WIFI_SSID); 
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
    if (millis() - start > 30000) {
      WiFi.disconnect(); WiFi.begin(WIFI_SSID, WIFI_PASSWORD); start = millis();
    }
  }
  Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());
}

bool reconnectMqtt() {
  Serial.println("Dang ket noi AWS IoT...");
  net.setCACert(AWS_ROOT_CA);
  net.setCertificate(DEVICE_CERT);
  net.setPrivateKey(DEVICE_PRIVATE_KEY);
  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setCallback(callback); 
  client.setBufferSize(4096);   

  if (client.connect(MQTT_CLIENT_ID)) {
    Serial.println("AWS IoT Connected!");
    // Subscribe topic nhận lệnh
    client.subscribe(MQTT_TOPIC_SUBSCRIBE); 
    return true;
  }
  Serial.print("Failed, rc="); Serial.println(client.state());
  return false;
}

void publishData(String data) {
  if (!client.connected()) return;
  // Gửi vị trí thông thường (không có status OK)
  String payload = "{\"device_id\":\"" + String(MQTT_CLIENT_ID) + "\",\"position\":\"" + data + "\"}";
  client.publish(MQTT_TOPIC_PUBLISH, payload.c_str());
}

// ==========================================================
// SETUP & LOOP
// ==========================================================
void setup() {
  Serial.begin(115200);
  SerialGM65.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(100);
  Serial.println("\n=== SYSTEM START ===");
  connectWiFi();
  net.setCACert(AWS_ROOT_CA);
  net.setCertificate(DEVICE_CERT);
  net.setPrivateKey(DEVICE_PRIVATE_KEY);
  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setCallback(callback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      if (reconnectMqtt()) lastReconnectAttempt = 0;
    }
  } else {
    client.loop();
  }

  // 3. Đọc Barcode
  if (SerialGM65.available()) {
    String barcode = SerialGM65.readStringUntil('\n');
    barcode.trim(); 
    
    if (barcode.length() > 0) {
      // Logic kiểm tra xem có đúng điểm đích không
      if (isMoving && barcode == currentTarget) {
          // --- LOG YÊU CẦU ---
          Serial.println("da den duoc diem do");
          Serial.println("Ok");

          // Gửi OK lên AWS
          publishOk(barcode);

          // Reset trạng thái
          isMoving = false;
          currentTarget = "";
      } else {
          // Quét được QR khác hoặc đang đi tự do -> cập nhật vị trí lên web thôi
          Serial.println("GM65 Scanned: " + barcode);
          publishData(barcode);
      }
    }
  }
  delay(10); 
}