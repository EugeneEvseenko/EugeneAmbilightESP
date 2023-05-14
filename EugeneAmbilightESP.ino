/* =============== BLYNK AUTH INFO =============== */
#define BLYNK_TEMPLATE_ID "TMPLl0jZXlNn"
#define BLYNK_DEVICE_NAME "Home Lights"
#define BLYNK_AUTH_TOKEN "FNbQqaK99s14nya8zL0QDT57ucyu3Pez"
#define BLYNK_FIRMWARE_VERSION "1.3.1"
/* =============================================== */

/* =================== OPTIONS =================== */
#define BLYNK_PRINT Serial
#define LOCAL_DEBUG false
#define PWM_MAX_VALUE 255
/* =============================================== */

/* =============== PINOUT SETTINGS =============== */
#define USB_PIN 12
#define GYRLAND_PIN 14
/* =============================================== */

/* ================= EVENT TYPES ================= */
#define NO_EVENT 0
#define EVENT_ON 1
#define EVENT_OFF 2
#define CHANGE_BRIGHTNESS 3
/* =============================================== */

/* ================ VIRTUAL PINS ================= */
#define USB_BRIGHTNESS V0
#define USB_STATE V1

#define GYRLAND_STATE V2
#define GYRLAND_BRIGHTNESS V3

/* =============================================== */

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <TimerMs.h>

BlynkTimer blynkState;
TimerMs tmrUsb(1);
TimerMs tmrGyrland(1);
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Eugene";
char pass[] = "787898852456";

int usbBright, gyrlandBright = 0;
bool usbState, gyrlandState = 0;
int tempValUsb, tempValGyrland = 0;
bool tmpVectorUsb, tmpVectorGyrland = 0;
byte UsbEvent, GyrlandEvent = NO_EVENT;
int SwitchPowerValUsb, SwitchPowerValGyrland = 0;

// синхронизация при подключении
BLYNK_CONNECTED() {
  Blynk.syncVirtual(USB_STATE, GYRLAND_STATE);
  Blynk.syncVirtual(USB_BRIGHTNESS, GYRLAND_BRIGHTNESS);
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
    tempValUsb = map(param.asInt(), 1, 100, 1, PWM_MAX_VALUE);
    UsbEvent = CHANGE_BRIGHTNESS;
    tmpVectorUsb = tempValUsb > usbBright;
    tmrUsb.start();
  }else usbBright = map(param.asInt(), 1, 100, 1, PWM_MAX_VALUE);
}

// опрос состояния
void BlynkRead()
{
  Blynk.virtualWrite(USB_STATE, usbState);
  Blynk.virtualWrite(USB_BRIGHTNESS, map(usbBright, 1, PWM_MAX_VALUE, 1, 100));
  Blynk.virtualWrite(GYRLAND_BRIGHTNESS, map(gyrlandBright, 1, PWM_MAX_VALUE, 1, 100));
  Blynk.virtualWrite(GYRLAND_STATE, gyrlandState);
}

// включение/выключение гирлянды
BLYNK_WRITE(GYRLAND_STATE)
{
  WriteLog("SWITCH GYRLAND");
  if (param.asInt() == gyrlandState) return;
  GyrlandEvent = param.asInt() ? EVENT_ON : EVENT_OFF;
  tempValGyrland = param.asInt() ? gyrlandBright : 0;
  tmpVectorUsb = tempValGyrland > gyrlandBright;
  SwitchPowerValGyrland = param.asInt() ? 0 : gyrlandBright;
  tmrGyrland.start();
}

// установка яркости гирлянды
BLYNK_WRITE(GYRLAND_BRIGHTNESS)
{
  WriteLog("SET BRIGHT GYRLAND");
  if(gyrlandState){
    tempValGyrland = map(param.asInt(), 1, 100, 1, PWM_MAX_VALUE);
    GyrlandEvent = CHANGE_BRIGHTNESS;
    tmpVectorGyrland = tempValGyrland > gyrlandBright;
    tmrGyrland.start();
  }else gyrlandBright = map(param.asInt(), 1, 100, 1, PWM_MAX_VALUE);
}

