/* =============== BLYNK AUTH INFO =============== */
#define BLYNK_TEMPLATE_ID "BLYNK_TEMPLATE_ID"
#define BLYNK_DEVICE_NAME "Home Lights"
#define BLYNK_AUTH_TOKEN "BLYNK_AUTH_TOKEN"
/* =============================================== */

/* =================== OPTIONS =================== */
#define BLYNK_PRINT Serial
#define LOCAL_DEBUG true
/* =============================================== */

/* =============== PINOUT SETTINGS =============== */
#define NUM_LEDS 44
#define LED_PIN 13
#define LED_POWER_PIN 16
#define USB_PIN 12
#define GYRLAND_PIN 14
/* =============================================== */

/* ================= EVENT TYPES ================= */
#define NO_EVENT 0
#define EVENT_ON 1
#define EVENT_OFF 2
#define CHANGE_BRIGHTNESS 3
#define CHANGE_COLOR_DOWN 4
#define CHANGE_COLOR_UP 5
/* =============================================== */

/* ================ VIRTUAL PINS ================= */
#define RGB_STATE V4
#define RGB_COLOR V5
#define RGB_BRIGHTNESS V6
#define RGB_AMBILIGHT_STATE V7

#define USB_STATE V0
#define USB_BRIGHTNESS V1

#define GYRLAND_STATE V2

#define TEMPERATURE V8
#define HUMIDITY V9
/* =============================================== */

#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <BlynkSimpleEsp8266.h>
#include <Adafruit_AHTX0.h>
#include <TimerMs.h>
#include <ArduinoJson.h>

TimerMs tmrUsb(1);
TimerMs tmrRgb(1);
TimerMs tmrGyrland(1);
Adafruit_AHTX0 aht;
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Eugene";
char pass[] = "787898852456";
CRGB leds[NUM_LEDS];
ESP8266WebServer server(80);

int usbBright = 0;
byte rgbBright = 0;
bool usbState, rgbState, gyrlandState = 0;
byte rgbColor[3] = {0, 0, 0};
byte TmpRgbColor[3] = {0, 0, 0};
sensors_event_t humidity, temp;
int tempValUsb = 0;
byte tempValRgb = 0;
bool tmpVectorUsb, tmpVectorRgb = 0;
byte UsbEvent, RgbEvent, GyrlandEvent = NO_EVENT;
int SwitchPowerValUsb, SwitchPowerValRgb, SwitchPowerValGyrland = 0;
bool ambilightState = 0;
DynamicJsonDocument InputData(4096);

// синхронизация при подключении
BLYNK_CONNECTED() {
  Blynk.syncVirtual(USB_STATE, RGB_STATE, GYRLAND_STATE);
  Blynk.syncVirtual(USB_BRIGHTNESS, RGB_BRIGHTNESS);
  Blynk.syncVirtual(RGB_COLOR);
}
// запрос температуры
BLYNK_READ(TEMPERATURE)
{
  aht.getEvent(&humidity, &temp);
  Blynk.virtualWrite(TEMPERATURE, temp.temperature);
}
// запрос влажности
BLYNK_READ(HUMIDITY)
{
  aht.getEvent(&humidity, &temp);
  Blynk.virtualWrite(HUMIDITY, humidity.relative_humidity);
}

