/**
 * @file DEMO_FINAL_PFC.cpp
 * @brief Example project using ESP32 with PZEM004Tv30, DHT11, WiFi, and MQTT.
 * 
 * This project reads sensor data from the DHT11, LDR, and PZEM004Tv30 sensors, and sends the data
 * to an MQTT broker. It also handles relay control via MQTT commands.
 * 
 * @author Paulo Rodrigues, Carlos Santos
 * @date 2024-09-05
 */

#include <PZEM004Tv30.h>    // Library for PZEM004Tv30 sensor
#include <WiFi.h>           // WiFi library
#include <PubSubClient.h>   // MQTT library
#include "MyDHT11.h"        // Custom DHT11 library

#define LDRPIN 1            ///< Pin for the LDR sensor
#define DHT11_PIN 5         ///< Pin for the DHT11 sensor
#define RELAYPIN 21         ///< Pin for the relay
#define LEDPIN 6            ///< Pin for the error LED

// Variables for sensor readings
int temperature = 0;        ///< Temperature value from DHT11
int humidity = 0;           ///< Humidity value from DHT11
int ldr = 0;                ///< LDR sensor reading
int luz = 0;                ///< Light intensity percentage
float lumens = 0.0;         ///< Lumens value
float voltage = 0.0;        ///< Voltage from PZEM004Tv30
float current = 0.0;        ///< Current from PZEM004Tv30
float power = 0.0;          ///< Power from PZEM004Tv30
float energy = 0.0;         ///< Energy consumption from PZEM004Tv30
float frequency = 0.0;      ///< Frequency from PZEM004Tv30

PZEM004Tv30 pzem(Serial1, 16, 17); ///< Object for the PZEM004Tv30 power monitor
MyDHT11 dht(DHT11_PIN);           ///< Object for the custom DHT11 sensor

// Wi-Fi and MQTT credentials
const char* ssid = "e=mc2";        ///< WiFi SSID
const char* password = "1xperiencia";  ///< WiFi password
const char* mqtt_server = "192.168.1.16"; ///< MQTT server IP address

WiFiClient espClient;              ///< WiFi client object
PubSubClient client(espClient);    ///< MQTT client object

unsigned long currentMillis, previousMillis = 0; ///< Time variables for interval control
const long interval = 1000;        ///< Interval in milliseconds for sensor reading

/**
 * @brief Connects to WiFi network.
 * 
 * This function attempts to connect the ESP32 to a WiFi network using the specified
 * SSID and password.
 */
void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
}

/**
 * @brief Callback function for incoming MQTT messages.
 * 
 * Handles messages from subscribed MQTT topics and controls the relay based on the
 * "home/relay/command" topic.
 * 
 * @param topic The topic of the received MQTT message.
 * @param payload The message payload.
 * @param length The length of the payload.
 */
void callback(char* topic, byte* payload, unsigned int length) {
  char message[100];
  strncpy(message, (char*)payload, length);
  message[length] = '\0';

  if (strcmp(topic, "home/relay/command") == 0) {
    if (strcmp(message, "ON") == 0) {
      digitalWrite(RELAYPIN, LOW);
      client.publish("home/relay/state", "ON");
    } else if (strcmp(message, "OFF") == 0) {
      digitalWrite(RELAYPIN, HIGH);
      client.publish("home/relay/state", "OFF");
    }
  }
}

/**
 * @brief Reconnects to MQTT broker.
 * 
 * Attempts to connect to the MQTT broker and subscribes to necessary topics.
 * Turns on the LED if the connection fails, and turns it off if successful.
 */
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("home/sensor/leituras");
      client.subscribe("home/relay/command");
      digitalWrite(LEDPIN, LOW);   // Turn off the LED when connected
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      digitalWrite(LEDPIN, HIGH);  // Turn on the LED if connection fails
      delay(5000);
    }
  }
}

/**
 * @brief Reads temperature and humidity from the DHT11 sensor.
 * 
 * Uses the custom DHT11 library to obtain temperature and humidity values.
 * 
 * @return bool True if the reading is successful, false otherwise.
 */
bool readDht() {
  if (!dht.read()) {
    temperature = NAN;
    humidity = NAN;
    return false;
  }

  temperature = dht.getTemperature();
  humidity = dht.getHumidity();
  return true;
}

