#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "ac_raw_data.h"

/*
  ESP8266 Midea AC raw IR gateway

  WiFi:
    SSID: AC_Remote_AP
    PASS: <WIFI_PASSWORD>

  IR:
    NodeMCU D2 = GPIO4
    Carrier = 38kHz

  APIs:
    /ping
    /api/state
    /api/send?cmd=cool_26
    /api/ac?power=on&mode=cool&temp=26
*/

const char* WIFI_SSID = "AC_Remote_AP";
const char* WIFI_PASSWORD = "<WIFI_PASSWORD>";


const uint16_t IR_LED_PIN = 4;  // NodeMCU D2 = GPIO4
const uint16_t IR_CARRIER_KHZ = 38;

IRsend irsend(IR_LED_PIN);
ESP8266WebServer server(80);

const bool IR_COMPENSATION_ENABLED = true;
const uint16_t MAX_RAW_LEN = 300;

const uint8_t SEND_REPEAT_COUNT = 1;
const uint16_t SEND_REPEAT_GAP_MS = 250;

/*
  这组参数是根据你这次 51 反抓结果调整的：

  当前现象�?    原始�?space �?300~350us
    ESP8266 反抓�?space �?100~200us

    原始�?space �?1400~1450us
    ESP8266 反抓�?space �?1200~1300us

  所以：
    mark 补偿略降�?    space 补偿明显增加
*/
const char* FIRMWARE_VERSION = "comp-test-20260602-06";

const uint16_t MARK_SHORT_COMP_US = 200;
const uint16_t SPACE_SHORT_COMP_US = 210;
const uint16_t SPACE_LONG_COMP_US = 200;
const uint16_t HEADER_SPACE_COMP_US = 200;
const uint16_t FRAME_GAP_SPACE_COMP_US = 100;

String lastCommand = "none";
String lastPower = "off";
String lastMode = "cool";
int lastTemp = 26;
String lastFan = "auto";
unsigned long lastSendMillis = 0;

String jsonEscape(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", "\\n");
  s.replace("\r", "\\r");
  return s;
}

void sendJson(int code, const String& body) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(code, "application/json; charset=utf-8", body);
}

void sendRawWithCompensation(const uint16_t raw[], uint16_t len) {
  static uint16_t fixedRaw[MAX_RAW_LEN];

  if (len > MAX_RAW_LEN) {
    Serial.print("Raw length too long: ");
    Serial.println(len);
    return;
  }

  for (uint16_t i = 0; i < len; i++) {
    uint16_t v = raw[i];
    bool isMark = (i % 2 == 0);

    if (IR_COMPENSATION_ENABLED) {
      if (isMark) {
        if (v < 1000) {
          v += MARK_SHORT_COMP_US;
        }
      } else {
        if (v < 1000) {
          v += SPACE_SHORT_COMP_US;
        } else if (v >= 1000 && v < 2000) {
          v += SPACE_LONG_COMP_US;
        } else if (v >= 3500 && v <= 4500) {
          v += HEADER_SPACE_COMP_US;
        } else if (v > 4500 && v <= 6000) {
          v += FRAME_GAP_SPACE_COMP_US;
        }
      }
    }

    fixedRaw[i] = v;
  }

  for (uint8_t i = 0; i < SEND_REPEAT_COUNT; i++) {
    Serial.print("Raw send repeat ");
    Serial.print(i + 1);
    Serial.print("/");
    Serial.println(SEND_REPEAT_COUNT);

    irsend.sendRaw(fixedRaw, len, IR_CARRIER_KHZ);
    delay(SEND_REPEAT_GAP_MS);
    yield();
  }
}

void sendRawCode(const uint16_t data[], uint16_t len, const char* commandName) {
  Serial.println();
  Serial.print("Sending raw IR command: ");
  Serial.print(commandName);
  Serial.print(", length = ");
  Serial.println(len);

  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);

  Serial.print("Compensation: ");
  Serial.println(IR_COMPENSATION_ENABLED ? "ON" : "OFF");

  Serial.print("Repeat count: ");
  Serial.println(SEND_REPEAT_COUNT);

  sendRawWithCompensation(data, len);

  lastCommand = String(commandName);
  lastSendMillis = millis();

  Serial.println("IR send done.");
}