// Установка яркости ленты
BLYNK_WRITE(RGB_BRIGHTNESS)
{
  WriteLog("SET BRIGHT RGB");
  if (rgbState){
    tempValRgb = map(param.asInt(), 1, 100, 1, 255);
    RgbEvent = CHANGE_BRIGHTNESS;
    tmpVectorRgb = tempValRgb > rgbBright;
    tmrRgb.start();
  } else rgbBright = map(param.asInt(), 1, 100, 1, 255);
}
// чтение яркости ленты
BLYNK_READ(RGB_BRIGHTNESS)
{
  WriteLog("READ BRIGHT RGB");
  Blynk.virtualWrite(RGB_BRIGHTNESS, map(rgbBright, 1, 255, 1, 100));
}
// включение/выключение ленты
BLYNK_WRITE(RGB_STATE)
{
  WriteLog("SWITCH STATE RGB");
  if (param.asInt() == rgbState) return;
  RgbEvent = param.asInt() ? EVENT_ON : EVENT_OFF;
  tempValRgb = param.asInt() ? rgbBright : 0;
  tmpVectorRgb = tempValRgb > rgbBright;
  SwitchPowerValRgb = param.asInt() ? 0 : rgbBright;
  tmrRgb.start();
}
// чтение состояния ленты
BLYNK_READ(RGB_STATE)
{
  WriteLog("READ STATE RGB");
  Blynk.virtualWrite(RGB_STATE, rgbState);
}
// установка цвета ленты
BLYNK_WRITE(RGB_COLOR)
{
  WriteLog("SET COLOR RGB");
  if(rgbState){
    RgbEvent = CHANGE_COLOR_DOWN;
    SwitchPowerValRgb = rgbBright;
    TmpRgbColor[0] = param[0].asInt();
    TmpRgbColor[1] = param[1].asInt();
    TmpRgbColor[2] = param[2].asInt();
    tmrRgb.start();
  }else{
    rgbColor[0] = param[0].asInt();
    rgbColor[1] = param[1].asInt();
    rgbColor[2] = param[2].asInt();
  }
}
// чтение цвета
BLYNK_READ(RGB_COLOR)
{
  WriteLog("READ STATE RGB");
  Blynk.virtualWrite(RGB_COLOR, rgbColor[0], rgbColor[1], rgbColor[2]);
}
// чтение состояния юсби
BLYNK_READ(USB_STATE)
{
  WriteLog("READ STATE USB");
  Blynk.virtualWrite(USB_STATE, usbState);
}
// включение/выключение юсби
BLYNK_WRITE(USB_STATE)
{
  WriteLog("SWITCH USB");
  if (param.asInt() == usbState) return;
  UsbEvent = param.asInt() ? EVENT_ON : EVENT_OFF;
  tempValUsb = param.asInt() ? usbBright : 0;
  tmpVectorUsb = tempValUsb > usbBright;
  SwitchPowerValUsb = param.asInt() ? 0 : usbBright;
  tmrUsb.start();
}
// установка яркости юсби
BLYNK_WRITE(USB_BRIGHTNESS)
{
  WriteLog("SET BRIGHT USB");
  if(usbState){
    tempValUsb = map(param.asInt(), 1, 100, 1, 1023);
    UsbEvent = CHANGE_BRIGHTNESS;
    tmpVectorUsb = tempValUsb > usbBright;
    tmrUsb.start();
  }else usbBright = map(param.asInt(), 1, 100, 1, 1023);
}
// запрос яркости юсби
BLYNK_READ(USB_BRIGHTNESS)
{
  WriteLog("READ BRIGHT USB");
  Blynk.virtualWrite(USB_BRIGHTNESS, map(usbBright, 1, 1023, 1, 100));
}

// чтение состояния гирлянды
BLYNK_READ(GYRLAND_STATE)
{
  WriteLog("READ GYRLAND_STATE");
  Blynk.virtualWrite(GYRLAND_STATE, gyrlandState);
}
// включение/выключение гирлянды
BLYNK_WRITE(GYRLAND_STATE)
{
  WriteLog("SET GYRLAND_STATE 1");
  if (param.asInt() == gyrlandState) return;
  gyrlandState = param.asInt();
  analogWrite(GYRLAND_PIN, gyrlandState ? 1023 : 0);
  WriteLog("SET GYRLAND_STATE 2");
  Blynk.virtualWrite(GYRLAND_STATE, gyrlandState);
}

