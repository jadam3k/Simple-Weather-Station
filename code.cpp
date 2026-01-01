#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ================= WiFi =================
const char* ssid = "No i wont give you my ssid";
const char* password = "No i wont give you my pass";

// ================= LED =================
const int greenLed = 17;
const int redLed   = 4;
const int whiteLed = 16;

// ================= BUTTON =================
#define BUTTON_PIN 27
bool showSensorScreen = false;
bool lastButtonState = HIGH;

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
#define SDA_PIN       21
#define SCL_PIN       22
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ================= NTP =================
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);

// ================= WEATHER =================
float currentTemp = 0.0;
const float krakowLat = 50.0647;
const float krakowLon = 19.9450;
unsigned long lastWeatherMillis = 0;
const long weatherInterval = 60000;

// ================= DHT SENSOR =================
#define DHTPIN 25       // BEZPIECZNY PIN dla ESP32
#define DHTTYPE DHT11   // ZMIEN NA DHT11 JESLI MASZ DHT11
DHT dht(DHTPIN, DHTTYPE);

// ================= RED LED BLINK =================
unsigned long previousMillis = 0;
const long interval = 500;
bool redState = LOW;

// =================================================

void setup() {
  Serial.begin(115200);
  Serial.println("START OK");

  pinMode(greenLed, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(whiteLed, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(greenLed, LOW);
  digitalWrite(redLed, HIGH);
  digitalWrite(whiteLed, LOW);

  // OLED
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED nie znaleziony!");
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // DHT
  dht.begin();

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Laczenie z WiFi...");
}

// =================================================

void loop() {
  // ===== BUTTON =====
  bool buttonState = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && buttonState == LOW) {
    showSensorScreen = !showSensorScreen;
    Serial.println("ZMIANA EKRANU");
    delay(200); // debounce
  }
  lastButtonState = buttonState;

  // ===== WIFI LEDS =====
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(greenLed, HIGH);
    digitalWrite(redLed, LOW);
    digitalWrite(whiteLed, HIGH);
    timeClient.update();

    unsigned long currentMillis = millis();
    if (currentMillis - lastWeatherMillis > weatherInterval) {
      fetchWeather();
      lastWeatherMillis = currentMillis;
    }
  } else {
    digitalWrite(whiteLed, LOW);
    digitalWrite(greenLed, HIGH);

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      redState = !redState;
      digitalWrite(redLed, redState);
    }
  }

  // ===== OLED ZAWSZE =====
  if (showSensorScreen) {
    displaySensorData();
  } else {
    displayData();
  }

  delay(1000);
}

// =================================================
// POBIERANIE POGODY
void fetchWeather() {
  HTTPClient http;
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" +
               String(krakowLat, 6) +
               "&longitude=" + String(krakowLon, 6) +
               "&current_weather=true";

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc;

    if (!deserializeJson(doc, payload)) {
      currentTemp = doc["current_weather"]["temperature"].as<float>();
    }
  }
  http.end();
}

// =================================================
// EKRAN 1 – CZAS + POGODA
void displayData() {
  display.clearDisplay();

  int h = timeClient.getHours();
  int m = timeClient.getMinutes();
  int s = timeClient.getSeconds();

  String daysOfWeek[7] = {"Niedz", "Pon", "Wt", "Sr", "Czw", "Pt", "Sob"};
  int d = (timeClient.getDay() + 6) % 7;

  display.setCursor(0, 0);
  display.print("Czas ");
  if (h < 10) display.print("0");
  display.print(h); display.print(":");
  if (m < 10) display.print("0");
  display.print(m); display.print(":");
  if (s < 10) display.print("0");
  display.print(s);

  display.setCursor(0, 10);
  display.print("Dzien ");
  display.print(daysOfWeek[d]);

  display.setCursor(0, 20);
  display.print("Temp Krakow: ");
  display.print(currentTemp);
  display.print(" C");

  display.display();
}

// =================================================
// EKRAN 2 – SENSOR TEMP + WILG
void displaySensorData() {
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Sensor T&H");

  if (isnan(temp) || isnan(hum)) {
    display.setCursor(0, 14);
    display.print("Blad odczytu");
  } else {
    display.setCursor(0, 12);
    display.print("Temp: ");
    display.print(temp);
    display.print(" C");

    display.setCursor(0, 22);
    display.print("Wilg: ");
    display.print(hum);
    display.print(" %");
  }

  display.display();
}