/**
 * @brief Converts LDR sensor reading to lumens.
 * 
 * Converts the analog LDR sensor value into lumens based on a voltage divider equation.
 * 
 * @param ldrValue The raw analog value from the LDR sensor.
 * @return float The calculated lumens based on the LDR reading.
 */
float LdrToLumens(int ldrValue) {
  float resistance = 10000.0;  // Assuming a 10k ohm resistor in a voltage divider
  float Vldr = (ldrValue * 3.3) / 4095;
  float ldr_resistance = ((resistance * Vldr)/3.3)/(1 - (Vldr/3.3));

  if (ldr_resistance > 323000) return 2;
  else if (ldr_resistance > 31700) return 8;
  else if (ldr_resistance > 17780) return 13;
  else if (ldr_resistance > 10000) return 20;
  else if (ldr_resistance > 2035) return 200;
  else if (ldr_resistance > 2017) return 250;
  else return 10000;  
}

/**
 * @brief Reads LDR sensor value.
 * 
 * Reads the analog value from the LDR, calculates light intensity and lumens, and checks if the value is valid.
 * 
 * @return bool True if the LDR reading is within the valid range, false otherwise.
 */
bool readLdr() {
  ldr = analogRead(LDRPIN);
  luz = 100 * (4095 - ldr) / 4095;  
  lumens = LdrToLumens(ldr);    
  if (ldr < 0 || ldr > 4095) {
    return false;
  }
  return true;
}

/**
 * @brief Reads values from PZEM004Tv30 sensor.
 * 
 * Reads voltage, current, power, energy, and frequency from the PZEM004Tv30 sensor.
 * 
 * @return bool True if the reading is successful, false if any value is invalid.
 */
bool readPzem() {
  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();
  frequency = pzem.frequency();

  if (isnan(voltage) || isnan(current) || isnan(power) || isnan(energy) || isnan(frequency)) {
    return false;
  }

  return true;
}

/**
 * @brief Publishes sensor data to MQTT broker.
 * 
 * Sends temperature, humidity, luminosity, lumens, voltage, current, power, energy, and frequency as a JSON message.
 */
void publishAllData() {
  char data[300];
  
  snprintf(data, sizeof(data),
           "{\"temperature\":%d,\"humidity\":%d,\"luminosity\":%d,\"lumens\":%.1f,\"voltage\":%.1f,\"current\":%.2f,\"power\":%.2f,\"energy\":%.3f,\"frequency\":%.1f}",
           temperature, humidity, luz, lumens, voltage, current, power, energy, frequency);

  Serial.println("Publishing data:");
  Serial.println(data);

  client.publish("home/sensor/data", data);
}

/**
 * @brief Arduino setup function.
 * 
 * Initializes WiFi, MQTT, and sensor pins, and sets up initial connections.
 */
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(RELAYPIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(RELAYPIN, HIGH);
  digitalWrite(LEDPIN, LOW);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  digitalWrite(LEDPIN, HIGH);
  delay(3000);
  digitalWrite(LEDPIN, LOW);
}

/**
 * @brief Arduino loop function.
 * 
 * Main loop that checks for sensor data, publishes it, and handles reconnections.
 */
void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();              // Attempt to reconnect
  }

  // Check MQTT connection
  if (!client.connected()) {
    reconnect();               // Attempt to reconnect
  }
  client.loop();  // Handle MQTT client

  // Handle sensor readings every interval
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    bool error = false;

    bool dhtSuccess = readDht();  // Read DHT
    bool ldrSuccess = readLdr();  // Read LDR
    bool pzemSuccess = readPzem(); // Read PZEM

    // If any sensor reading fails, set error flag
    if (!dhtSuccess || !ldrSuccess || !pzemSuccess) {
      error = true;
    }

    // Publish only if there are no errors
    if (!error) {
      publishAllData();
    }
  

  // Turn on LED if there's any error (WiFi, MQTT, or sensor failure)
  if (error) {
    digitalWrite(LEDPIN, HIGH);  // Turn on LED for error
  } else {
    digitalWrite(LEDPIN, LOW);   // Turn off LED if no error
  }
  }
}