void setup()
{
  Serial.begin(115200);
  analogWriteFreq(150);
  WiFi.hostname("Eugene Ambilight 1.2");
  tmrUsb.setPeriodMode();
  tmrRgb.setPeriodMode();
  tmrGyrland.setPeriodMode();
  tmrUsb.stop();
  tmrRgb.stop();
  tmrGyrland.stop();
  aht.begin();
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  Blynk.begin(auth, ssid, pass);
  WiFi.setAutoReconnect(true);
  server.begin();
  server.on("/set-rgb", SetRGB);
  server.on("/ambilight-state", ChangeAmbilightState);
  server.on("/ambilight", GoAmbilight);
  server.on("/ping", Ping);
  server.onNotFound([] () {
    server.send(404, "text/plain", "Not found");
  });
  WriteLog(WiFi.localIP());
  analogWrite(LED_POWER_PIN, 1023);
}

void loop()
{
  Blynk.run();
  server.handleClient();
  tmrUsb.tick();
  tmrRgb.tick();
  if (tmrUsb.ready()) {
    switch (UsbEvent)
    {
      case EVENT_ON: {
          if (SwitchPowerValUsb != usbBright) {
            SwitchPowerValUsb++;
            analogWrite(USB_PIN, SwitchPowerValUsb);
            WriteLog("[USB] EVENT ON" + String(SwitchPowerValUsb));
          } else {
            UsbEvent = NO_EVENT;
            usbState = true;
          }
        } break;
      case EVENT_OFF: {
          if (SwitchPowerValUsb > 0) {
            SwitchPowerValUsb--;
            analogWrite(USB_PIN, SwitchPowerValUsb);
            WriteLog("[USB] EVENT OFF" + String(SwitchPowerValUsb));
          } else {
            UsbEvent = NO_EVENT;
            usbState = false;
          }
        } break;
      case CHANGE_BRIGHTNESS: {
          if (usbBright != tempValUsb) {
            usbBright += tmpVectorUsb ? 1 : -1;
            analogWrite(USB_PIN, usbBright);
            WriteLog("[USB] CHANGE BRIGHTNESS" + String(usbBright));
          } else {
            UsbEvent = NO_EVENT;
            usbState = usbBright > 0;
          }
        } break;
      case NO_EVENT: {
          WriteLog("NO_EVENT USB");
          tmrUsb.stop();
          Blynk.virtualWrite(USB_STATE, usbState);
          WriteLog("[USB] EVENT END" + String(usbBright));
        } break;
    }
  }
  if (tmrRgb.ready()) {
    switch (RgbEvent) {
      case EVENT_ON: case CHANGE_COLOR_UP: {
          if (SwitchPowerValRgb != rgbBright) {
            SwitchPowerValRgb++;
            LEDS.setBrightness(SwitchPowerValRgb);
            LEDS.showColor(CRGB(rgbColor[0], rgbColor[1], rgbColor[2]));
          } else {
            RgbEvent = NO_EVENT;
            rgbState = true;
          }
          //WriteLog(RgbEvent == CHANGE_COLOR_UP ? "CHANGE_COLOR_UP " : "EVENT_ON " + String(SwitchPowerValRgb));
        } break;
      case EVENT_OFF: case CHANGE_COLOR_DOWN: {
          if (SwitchPowerValRgb > 0) {
            SwitchPowerValRgb--;
            LEDS.setBrightness(SwitchPowerValRgb);
            LEDS.showColor(CRGB(rgbColor[0], rgbColor[1], rgbColor[2]));
          } else {
            if (RgbEvent == CHANGE_COLOR_DOWN) {
              RgbEvent = CHANGE_COLOR_UP;
              rgbColor[0] = TmpRgbColor[0];
              rgbColor[1] = TmpRgbColor[1];
              rgbColor[2] = TmpRgbColor[2];
            } else {
              RgbEvent = NO_EVENT;
              rgbState = false;
            }
          }
          //WriteLog(RgbEvent == CHANGE_COLOR_DOWN ? "CHANGE_COLOR_DOWN " : "EVENT_OFF " + String(SwitchPowerValRgb));
        } break;
      case CHANGE_BRIGHTNESS: {
          if (rgbBright != tempValRgb) {
            rgbBright += tmpVectorRgb ? 1 : -1;
            LEDS.setBrightness(rgbBright);
            LEDS.showColor(CRGB(rgbColor[0], rgbColor[1], rgbColor[2]));
          } else {
            RgbEvent = NO_EVENT;
            rgbState = rgbBright > 0;
          }
          //WriteLog("CHANGE_BRIGHTNESS " + String(rgbBright));
        } break;
      case NO_EVENT: {
          //WriteLog("NO_EVENT RGB");
          tmrRgb.stop();
          Blynk.virtualWrite(RGB_STATE, rgbState);
        } break;
    }
  }
}