bool sendCommandByName(String cmd) {
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "power_on" || cmd == "on") {
    sendRawCode(raw_power_on, raw_power_on_len, "power_on");
    lastPower = "on";
    return true;
  }

  if (cmd == "power_off" || cmd == "off") {
    sendRawCode(raw_power_off, raw_power_off_len, "power_off");
    lastPower = "off";
    return true;
  }

  if (cmd == "cool_23") {
    sendRawCode(raw_cool_23, raw_cool_23_len, "cool_23");
    lastPower = "on";
    lastMode = "cool";
    lastTemp = 23;
    return true;
  }

  if (cmd == "cool_24") {
    sendRawCode(raw_cool_24, raw_cool_24_len, "cool_24");
    lastPower = "on";
    lastMode = "cool";
    lastTemp = 24;
    return true;
  }

  if (cmd == "cool_25") {
    sendRawCode(raw_cool_25, raw_cool_25_len, "cool_25");
    lastPower = "on";
    lastMode = "cool";
    lastTemp = 25;
    return true;
  }

  if (cmd == "cool_26") {
    sendRawCode(raw_cool_26, raw_cool_26_len, "cool_26");
    lastPower = "on";
    lastMode = "cool";
    lastTemp = 26;
    return true;
  }

  if (cmd == "fan_low" || cmd == "fan_1") {
    sendRawCode(raw_fan_low, raw_fan_low_len, "fan_low");
    lastPower = "on";
    lastMode = "fan";
    lastFan = "low";
    return true;
  }

  if (cmd == "fan_mid" || cmd == "fan_2") {
    sendRawCode(raw_fan_mid, raw_fan_mid_len, "fan_mid");
    lastPower = "on";
    lastMode = "fan";
    lastFan = "mid";
    return true;
  }

  if (cmd == "fan_high" || cmd == "fan_3") {
    sendRawCode(raw_fan_high, raw_fan_high_len, "fan_high");
    lastPower = "on";
    lastMode = "fan";
    lastFan = "high";
    return true;
  }

  return false;
}

bool sendByModeTemp(String power, String mode, int temp, String fan) {
  power.trim();
  power.toLowerCase();

  mode.trim();
  mode.toLowerCase();

  fan.trim();
  fan.toLowerCase();

  if (power == "off") {
    return sendCommandByName("power_off");
  }

  if (power == "on" && mode == "cool") {
    if (temp == 23) return sendCommandByName("cool_23");
    if (temp == 24) return sendCommandByName("cool_24");
    if (temp == 25) return sendCommandByName("cool_25");
    if (temp == 26) return sendCommandByName("cool_26");
  }

  if (power == "on" && mode == "fan") {
    if (fan == "low") return sendCommandByName("fan_low");
    if (fan == "mid") return sendCommandByName("fan_mid");
    if (fan == "high") return sendCommandByName("fan_high");
  }

  if (fan == "low") return sendCommandByName("fan_low");
  if (fan == "mid") return sendCommandByName("fan_mid");
  if (fan == "high") return sendCommandByName("fan_high");

  return false;
}

void handleRoot() {
  String body;
  body.reserve(256);

  body += "{";
  body += "\"status\":\"ok\",";
  body += "\"device\":\"esp8266-midea-raw-ir-gateway\",";
  body += "\"firmware\":\"";
  body += FIRMWARE_VERSION;
  body += "\",";
  body += "\"usage\":\"/ping, /api/state, /api/send?cmd=cool_26, /api/ac?power=on&mode=cool&temp=26\"";
  body += "}";

  sendJson(200, body);
}

void handlePing() {
  String body;
  body.reserve(256);

  body += "{";
  body += "\"status\":\"ok\",";
  body += "\"device\":\"esp8266-midea-raw-ir-gateway\",";
  body += "\"firmware\":\"";
  body += FIRMWARE_VERSION;
  body += "\",";
  body += "\"ip\":\"";
  body += WiFi.localIP().toString();
  body += "\"";
  body += "}";

  sendJson(200, body);
}

