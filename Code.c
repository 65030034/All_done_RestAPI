#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ModbusMaster.h>

// Wi-Fi credentials
const char* ssid = "VisawaHouse_2.4G";
const char* password = "mag0859603510";

// Configuration for API
const char* siteID = "KMb827eb3fe41f";
const int deviceID = 4;
const char* BEARIOT_IP = "192.168.1.34";
const int BEARIOT_PORT = 3300;
String API_ENDPOINT = "http://" + String(BEARIOT_IP) + ":" + String(BEARIOT_PORT) + "/api/interfaces/update";

// LDR Pin
const int LDR_PIN = 34;

// RS485 Modbus Pins
#define MAX485_DE 5
#define MAX485_RE 18

static uint8_t xymdSlaveAddr = 0x01;
HardwareSerial chat(1);
ModbusMaster node;

// Sensor Values
int ldrValue = 0;
int adjustedLdrValue = 0;
float temperature = 0;
float humidity = 0;

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.println("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void preTransmission() {
  digitalWrite(MAX485_RE, HIGH);
  digitalWrite(MAX485_DE, HIGH);
  delay(1);
}

void postTransmission() {
  delay(3);
  digitalWrite(MAX485_RE, LOW);
  digitalWrite(MAX485_DE, LOW);
}

void readLDR() {
  ldrValue = analogRead(LDR_PIN);
  adjustedLdrValue = 4095 - ldrValue;
  Serial.print(" | Adjusted Value (Light Intensity): ");
  Serial.println(adjustedLdrValue);
}

void readModbusSensor() {
  uint8_t result;
  
  result = node.readInputRegisters(0x0001, 1);
  if (result == node.ku8MBSuccess) {
    temperature = node.getResponseBuffer(0) / 100.0;
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
  } else {
    Serial.println("Failed to read Temperature");
  }

  result = node.readInputRegisters(0x0002, 1);
  if (result == node.ku8MBSuccess) {
    humidity = node.getResponseBuffer(0) / 100.0;
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  } else {
    Serial.println("Failed to read Humidity");
  }
}

String generatePayload(int ldrValue, float temperature, float humidity) {
  StaticJsonDocument<200> doc;
  String isoDate = "2024-06-17T10:00:00Z";
  doc["siteID"] = siteID;
  doc["deviceID"] = deviceID;
  doc["date"] = isoDate;
  doc["offset"] = 0;
  doc["connection"] = "REST";

  JsonArray tagObj = doc.createNestedArray("tagObj");
  
  JsonObject obj1 = tagObj.createNestedObject();
  obj1["status"] = true;
  obj1["label"] = "ldr_sensor";
  obj1["value"] = ldrValue;
  obj1["record"] = true;
  obj1["update"] = "All";

  JsonObject obj2 = tagObj.createNestedObject();
  obj2["status"] = true;
  obj2["label"] = "temperature";
  obj2["value"] = temperature;
  obj2["record"] = true;
  obj2["update"] = "All";

  JsonObject obj3 = tagObj.createNestedObject();
  obj3["status"] = true;
  obj3["label"] = "humid";
  obj3["value"] = humidity;
  obj3["record"] = true;
  obj3["update"] = "All";

  String payload;
  serializeJson(doc, payload);
  return payload;
}

void sendData(String payload) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(API_ENDPOINT);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(payload);
    if (httpResponseCode == 200) {
      Serial.println("Data sent successfully!");
    } else {
      Serial.print("Failed to send data. HTTP response code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Error: Not connected to Wi-Fi");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LDR_PIN, INPUT);
  pinMode(MAX485_RE, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MAX485_RE, LOW);
  digitalWrite(MAX485_DE, LOW);

  chat.begin(9600, SERIAL_8N1, 16, 17);
  node.begin(xymdSlaveAddr, chat);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  connectWiFi();
}

void loop() {
  readLDR();
  readModbusSensor();
  
  String payload = generatePayload(adjustedLdrValue, temperature, humidity);
  sendData(payload);
  delay(2000);
}
