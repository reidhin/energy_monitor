// https://github.com/mruettgers/SMLReader
// https://www.bitsandparts.nl/Lichtsensor-Fototransistor-BPW40-p115130
// BPW40
// flat side = collector
// pijl gaat naar emitter

// https://subscription.packtpub.com/book/iot-and-hardware/9781785888564/3/ch03lvl1sec27/reading-and-counting-pulses-with-arduino
// https://roboticsbackend.com/arduino-pulsein-with-interrupts/
// https://forum.arduino.cc/t/counting-pulse-of-energy-meter/550830/8

// follow this first: https://randomnerdtutorials.com/esp32-esp8266-plot-chart-web-server/


// interrupts: https://gammon.com.au/interrupts
/**
 * ----------------------------------------------------------------------------
 * Energy Monitor
 * ----------------------------------------------------------------------------
 * © 2024 Hans Weda
 * ----------------------------------------------------------------------------
 */

// include secrets to connect to wifi 
#include "arduino_secrets.h"

// File system
#include <LittleFS.h>

// WiFi and webserver stuff
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// ----------------------------------------------------------------------------
// Definition of macros
// ----------------------------------------------------------------------------

// Port for webserver
#define HTTP_PORT 80

// ----------------------------------------------------------------------------
// Definition of global constants
// ----------------------------------------------------------------------------

const int length = 200;

// ----------------------------------------------------------------------------
// Definition of the Led component
// ----------------------------------------------------------------------------

struct Led {
  // state variables
  uint8_t pin;
  bool    on;
  int     brightness;

  // methods
  void update() {
    analogWrite(pin, on ? brightness : 255);
  }
};

// ----------------------------------------------------------------------------
// Definition of the analog sensor
// ----------------------------------------------------------------------------

struct AnalogSensor {
  // state variables
  uint8_t pin;

  // methods
  int read() {
    return analogRead(pin);
  }
};

// ----------------------------------------------------------------------------
// Definition of global variables
// ----------------------------------------------------------------------------

Led onboard_led = { LED_BUILTIN, false, 255 };  // LED_BUILTIN is a variable arduino

AnalogSensor photo_resistor = { A0 };  // A0 is an arduino variable - ESP8266 Analog Pin ADC0 = A0

AsyncWebServer server(HTTP_PORT);

int sensorValue = 0;

int count = 0;

int values[length];
long timestamps[length];

time_t now;

// ----------------------------------------------------------------------------
// LittleFS initialization
// ----------------------------------------------------------------------------

void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("Cannot mount LittleFS volume...");
    while (1) {
        onboard_led.on = millis() % 200 < 50;
        onboard_led.update();
    }
  }
}

// ----------------------------------------------------------------------------
// Connecting to the WiFi network
// ----------------------------------------------------------------------------

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SECRET_SSID, SECRET_PASSWORD);
  Serial.printf("Trying to connect [%s] ", WiFi.macAddress().c_str());
  while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }
  Serial.printf(" %s\n", WiFi.localIP().toString().c_str());
  
  //The ESP8266 tries to reconnect automatically when the connection is lost
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

// ----------------------------------------------------------------------------
// Web server initialization
// ----------------------------------------------------------------------------

String processor(const String &var) {
  return String();
}

void onRootRequest(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/index.html", "text/html", false, processor);
}

void onSensorRequest(AsyncWebServerRequest *request) {
  request->send(200, "text/html", String(photo_resistor.read()));
}

void onDataRequest(AsyncWebServerRequest *request) {
  // Allocate a temporary JsonDocument
  JsonDocument doc;

  // Create the data array
  JsonArray dataValues = doc["data"].to<JsonArray>();
  for (byte i = 0; i < length; i = i + 1) {
    JsonArray measurement = dataValues.add<JsonArray>();
    measurement.add(timestamps[(i + count) % length]);
    measurement.add(values[(i + count) % length]);
  };
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

void initWebServer() {
  server.on("/", onRootRequest);
  server.serveStatic("/", LittleFS, "/");
  server.on("/energy", onSensorRequest);
  server.on("/data", onDataRequest);
  server.begin();
  Serial.println("HTTP Server Started");
}

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

void setup() {
  // put your setup code here, to run once:
  pinMode(onboard_led.pin, OUTPUT);

  Serial.begin(115200); delay(500);

  initLittleFS();
  initWiFi();
  initWebServer();
  
  onboard_led.brightness = 200;
  onboard_led.on = false;
  onboard_led.update();
}

void loop() {
  // read the analog in value
  sensorValue = photo_resistor.read();
  time(&now);

  // flash the onboard led
  // onboard_led.on = millis() % 2000 < 200;
  // onboard_led.update();

  // print the readings in the Serial Monitor
  // Serial.print("sensor = ");
  // Serial.println(sensorValue);
  
  values[count % length] = sensorValue;
  timestamps[count % length] = millis();
  count +=  1;

  // if (!(count % 10)) {
  //   Serial.print("count = ");
  //   Serial.println(count);
  //   for (byte i = 0; i < length; i = i + 1) {
  //     Serial.print(values[i]);
  //     Serial.print(", ");
  //   };
  //   Serial.println();
  //   for (byte i = 0; i < length; i = i + 1) {
  //     Serial.print(timestamps[i]);
  //     Serial.print(", ");
  //   };
  //   Serial.println();
  // }

  delay(25);
}