void SetRGB(){
  if (server.method() == HTTP_POST) {
    if(server.arg("plain")){
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, server.arg("plain"));
      if(doc.containsKey("r") && doc.containsKey("g") && doc.containsKey("b")){
        if(doc["delay"].as<bool>()){
          if(rgbState){
            RgbEvent = CHANGE_COLOR_DOWN;
            SwitchPowerValRgb = rgbBright;
            TmpRgbColor[0] = doc["r"].as<int>();
            TmpRgbColor[1] = doc["g"].as<int>();
            TmpRgbColor[2] = doc["b"].as<int>();
            tmrRgb.start();
          }else{
            rgbColor[0] = doc["r"].as<int>();
            rgbColor[1] = doc["g"].as<int>();
            rgbColor[2] = doc["b"].as<int>();
          }
        }else 
          LEDS.showColor(CRGB(doc["r"].as<int>(), doc["g"].as<int>(), doc["b"].as<int>()));
        server.send(200, "text/plain", "ok");
      }else server.send(400, "text/plain", "Bad request");
    }else server.send(400, "text/plain", "Bad request");
  }else server.send(405, "text/plain", "Method Not Allowed");
}

void GoAmbilight(){
  WriteLog("GoAmbilight");
  if (server.method() == HTTP_POST) {
    if(server.arg("plain")){
      deserializeJson(InputData, server.arg("plain"));
      if(InputData.containsKey("Items") && InputData.containsKey("ItemsCount")){
        for (int i = 0; i < sizeof(InputData["Items"]) / 2 - 1; i++){
          Serial.print(InputData["Items"][i]["r"].as<int>()); Serial.print(" ");
          Serial.print(InputData["Items"][i]["g"].as<int>()); Serial.print(" ");
          Serial.print(InputData["Items"][i]["b"].as<int>()); Serial.print(" ");
          WriteLog(InputData["Items"][i]["i"].as<int>());
          leds[InputData["Items"][i]["i"].as<int>()] = CRGB(InputData["Items"][i]["r"].as<int>(), InputData["Items"][i]["g"].as<int>(), InputData["Items"][i]["b"].as<int>());
          FastLED.show();
        }
        server.send(200, "text/plain", "ok");
      }else server.send(400, "text/plain", "Bad request");
    }else server.send(400, "text/plain", "Bad request");
  }else server.send(405, "text/plain", "Method Not Allowed");
}

void ChangeAmbilightState(){
  if (server.method() == HTTP_POST) {
    if(server.arg("state")){
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, server.arg("plain"));
      if(doc.containsKey("state")){
        ambilightState = doc["state"] == true;
        server.send(200, "text/plain", String(ambilightState));
      }else server.send(400, "text/plain", "Bad request");
    }else server.send(400, "text/plain", "Bad request");
  }else server.send(405, "text/plain", "Method Not Allowed");
}

void Ping(){
  if(server.arg("ping")){
    DynamicJsonDocument response(255);
    response["name"] = String(BLYNK_DEVICE_NAME);
    response["token"] = String(BLYNK_AUTH_TOKEN);
    response["leds"] = NUM_LEDS;
    server.send(200, "application/json", response.as<String>());
  }
}

void WriteLog(String text){
  if(LOCAL_DEBUG)
    Serial.println(text);
}