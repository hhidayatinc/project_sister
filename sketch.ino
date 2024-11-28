#include <WiFi.h>  
#include <PubSubClient.h>
#include <DHTesp.h>

const int DHT_PIN = 15;  
const int ACTUATOR_PIN = 2; // Pin aktuator (LED/Relay)
DHTesp dht; 

const char* ssid = "Wokwi-GUEST"; // Wi-Fi SSID
const char* password = "";        // Wi-Fi password
const char* mqtt_server = "test.mosquitto.org"; // MQTT broker URL

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
float temp = 0;
float hum = 0;

// Fungsi untuk mengkategorikan suhu
String getTemperatureCategory(float temp) {
  if (temp < 20) {
    return "Cold";
  } else if (temp >= 20 && temp <= 30) {
    return "Comfortable";
  } else {
    return "Hot";
  }
}

// Fungsi untuk mengkategorikan kelembapan
String getHumidityCategory(float hum) {
  if (hum < 40) {
    return "Dry";
  } else if (hum >= 40 && hum <= 70) {
    return "Normal";
  } else {
    return "Humid";
  }
}

// Fungsi untuk mengevaluasi kualitas udara menggunakan logika fuzzy
String evaluateAirQuality(String temperatureCategory, String humidityCategory) {
  if (temperatureCategory == "Comfortable" && humidityCategory == "Normal") {
    return "Good";
  } else if ((temperatureCategory == "Cold" && humidityCategory == "Dry") ||
             (temperatureCategory == "Hot" && humidityCategory == "Humid")) {
    return "Poor";
  } else {
    return "Moderate";
  }
}

// Fungsi untuk mengontrol aktuator
void controlActuator(String airQuality) {
  if (airQuality == "Good") {
    digitalWrite(ACTUATOR_PIN, LOW); // Matikan aktuator (contoh: LED mati)
    Serial.println("Actuator OFF: Air quality is Good.");
  } else {
    digitalWrite(ACTUATOR_PIN, HIGH); // Nyalakan aktuator (contoh: LED menyala)
    Serial.println("Actuator ON: Air quality is Poor or Moderate.");
  }
}

void setup_wifi() { 
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) { 
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) { 
    Serial.print((char)payload[i]);
  }
}

void reconnect() { 
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected");
      client.publish("/Sister/Publish", "Welcome");
      client.subscribe("/Sister/Subscribe"); 
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(ACTUATOR_PIN, OUTPUT); // Set pin aktuator sebagai output
  digitalWrite(ACTUATOR_PIN, LOW); // Pastikan aktuator mati saat awal

  Serial.begin(115200);
  setup_wifi(); 
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback); 
  dht.setup(DHT_PIN, DHTesp::DHT22);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) { // Perintah publish data
    lastMsg = now;
    TempAndHumidity data = dht.getTempAndHumidity();

    String temp = String(data.temperature, 2);
    String hum = String(data.humidity, 1); 

    // Mengkategorikan suhu dan kelembapan
    String temperatureCategory = getTemperatureCategory(data.temperature);
    String humidityCategory = getHumidityCategory(data.humidity);

    // Evaluasi kualitas udara menggunakan logika fuzzy
    String airQuality = evaluateAirQuality(temperatureCategory, humidityCategory);

    // Kontrol aktuator berdasarkan kualitas udara
    controlActuator(airQuality);

    // Publish suhu, kelembapan, dan kualitas udara
    client.publish("/Sister/temp", temp.c_str()); 
    client.publish("/Sister/hum", hum.c_str()); 
    client.publish("/Sister/air_quality", airQuality.c_str());  // Publish kualitas udara

    // Debug
    Serial.print("Temperature: ");
    Serial.println(temp);
    Serial.print("Humidity: ");
    Serial.println(hum);
    Serial.print("Air Quality: ");
    Serial.println(airQuality);
  }
}
