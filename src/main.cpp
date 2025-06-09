#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi
const char* ssid = "UPC2DA7B8";
const char* password = "9EvxEjfwxkzq";

// MQTT
const char* mqtt_server = "test.mosquitto.org";  // Change if using a different broker
const int mqtt_port = 1883;  
const char* SUBTOPIC_TEMPERATURE = "esp32-PKWIEIK/temperatura/";
const char* SUBTOPIC_PRESSURE = "esp32-PKWIEIK/cisnienie/";
const char* SUBTOPIC_ALTITUDE = "esp32-PKWIEIK/wysokosc/";

const char* PRESSURE_API = "https://api.open-meteo.com/v1/forecast?latitude=54.611667&longitude=18.808056&current=pressure_msl";
String jsonBuffer;

// Initialize Wi-Fi and MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password, 6);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    String clientId = "esp32-sol-clientId-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected");
      client.subscribe(SUBTOPIC_TEMPERATURE);
      client.subscribe(SUBTOPIC_PRESSURE);
      client.subscribe(SUBTOPIC_ALTITUDE);

    } else {
      delay(5000);
    }
  }
}

#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

Adafruit_BMP280 bmp; // I2C
//Adafruit_BMP280 bmp(BMP_CS);
//Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO,  BMP_SCK);

void setup() {
  Serial.begin(9600);
  while ( !Serial ) delay(100); 
  Serial.println(F("BMP280 test"));
  unsigned status;
  //status = bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);
  status = bmp.begin(0x76);
  
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
    }

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     // Tryb operacji.
                  Adafruit_BMP280::SAMPLING_X2,     // Próbkowanie temperatury 
                  Adafruit_BMP280::SAMPLING_X16,    // Próbkowanie ciśnienia
                  Adafruit_BMP280::FILTER_X16,      // Filtrowanie 
                  Adafruit_BMP280::STANDBY_MS_500); // Czas gotowości
}

void loop() {
   if (!client.connected()) {
   reconnect();
  }
    float pressure_api = 1011.9;
    HTTPClient http;
    http.begin(PRESSURE_API);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
        String payload = http.getString();

        // Parsowanie JSON
        JsonDocument doc; 
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            pressure_api = doc["current"]["pressure_msl"];
            Serial.print("API Pressure: ");
            Serial.print(pressure_api);
            Serial.println(" hPa");
        } else {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.f_str());
        }
    } else {
        Serial.print("HTTP GET Request failed, error: ");
        Serial.println(httpResponseCode);
    }
    http.end();

    Serial.print(F("Temperature = "));
    float temperature = bmp.readTemperature();
    Serial.print(temperature);
    Serial.println(" *C");

    Serial.print(F("Pressure = "));
    float pressure = bmp.readPressure();
    pressure /= 100;
    Serial.print(pressure);
    Serial.println(" Pa");

    Serial.print(F("Approx altitude = "));
    float altitude = bmp.readAltitude(pressure_api); // aktualnie ciśnienie na poziomie morza
    Serial.print(altitude);
    Serial.println(" m");

    Serial.println();

    char result[8]; 
    dtostrf(temperature, 6, 2, result); 
    client.publish(SUBTOPIC_TEMPERATURE,result, 10);

    dtostrf(pressure, 6, 2, result); 
    client.publish(SUBTOPIC_PRESSURE,result, 10);

    dtostrf(altitude, 6, 2, result);
    client.publish(SUBTOPIC_ALTITUDE,result, 10);

    client.loop();
    delay(1000);
}