void setup()
{
  Serial.begin(115200);
  analogWriteFreq(150);
  WiFi.hostname("Eugene Ambilight 1.3.1");
  blynkState.setInterval(1000L, BlynkRead);
  tmrUsb.setPeriodMode();
  tmrUsb.stop();
  tmrGyrland.setPeriodMode();
  tmrGyrland.stop();
  Blynk.begin(auth, ssid, pass);
  WiFi.setAutoReconnect(true);
  Serial.println(WiFi.localIP());
}

void loop()
{
  Blynk.run();
  blynkState.run();
  tmrUsb.tick();
  tmrGyrland.tick();
  if (tmrUsb.ready()) {
    switch (UsbEvent)
    {
      case EVENT_ON: {
          if (SwitchPowerValUsb != usbBright) {
            SwitchPowerValUsb++;
            analogWrite(USB_PIN, SwitchPowerValUsb);
            WriteLog("[USB] EVENT ON " + String(SwitchPowerValUsb));
          } else {
            UsbEvent = NO_EVENT;
            usbState = true;
          }
        } break;
      case EVENT_OFF: {
          if (SwitchPowerValUsb > 0) {
            SwitchPowerValUsb--;
            analogWrite(USB_PIN, SwitchPowerValUsb);
            WriteLog("[USB] EVENT OFF " + String(SwitchPowerValUsb));
          } else {
            UsbEvent = NO_EVENT;
            usbState = false;
          }
        } break;
      case CHANGE_BRIGHTNESS: {
          if (usbBright != tempValUsb) {
            usbBright += tmpVectorUsb ? 1 : -1;
            analogWrite(USB_PIN, usbBright);
            WriteLog("[USB] CHANGE BRIGHTNESS " + String(usbBright));
          } else {
            UsbEvent = NO_EVENT;
            usbState = usbBright > 0;
          }
        } break;
      case NO_EVENT: {
          WriteLog("NO_EVENT USB");
          tmrUsb.stop();
          Blynk.virtualWrite(USB_STATE, usbState);
          WriteLog("[USB] EVENT END " + String(usbBright));
        } break;
    }
  }

  if (tmrGyrland.ready()) {
    switch (GyrlandEvent)
    {
      case EVENT_ON: {
          if (SwitchPowerValGyrland != gyrlandBright) {
            SwitchPowerValGyrland++;
            analogWrite(GYRLAND_PIN, SwitchPowerValGyrland);
            WriteLog("[GYRLAND] EVENT ON " + String(SwitchPowerValGyrland));
          } else {
            GyrlandEvent = NO_EVENT;
            gyrlandState = true;
          }
        } break;
      case EVENT_OFF: {
          if (SwitchPowerValGyrland > 0) {
            SwitchPowerValGyrland--;
            analogWrite(GYRLAND_PIN, SwitchPowerValGyrland);
            WriteLog("[GYRLAND] EVENT OFF " + String(SwitchPowerValGyrland));
          } else {
            GyrlandEvent = NO_EVENT;
            gyrlandState = false;
          }
        } break;
      case CHANGE_BRIGHTNESS: {
          if (gyrlandBright != tempValGyrland) {
            gyrlandBright += tmpVectorGyrland ? 1 : -1;
            analogWrite(GYRLAND_PIN, gyrlandBright);
            WriteLog("[GYRLAND] CHANGE BRIGHTNESS " + String(gyrlandBright));
          } else {
            GyrlandEvent = NO_EVENT;
            gyrlandState = gyrlandBright > 0;
          }
        } break;
      case NO_EVENT: {
          WriteLog("NO_EVENT GYRLAND");
          tmrGyrland.stop();
          Blynk.virtualWrite(GYRLAND_STATE, gyrlandState);
          WriteLog("[GYRLAND] EVENT END " + String(gyrlandBright));
        } break;
    }
  }
}

void WriteLog(String text){
  if(LOCAL_DEBUG)
    Serial.println(text);
}