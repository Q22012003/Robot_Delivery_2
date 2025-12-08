#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// Nhúng 2 file cấu hình vào
#include "Certificates.h"
#include "Config.h"

// --- Khởi tạo đối tượng ---
WiFiClientSecure net;
PubSubClient client(net);
HardwareSerial SerialGM65(2);
long lastReconnectAttempt = 0;

// ==========================================================
// HÀM: CALLBACK (XỬ LÝ KHI CÓ DATA TỪ AWS GỬI VỀ)
// ==========================================================
// Hàm này sẽ chạy khi ESP32 nhận được tin nhắn từ Topic đã đăng ký
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("\n[MQTT] Message arrived on topic: ");
  Serial.println(topic);

  Serial.print("[MQTT] Content: ");
  // In nội dung tin nhắn ra Serial
  String message = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  Serial.println();
  
  // Ví dụ: Nếu muốn xử lý chuỗi message sau này thì code ở đây
  // if (message == "OPEN") { ... }
}

// ==========================================================
// HÀM: KẾT NỐI WIFI
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
      Serial.println("\nWiFi Timeout. Reconnecting...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      start = millis();
    }
  }
  Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());
}

// ==========================================================
// HÀM: KẾT NỐI AWS IOT
// ==========================================================
bool reconnectMqtt() {
  Serial.println("Dang ket noi AWS IoT...");
  
  // Nạp chứng chỉ
  net.setCACert(AWS_ROOT_CA);
  net.setCertificate(DEVICE_CERT);
  net.setPrivateKey(DEVICE_PRIVATE_KEY);
  
  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setCallback(callback); // <-- QUAN TRỌNG: Đăng ký hàm nhận dữ liệu
  client.setBufferSize(4096); 

  if (client.connect(MQTT_CLIENT_ID)) {
    Serial.println("AWS IoT Connected!");
    
    // <-- QUAN TRỌNG: Đăng ký lắng nghe Topic
    if(client.subscribe(MQTT_TOPIC_SUBSCRIBE)) {
        Serial.println("Subscribed to: " + String(MQTT_TOPIC_SUBSCRIBE));
    } else {
        Serial.println("Subscribe Failed!");
    }
    
    return true;
  }
  
  Serial.print("Failed, rc=");
  Serial.println(client.state());
  return false;
}

// ==========================================================
// HÀM: GỬI DỮ LIỆU
// ==========================================================
void publishData(String data) {
  if (!client.connected()) return;
  
  String payload = "{\"device_id\":\"" + String(MQTT_CLIENT_ID) + "\",\"position\":\"" + data + "\"}";
  Serial.println("Publishing: " + payload);
  
  if (client.publish(MQTT_TOPIC_PUBLISH, payload.c_str())) {
    Serial.println("-> Success");
  } else {
    Serial.println("-> Fail");
  }
}

// ==========================================================
// SETUP
// ==========================================================
void setup() {
  Serial.begin(115200);
  
  // Khởi tạo GM65
  SerialGM65.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
  
  delay(100);
  Serial.println("\n=== SYSTEM START ===");
  
  connectWiFi();
  
  // Setup ban đầu cho Client
  net.setCACert(AWS_ROOT_CA);
  net.setCertificate(DEVICE_CERT);
  net.setPrivateKey(DEVICE_PRIVATE_KEY);
  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setCallback(callback); // Đăng ký callback ngay từ setup
}

// ==========================================================
// LOOP
// ==========================================================
void loop() {
  // 1. Kiểm tra WiFi
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // 2. Kiểm tra MQTT
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      if (reconnectMqtt()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    client.loop(); // <-- Hàm này cực quan trọng để nhận dữ liệu về
  }

  // 3. Đọc Barcode từ GM65
  if (SerialGM65.available()) {
    String barcode = SerialGM65.readStringUntil('\n');
    barcode.trim(); 
    
    if (barcode.length() > 0) {
      Serial.println("GM65 Scanned: " + barcode);
      publishData(barcode);
    }
  }
  
  delay(10); 
}