#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <IRremoteESP8266.h>
#include <ir_Coolix.h>

/*
  ESP8266 Coolix A/C IR gateway

  Wi-Fi:
    SSID: AC_Remote_AP
    PASS: <WIFI_PASSWORD>

  IR:
    NodeMCU D2 = GPIO4

  APIs:
    /ping
    /api/state
    /api/ac?power=on&mode=cool&temp=26&fan=high
    /api/ac?power=off

  The captured Midea frames match the COOLIX 24-bit protocol, so this
  firmware uses IRCoolixAC instead of maintaining raw timing arrays.
*/

const char* WIFI_SSID = "AC_Remote_AP";
const char* WIFI_PASSWORD = "<WIFI_PASSWORD>";

const uint16_t kIrLed = 4;  // NodeMCU D2 = GPIO4.
const char* kProtocol = "COOLIX";
const char* kFirmwareVersion = "coolix-ac-gateway-20260605-01";

const uint8_t kDefaultTemp = 26;
const char* kDefaultMode = "cool";
const char* kDefaultFan = "high";

IRCoolixAC ac(kIrLed);
ESP8266WebServer server(80);

String lastCommand = "none";
String lastPower = "off";
String lastMode = kDefaultMode;
uint8_t lastTemp = kDefaultTemp;
String lastFan = kDefaultFan;
uint32_t lastCoolixState = 0;
unsigned long lastSendMillis = 0;

String jsonEscape(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  value.replace("\n", "\\n");
  value.replace("\r", "\\r");
  return value;
}

void sendJson(const int code, const String& body) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(code, "application/json; charset=utf-8", body);
}

String normalizeArg(String value) {
  value.trim();
  value.toLowerCase();
  return value;
}

String normalizePower(String power) {
  power = normalizeArg(power);
  if (power == "off") return "off";
  return "on";
}

bool isIntegerString(const String& value) {
  if (value.length() == 0) return false;
  for (uint16_t i = 0; i < value.length(); i++) {
    const char ch = value.charAt(i);
    if (ch < '0' || ch > '9') return false;
  }
  return true;
}

uint8_t clampTemp(const int value) {
  if (value < kCoolixTempMin) return kCoolixTempMin;
  if (value > kCoolixTempMax) return kCoolixTempMax;
  return value;
}

uint8_t parseTemp(String tempArg) {
  tempArg.trim();
  if (!isIntegerString(tempArg)) return kDefaultTemp;
  return clampTemp(tempArg.toInt());
}

uint8_t coolixModeFromArg(String mode, String& normalizedMode) {
  mode = normalizeArg(mode);

  if (mode == "heat") {
    normalizedMode = "heat";
    return kCoolixHeat;
  }
  if (mode == "dry") {
    normalizedMode = "dry";
    return kCoolixDry;
  }
  if (mode == "fan") {
    normalizedMode = "fan";
    return kCoolixFan;
  }
  if (mode == "auto") {
    normalizedMode = "auto";
    return kCoolixAuto;
  }

  normalizedMode = "cool";
  return kCoolixCool;
}

uint8_t coolixFanFromArg(String fan, String& normalizedFan) {
  fan = normalizeArg(fan);

  if (fan == "low") {
    normalizedFan = "low";
    return kCoolixFanMin;
  }
  if (fan == "mid" || fan == "medium") {
    normalizedFan = "mid";
    return kCoolixFanMed;
  }
  if (fan == "auto") {
    normalizedFan = "auto";
    return kCoolixFanAuto;
  }

  normalizedFan = "high";
  return kCoolixFanMax;
}

String coolixStateHex() {
  char buffer[11];
  snprintf(buffer, sizeof(buffer), "0x%06lX",
           static_cast<unsigned long>(lastCoolixState & 0xFFFFFF));
  return String(buffer);
}

void updateLastState(const String& command, const String& power,
                     const String& mode, const uint8_t temp,
                     const String& fan, const uint32_t sentState) {
  lastCommand = command;
  lastPower = power;
  if (power == "on") {
    lastMode = mode;
    lastTemp = temp;
    lastFan = fan;
  }
  lastCoolixState = sentState;
  lastSendMillis = millis();
}

void sendCoolixOff() {
  ac.off();
  const uint32_t sentState = ac.getRaw();
  ac.send();
  updateLastState("power_off", "off", lastMode, lastTemp, lastFan, sentState);
}

void sendCoolixOn(const String& mode, const uint8_t modeValue,
                  const uint8_t temp, const String& fan,
                  const uint8_t fanValue) {
  ac.on();
  ac.setMode(modeValue);
  ac.setTemp(temp);

  // kCoolixFan is synthetic in IRremoteESP8266. Setting temperature after
  // fan mode can replace its special temp code, so restore fan mode here.
  if (modeValue == kCoolixFan) {
    ac.setMode(kCoolixFan);
  }

  ac.setFan(fanValue);
  const uint32_t sentState = ac.getRaw();
  ac.send();
  updateLastState("ac", "on", mode, temp, fan, sentState);
}

String successJsonForOff() {
  String body;
  body.reserve(192);
  body += "{";
  body += "\"ok\":true,";
  body += "\"protocol\":\"";
  body += kProtocol;
  body += "\",";
  body += "\"power\":\"off\",";
  body += "\"coolix_state\":\"";
  body += coolixStateHex();
  body += "\"";
  body += "}";
  return body;
}

