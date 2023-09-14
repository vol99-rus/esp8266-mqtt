#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

// Настройки сети Wi-Fi
const char* ssid = "";
const char* password = "";

// Настройки MQTT
const char* mqttServer = "";
const int mqttPort = ;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* mqtt_topic = "";

// Настройки светодиодной ленты
#define STATUS_LED  D0
#define LED_PIN     D5
#define RELAY_PIN   D6
#define NUM_LEDS    120
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// Создаем объекты для работы с Wi-Fi, MQTT и светодиодной лентой
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(5000);
    Serial.print(".");
    // ESP.restart();
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}   

void reconnectToMqttBroker() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect("ESP8266Client", mqttUser, mqttPassword)) {
      Serial.println("Reconnected to MQTT broker");
      mqttClient.subscribe("#");
    } else {
      Serial.print("Failed to reconnect to MQTT broker, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void setAllLedsColor(String hexColor) {
  // Удаление символа # из hex-кода цвета
  hexColor.replace("#", "");

  // Преобразование hex-кода в объект CRGB
  CRGB color = CRGB::Black;
  sscanf(hexColor.c_str(), "%06x", &color);

  // Установка цвета для всех светодиодов
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
    // Получаем сообщение из топика управления
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  // Преобразование char* в объект типа String
  String topicString = String(topic);

  // Разбираем полученное сообщение
  if (topicString == "status") {
    if (message == "on") {
      // Включаем светодиодную ленту
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("Включаем ленту");
    } 
    else if (message == "off") {
      // Выключаем светодиодную ленту
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("Выключаем ленту");
    }
  }  
  else if (topicString == "color") {
    // Установка цвета светодиодной ленты на основе полученного hex-кода
    setAllLedsColor(message);
  }
  else if (topicString == "brightness") {
    // меняем яркость ленты
    int BRIGHTNESS = message.toInt();  // Конвертация строки в int
    FastLED.setBrightness(BRIGHTNESS);
  }
    else if (topicString == "effect") {
      if (message == "rainbow") rainbowCycle();
      else if (message == "color wipe") colorWipe(CRGB::Blue, 50);
      else if (message == "twinkle") twinkle(CRGB::Red, 10, 100);
      else if (message == "fire") fire(55, 120, 50);
      else if (message == "larsonscanner") larsonScanner(CRGB::Green, 3, 100);
      else if (message == "no effect") setAllLedsColor("#FFFFFF");
  } 
  else {
    Serial.println("Неизвестный топик");
  }  
  FastLED.show();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  // Инициализация светодиодной ленты
  byte BRIGHTNESS = 96;
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.setBrightness(BRIGHTNESS);
  // setAllLedsColor("#FFFFFF");
  // FastLED.show();

   // Настройка пина на выход
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pinMode(STATUS_LED, OUTPUT);

  // Подключение к Wi-Fi сети
  setup_wifi();

  // Инициализация OTA
  ArduinoOTA.onStart([]() {
    Serial.println("Начало обновления...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nОбновление завершено.");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Ошибка обновления[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Ошибка аутентификации");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Ошибка начала обновления");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Ошибка подключения");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Ошибка приема");
    else if (error == OTA_END_ERROR) Serial.println("Ошибка завершения обновления");
  });

  ArduinoOTA.begin();

  // Подключение к MQTT брокеру
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);
}

void loop() {
  ArduinoOTA.handle(); // Обработка событий OTA
  
  // Проверка статуса подключения к MQTT брокеру
  if (!mqttClient.connected()) {
    reconnectToMqttBroker();
  }
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastMsg > 60000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "start time %ld min", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    mqttClient.publish("testTopic", msg);
  }
}

//эффекты
//Rainbow Cycle (Цикл радуги):
void rainbowCycle() {
  uint8_t hue = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue++, 255, 255);
  }
  FastLED.show();
  FastLED.delay(20);
}

//Color Wipe (Заливка цветом):
void colorWipe(CRGB color, int wait) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
    FastLED.show();
    FastLED.delay(wait);
  }
}

//Larson Scanner (Эффект "Бегущая строка"):
void larsonScanner(CRGB color, int tailSize, int speed) {
  for (int i = 0; i < NUM_LEDS + tailSize; i++) {
    for (int j = 0; j < NUM_LEDS; j++) {
      if (j == i || j == i - tailSize) {
        leds[j] = color;
      } else {
        leds[j] = CRGB::Black;
      }
    }
    FastLED.show();
    FastLED.delay(speed);
  }
}

//Twinkle (Мерцание):
void twinkle(CRGB color, int count, int speed) {
  for (int i = 0; i < count; i++) {
    int randomLed = random(NUM_LEDS);
    leds[randomLed] = color;
    FastLED.show();
    delay(speed);
    leds[randomLed] = CRGB::Black;
    FastLED.show();
  }
}

//Fire (Огонь):
void fire(int cooling, int sparking, int speed) {
  static byte heat[NUM_LEDS];
  
  // Cooling
  for (int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8(heat[i], random(0, ((cooling * 10) / NUM_LEDS) + 2));
  }
  
  // Sparking
  for (int i = 0; i < NUM_LEDS; i++) {
    if (random(255) < sparking) {
      int cooldown = random(0, ((sparking * 10) / NUM_LEDS) + 2);
      heat[i] = qadd8(heat[i], cooldown);
    }
  }
  
  // Heat to LED color conversion
  for (int i = 0; i < NUM_LEDS; i++) {
    CRGB color = HeatColor(heat[i]);
    int pixelIndex = (NUM_LEDS - 1) - i;
    leds[pixelIndex] = color;
  }
  
  FastLED.show();
  FastLED.delay(speed);
}
