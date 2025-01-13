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

/* Configuration of NTP */
	// this must always be done.
	// set up TZ string to use a POSIX/gnu TZ string for local timezone
	// TZ string information:
	// https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
#define MY_NTP_SERVER "pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0,M10.5.0/3"  

// ----------------------------------------------------------------------------
// Definition of global constants
// ----------------------------------------------------------------------------

const int length = 200;

const int delta_time = 25;


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
// Definition of the sensor data
// ----------------------------------------------------------------------------

struct SensorData {
  // state variables - data
  int count_sample;
  int data_values[length];
  unsigned long data_timestamps[length];

  // state variables - peaks
  int count_peaks;
  int peak_values[length];
  unsigned long peak_timestamps[length];
  int min_value;
  int max_value;
  unsigned long peak_start;
  unsigned long peak_end;
  unsigned long peak_max;
  
  // state variables - peak detection
  int threshold;
  int lag;
  bool peak;
  double avg;

  long off_set;

  // initialization
  void initialize(int init_threshold, int init_lag, long init_off_set) {
    // initialize variables
    count_sample = 0;
    count_peaks = 0;
    threshold = init_threshold;
    lag = init_lag;
    avg = 0.0;
    peak = false;
    off_set = init_off_set;

    // initialize arrays
    for (int i = 0; i < length; ++i) {
      data_values[i] = 0;
      data_timestamps[i] = 0;
      peak_values[i] = 0;
      peak_timestamps[i] = 0;
    };

  }

  // update the data
  void update(int currentValue, unsigned long current_millis) {
    // define indices for convenience
    int current_index = count_sample % length;
    int next_index = (count_sample + 1) % length;
    
    // set data values
    data_values[current_index] = currentValue;
    data_timestamps[current_index] = current_millis;
    count_sample += 1;
    
    // compare
    if ( (currentValue - threshold) > avg && !peak ) {
      // start of peak
      peak = true;
      min_value = avg;
      max_value = currentValue;
      peak_start = current_millis;
      peak_max = current_millis;
    }

    if (peak && currentValue > max_value) {
      // during peak
      max_value = currentValue;
      peak_max = current_millis;
    }

    if ( (currentValue - threshold) < avg && peak ) {
      // end of peak
      peak = false;
      peak_end = current_millis;
      // check validity of peak
      if ( (max_value - min_value > 2*threshold) && (peak_end - peak_start > 50) && (peak_end - peak_start < 1000) ) {
        // peak amplitude should be 2 times the threshold
        // peak duration should be between 50 and 1000 milliseconds
        peak_timestamps[count_peaks % length] = peak_max;
        peak_values[count_peaks % length] = max_value;
        count_peaks += 1;

        // reset if the count becomes too large
        if (count_peaks >= 16383) count_peaks = count_peaks % length;  
      }
    }

    // update average
    avg = (currentValue + (lag - 1) * avg) / lag;

    // reset if the count becomes too large
    if (count_sample >= 16383) // 2^14
      count_sample = count_sample % length;
  }


  // method to return json data
  JsonDocument get_json() {
    // Allocate a temporary JsonDocument
    JsonDocument doc;

    // Create the data array
    JsonArray dataValues = doc["data"].to<JsonArray>();  // data samples
    JsonArray peakValues = doc["peaks"].to<JsonArray>(); // peak data
    for (byte i = 0; i < length; i = i + 1) {
      JsonArray measurement = dataValues.add<JsonArray>();
      measurement.add(data_timestamps[(i + count_sample) % length]);
      measurement.add(data_values[(i + count_sample) % length]);
      JsonArray peaks = peakValues.add<JsonArray>();
      peaks.add(peak_timestamps[(i + count_peaks) % length]);
      peaks.add(peak_values[(i + count_peaks) % length]);
    };
    doc["offset"] = off_set;
    return doc; 
  }
};


// ----------------------------------------------------------------------------
// Definition of global variables
// ----------------------------------------------------------------------------

Led onboard_led = { LED_BUILTIN, false, 255 };  // LED_BUILTIN is a variable arduino

AnalogSensor photo_resistor = { A0 };  // A0 is an arduino variable - ESP8266 Analog Pin ADC0 = A0

AsyncWebServer server(HTTP_PORT);

AsyncEventSource events("/events");

int sensorValue = 0;

unsigned long currentMillis = 0;
unsigned long nextMillis = 0;

int previous_count_peaks = 0;

SensorData photo_resistor_data;

long offSet;
timeval tv;
bool cbtime_set = false;


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
  String response;
  serializeJson(photo_resistor_data.get_json(), response);
  request->send(200, "application/json", response);
}

void initWebServer() {
  server.on("/", onRootRequest);
  server.serveStatic("/", LittleFS, "/");
  server.on("/energy", onSensorRequest);
  server.on("/data", onDataRequest);
  
  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
  });
  
  server.addHandler(&events);
  
  server.begin();
  Serial.println("HTTP Server Started");
}

// ----------------------------------------------------------------------------
// Time functions
// ----------------------------------------------------------------------------

void time_is_set (void) {
  gettimeofday (&tv, NULL);
  cbtime_set = true;
  Serial.println("------------------ settimeofday() was called ------------------");
}

// method to get milliseconds
unsigned long getMillis() {
  gettimeofday(&tv, NULL);
  return 1000 * (tv.tv_sec - offSet) + tv.tv_usec / 1000;
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
  
  onboard_led.brightness = 0;
  onboard_led.on = false;
  onboard_led.update();
  
  // --> Here is the IMPORTANT ONE LINER needed in your sketch!
  // This line is needed such that `time` refers to seconds since 1970
  // instead of seconds since starting the program.
  // The sync is done with the ntp-server.
  configTime(MY_TZ, MY_NTP_SERVER); 
  
  settimeofday_cb (time_is_set);
    Serial.println("Setting time: ");
    while (!cbtime_set) {
      Serial.print(".");
      delay(500);
    }
  Serial.println("done!");

  offSet = tv.tv_sec;
  Serial.print("Offset: ");
  Serial.println(offSet);

  photo_resistor_data.initialize(20, 10, offSet);

}


// ----------------------------------------------------------------------------
// Loop
// ----------------------------------------------------------------------------

void loop() {
  // flash the onboard led
  onboard_led.on = millis() % 2000 < 100;
  onboard_led.update();

  // probeer elke 25 ms te meten (minstens)
  // currentMillis = millis();
  currentMillis = getMillis();
  if (currentMillis > nextMillis) {
    // define next time to sample
    nextMillis = currentMillis + delta_time;

    // read the analog in value
    sensorValue = photo_resistor.read();

    photo_resistor_data.update(sensorValue, currentMillis);

    if (previous_count_peaks != photo_resistor_data.count_peaks) {
      // new peak is found
      
      String event_data;
      // Allocate a temporary JsonDocument
      JsonDocument doc;
      doc["timestamp"] = photo_resistor_data.peak_max;
      doc["ipi"] = photo_resistor_data.peak_max - photo_resistor_data.peak_timestamps[(previous_count_peaks - 1) % length];
      doc["offset"] = offSet;

      serializeJson(doc, event_data);

      // send event
      events.send(event_data, "peak", currentMillis);

      // set count_peaks
      previous_count_peaks = photo_resistor_data.count_peaks;
    }

  };

}

