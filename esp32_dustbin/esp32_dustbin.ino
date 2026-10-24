/*
 * BinPulse — ESP32 firmware for a segregation dustbin
 * ---------------------------------------------------
 * Reads an ultrasonic sensor (HC-SR04) per bin, computes fill %,
 * and publishes all bins as one JSON message over MQTT.
 * Pair it with the BinPulse web dashboard (index.html).
 *
 * Board:  ESP32 Dev Module  (Arduino IDE → Boards Manager → "esp32")
 * Library: PubSubClient by Nick O'Leary (Library Manager → "PubSubClient")
 *
 * Dashboard must use the SAME broker host and Device ID as below.
 */

#include <WiFi.h>
#include <PubSubClient.h>

// ==================== USER CONFIG ====================
const char* WIFI_SSID = "YourWiFiName";
const char* WIFI_PASS = "YourWiFiPassword";

const char* MQTT_HOST = "broker.emqx.io";   // free public broker — must match dashboard
const int   MQTT_PORT = 1883;
const char* DEVICE_ID = "bin01";            // must match Device ID in the dashboard

// One HC-SR04 per bin, mounted at the top looking down:
//   name (also the JSON key), TRIG pin, ECHO pin, bin height in cm.
// The dashboard auto-shows a card for every name here.
struct Bin { const char* name; int trig; int echo; float heightCm; };
Bin bins[] = {
  { "wet",    5, 18, 30.0 },
  { "dry",   19, 21, 30.0 },
  { "metal", 22, 23, 30.0 },
  // add more bins here, e.g. { "glass", 32, 33, 30.0 },
};
const int NUM_BINS = sizeof(bins) / sizeof(bins[0]);

const float SENSOR_GAP_CM = 4.0;        // sensor reading when bin is 100% full
const unsigned long PUBLISH_MS = 5000;  // publish every 5 s

const int BUZZER_PIN = -1;              // e.g. 4 to enable a local buzzer, -1 to disable
const int FULL_PCT   = 80;              // buzzer fires when any bin is at/above this %
// =====================================================

WiFiClient net;
PubSubClient mqtt(net);
unsigned long lastPublish = 0;

String topicFor(const char* leaf) { return String("dustbin/") + DEVICE_ID + "/" + leaf; }

float readDistanceCm(int trig, int echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(4);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long us = pulseIn(echo, HIGH, 25000UL);  // 25 ms timeout ≈ 4 m; 0 = no echo
  if (us <= 0) return -1.0;
  return us * 0.0343f / 2.0f;
}

// Median of 3 readings → one bad echo won't spike the value.
int fillPercent(const Bin& b) {
  float v[3]; int n = 0;
  for (int i = 0; i < 3; i++) {
    float d = readDistanceCm(b.trig, b.echo);
    if (d > 0 && d < 200) v[n++] = d;   // ignore timeouts / wild values
    delay(30);
  }
  if (n == 0) return -1;
  for (int i = 0; i < n; i++)
    for (int j = i + 1; j < n; j++)
      if (v[j] < v[i]) { float t = v[i]; v[i] = v[j]; v[j] = t; }
  float dist = v[n / 2];
  float pct = (b.heightCm - dist) / (b.heightCm - SENSOR_GAP_CM) * 100.0f;
  return (int)constrain(pct, 0.0f, 100.0f);
}

void publishState() {
  String json = "{";
  bool anyFull = false;
  for (int i = 0; i < NUM_BINS; i++) {
    int pct = fillPercent(bins[i]);
    if (i) json += ",";
    json += "\"" + String(bins[i].name) + "\":" + String(pct < 0 ? -1 : pct);
    if (pct >= FULL_PCT) anyFull = true;
  }
  json += "}";
  mqtt.publish(topicFor("state").c_str(), json.c_str(), true);  // retained
  Serial.println("Published: " + json);

  if (anyFull && BUZZER_PIN >= 0) {
    for (int i = 0; i < 3; i++) {           // short beep-beep-beep
      digitalWrite(BUZZER_PIN, HIGH); delay(90);
      digitalWrite(BUZZER_PIN, LOW);  delay(90);
    }
  }
}

void onCmd(char* topic, byte* payload, unsigned int len) {
  String msg;
  for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];
  Serial.println("CMD: " + msg);
  if (msg.indexOf("ping") >= 0) publishState();  // dashboard "Ping device" button
}

void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("WiFi connecting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(400); Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED
    ? "\nWiFi OK, IP: " + WiFi.localIP().toString()
    : "\nWiFi failed, retrying next loop");
}

void ensureMqtt() {
  if (mqtt.connected()) return;
  Serial.print("MQTT connecting");
  String cid = "esp32-" + String(DEVICE_ID);
  String willTopic = topicFor("status");
  // Last Will: broker marks the device "offline" automatically if it drops
  bool ok = mqtt.connect(cid.c_str(), willTopic.c_str(), 0, true, "offline");
  if (ok) {
    mqtt.publish(topicFor("status").c_str(), "online", true);  // retained
    mqtt.subscribe(topicFor("cmd").c_str());
    Serial.println(" OK");
    publishState();  // send data right away
  } else {
    Serial.printf(" failed, rc=%d (retry next loop)\n", mqtt.state());
    delay(500);
  }
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < NUM_BINS; i++) {
    pinMode(bins[i].trig, OUTPUT);
    pinMode(bins[i].echo, INPUT);
    digitalWrite(bins[i].trig, LOW);
  }
  if (BUZZER_PIN >= 0) pinMode(BUZZER_PIN, OUTPUT);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onCmd);
  mqtt.setKeepAlive(30);

  Serial.println("\nBinPulse ESP32 starting — " + String(NUM_BINS) + " bins");
}

void loop() {
  ensureWifi();
  if (WiFi.status() == WL_CONNECTED) {
    ensureMqtt();
    mqtt.loop();
    if (mqtt.connected() && millis() - lastPublish >= PUBLISH_MS) {
      lastPublish = millis();
      publishState();
    }
  }
}