String successJsonForOn(const String& mode, const uint8_t temp,
                        const String& fan) {
  String body;
  body.reserve(256);
  body += "{";
  body += "\"ok\":true,";
  body += "\"protocol\":\"";
  body += kProtocol;
  body += "\",";
  body += "\"power\":\"on\",";
  body += "\"mode\":\"";
  body += jsonEscape(mode);
  body += "\",";
  body += "\"temp\":";
  body += String(temp);
  body += ",";
  body += "\"fan\":\"";
  body += jsonEscape(fan);
  body += "\",";
  body += "\"coolix_state\":\"";
  body += coolixStateHex();
  body += "\"";
  body += "}";
  return body;
}

void applyAcState(String powerArg, String modeArg, String tempArg,
                  String fanArg) {
  const String power = normalizePower(powerArg);

  if (power == "off") {
    Serial.println("Sending COOLIX power off");
    sendCoolixOff();
    sendJson(200, successJsonForOff());
    return;
  }

  String mode;
  String fan;
  const uint8_t modeValue = coolixModeFromArg(modeArg, mode);
  const uint8_t fanValue = coolixFanFromArg(fanArg, fan);
  const uint8_t temp = parseTemp(tempArg);

  Serial.print("Sending COOLIX state: mode=");
  Serial.print(mode);
  Serial.print(", temp=");
  Serial.print(temp);
  Serial.print(", fan=");
  Serial.println(fan);

  sendCoolixOn(mode, modeValue, temp, fan, fanValue);
  sendJson(200, successJsonForOn(mode, temp, fan));
}

bool legacyCommandToState(String cmd, String& power, String& mode,
                          String& temp, String& fan) {
  cmd = normalizeArg(cmd);

  power = "on";
  mode = kDefaultMode;
  temp = String(kDefaultTemp);
  fan = kDefaultFan;

  if (cmd == "power_off" || cmd == "off") {
    power = "off";
    return true;
  }
  if (cmd == "power_on" || cmd == "on") {
    return true;
  }

  if (cmd.startsWith("cool_")) {
    const String tempPart = cmd.substring(5);
    if (isIntegerString(tempPart)) {
      mode = "cool";
      temp = tempPart;
      return true;
    }
  }

  if (cmd == "fan_low" || cmd == "fan_1") {
    mode = "fan";
    fan = "low";
    return true;
  }
  if (cmd == "fan_mid" || cmd == "fan_2") {
    mode = "fan";
    fan = "mid";
    return true;
  }
  if (cmd == "fan_high" || cmd == "fan_3") {
    mode = "fan";
    fan = "high";
    return true;
  }

  return false;
}

void handleRoot() {
  String body;
  body.reserve(256);
  body += "{";
  body += "\"ok\":true,";
  body += "\"device\":\"esp8266-coolix-ac-gateway\",";
  body += "\"protocol\":\"";
  body += kProtocol;
  body += "\",";
  body += "\"firmware\":\"";
  body += kFirmwareVersion;
  body += "\",";
  body += "\"usage\":\"/ping, /api/state, /api/ac?power=on&mode=cool&temp=26&fan=high\"";
  body += "}";
  sendJson(200, body);
}

void handlePing() {
  String body;
  body.reserve(256);
  body += "{";
  body += "\"ok\":true,";
  body += "\"device\":\"esp8266-coolix-ac-gateway\",";
  body += "\"protocol\":\"";
  body += kProtocol;
  body += "\",";
  body += "\"firmware\":\"";
  body += kFirmwareVersion;
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
  body += "\"ok\":true,";
  body += "\"device\":\"esp8266-coolix-ac-gateway\",";
  body += "\"protocol\":\"";
  body += kProtocol;
  body += "\",";
  body += "\"firmware\":\"";
  body += kFirmwareVersion;
  body += "\",";
  body += "\"ip\":\"";
  body += WiFi.localIP().toString();
  body += "\",";
  body += "\"wifi_rssi\":";
  body += String(WiFi.RSSI());
  body += ",";
  body += "\"ir_led_pin\":";
  body += String(kIrLed);
  body += ",";
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
  body += "\",";
  body += "\"coolix_state\":\"";
  body += coolixStateHex();
  body += "\",";
  body += "\"last_send_millis\":";
  body += String(lastSendMillis);
  body += "}";
  sendJson(200, body);
}

void handleAc() {
  applyAcState(
      server.arg("power"),
      server.arg("mode"),
      server.arg("temp"),
      server.arg("fan"));
}

void handleSend() {
  String power;
  String mode;
  String temp;
  String fan;

  if (!legacyCommandToState(server.arg("cmd"), power, mode, temp, fan)) {
    sendJson(400, "{\"ok\":false,\"protocol\":\"COOLIX\",\"error\":\"unknown cmd\"}");
    return;
  }

  applyAcState(power, mode, temp, fan);
}

void handleNotFound() {
  String body;
  body.reserve(256);
  body += "{";
  body += "\"ok\":false,";
  body += "\"protocol\":\"";
  body += kProtocol;
  body += "\",";
  body += "\"error\":\"not found\",";
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
  Serial.println("ESP8266 Coolix A/C IR gateway starting...");
  Serial.print("Firmware: ");
  Serial.println(kFirmwareVersion);

  ac.begin();
  ac.stateReset();
  lastCoolixState = ac.getRaw();

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    yield();
  }

  Serial.println();
  Serial.print("Wi-Fi connected, IP = ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/ping", HTTP_GET, handlePing);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/ac", HTTP_GET, handleAc);
  server.on("/api/send", HTTP_GET, handleSend);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient();
}