void handleState() {
  String body;
  body.reserve(512);

  body += "{";
  body += "\"status\":\"ok\",";
  body += "\"device\":\"esp8266-midea-raw-ir-gateway\",";
  body += "\"firmware\":\"";
  body += FIRMWARE_VERSION;
  body += "\",";
  body += "\"ip\":\"";
  body += WiFi.localIP().toString();
  body += "\",";
  body += "\"wifi_rssi\":";
  body += String(WiFi.RSSI());
  body += ",";
  body += "\"last_command\":\"";
  body += jsonEscape(lastCommand);
  body += "\",";
  body += "\"last_power\":\"";
  body += jsonEscape(lastPower);
  body += "\",";
  body += "\"last_mode\":\"";
  body += jsonEscape(lastMode);
  body += "\",";
  body += "\"last_temp\":";
  body += String(lastTemp);
  body += ",";
  body += "\"last_fan\":\"";
  body += jsonEscape(lastFan);
  body += "\",";
  body += "\"last_send_millis\":";
  body += String(lastSendMillis);
  body += ",";
  body += "\"compensation_enabled\":";
  body += IR_COMPENSATION_ENABLED ? "true" : "false";
  body += ",";
  body += "\"mark_short_comp_us\":";
  body += String(MARK_SHORT_COMP_US);
  body += ",";
  body += "\"space_short_comp_us\":";
  body += String(SPACE_SHORT_COMP_US);
  body += ",";
  body += "\"space_long_comp_us\":";
  body += String(SPACE_LONG_COMP_US);
  body += ",";
  body += "\"header_space_comp_us\":";
  body += String(HEADER_SPACE_COMP_US);
  body += ",";
  body += "\"frame_gap_space_comp_us\":";
  body += String(FRAME_GAP_SPACE_COMP_US);
  body += ",";
  body += "\"repeat_count\":";
  body += String(SEND_REPEAT_COUNT);
  body += ",";
  body += "\"repeat_gap_ms\":";
  body += String(SEND_REPEAT_GAP_MS);
  body += "}";

  sendJson(200, body);
}

void handleSend() {
  String cmd = server.arg("cmd");
  cmd.trim();

  if (cmd == "") {
    sendJson(400, "{\"status\":\"error\",\"message\":\"missing cmd. Example: /api/send?cmd=cool_26\"}");
    return;
  }

  bool ok = sendCommandByName(cmd);

  if (!ok) {
    sendJson(400, "{\"status\":\"error\",\"message\":\"unknown cmd\"}");
    return;
  }

  String body;
  body.reserve(256);

  body += "{";
  body += "\"status\":\"ok\",";
  body += "\"firmware\":\"";
  body += FIRMWARE_VERSION;
  body += "\",";
  body += "\"cmd\":\"";
  body += jsonEscape(cmd);
  body += "\",";
  body += "\"last_command\":\"";
  body += jsonEscape(lastCommand);
  body += "\"";
  body += "}";

  sendJson(200, body);
}

void handleAc() {
  String power = server.arg("power");
  String mode = server.arg("mode");
  String tempStr = server.arg("temp");
  String fan = server.arg("fan");

  power.trim();
  mode.trim();
  tempStr.trim();
  fan.trim();

  if (power == "") power = lastPower;
  if (mode == "") mode = lastMode;
  if (fan == "") fan = lastFan;

  int temp = lastTemp;
  if (tempStr != "") {
    temp = tempStr.toInt();
  }

  bool ok = sendByModeTemp(power, mode, temp, fan);

  if (!ok) {
    sendJson(400, "{\"status\":\"error\",\"message\":\"unsupported state\"}");
    return;
  }

  String body;
  body.reserve(320);

  body += "{";
  body += "\"status\":\"ok\",";
  body += "\"firmware\":\"";
  body += FIRMWARE_VERSION;
  body += "\",";
  body += "\"power\":\"";
  body += jsonEscape(lastPower);
  body += "\",";
  body += "\"mode\":\"";
  body += jsonEscape(lastMode);
  body += "\",";
  body += "\"temp\":";
  body += String(lastTemp);
  body += ",";
  body += "\"fan\":\"";
  body += jsonEscape(lastFan);
  body += "\",";
  body += "\"last_command\":\"";
  body += jsonEscape(lastCommand);
  body += "\"";
  body += "}";

  sendJson(200, body);
}

void handleNotFound() {
  String body;
  body.reserve(256);

  body += "{";
  body += "\"status\":\"error\",";
  body += "\"message\":\"not found\",";
  body += "\"uri\":\"";
  body += jsonEscape(server.uri());
  body += "\"";
  body += "}";

  sendJson(404, body);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("ESP8266 Midea raw IR gateway starting...");
  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);

  irsend.begin();

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    yield();
  }

  Serial.println();
  Serial.print("WiFi connected, IP = ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/ping", HTTP_GET, handlePing);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/send", HTTP_GET, handleSend);
  server.on("/api/ac", HTTP_GET, handleAc);
  server.onNotFound(handleNotFound);

  server.begin();

  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient();
}
