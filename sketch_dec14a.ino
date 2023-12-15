#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
// Укажите SSID и пароль вашей WiFi-сети
const char* ssid = "iPhone (Артем)";
const char* password = "da090205";

// Укажите хост и порт сервера Azure IoT Hub MQTT
const char* mqttServer = "AlarmSystemDavydenko.azure-devices.net";
const int mqttPort = 8883; // Используйте порт 8883 для MQTT over SSL

// Укажите идентификатор устройства и ключи безопасности
const char* deviceId = "esp32projectdavydenko";
const char* deviceKey = "wlAK6NmBc4fseb7SYEFGIuLZPBRFBhw3HAIoTC9CpaE=";

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// Определение пинов
const int motionSensorPin = 4; // Датчик движения подключен к пину 2
const int soundSensorPin = 0; // Датчик звука подключен к пину 0
const int buzzerPin = 16; // Зумер подключен к пину 16
const int ledPin1 = 26; // Светодиод 1 подключен к пину 26
const int ledPin2 = 27; // Светодиод 2 подключен к пину 27

// Переменные для состояния системы
bool alarmActive = false;
bool sensorsBlocked = false;
unsigned long alarmStartTime = 0;
unsigned long systemStartTime; // Время начала работы системы
const long startDelay = 5000; // Задержка перед началом работы системы (5 секунд)


void setup() {
  Serial.begin(9600); // Начало последовательной связи
  // Настройка пинов
  pinMode(motionSensorPin, INPUT_PULLUP);
  pinMode(soundSensorPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);

  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  
    // Подключение к MQTT
  mqttClient.setServer(mqttServer, mqttPort);
    // Настройка обратного вызова, который срабатывает при получении сообщения от MQTT-брокера
  mqttClient.setCallback(mqttCallback);

  // Попытка подключения к MQTT
  if (mqttClient.connect(deviceId, deviceId, deviceKey)) {
    Serial.println("Connected to Azure IoT Hub");
  } else {
    Serial.println("Failed to connect to Azure IoT Hub");
  }

  systemStartTime = millis(); // Запоминаем время начала работы системы
}
void loop() {
  if (!mqttClient.connected()) {
    reconnectToMQTT();
  }
  mqttClient.loop();
  // Проверяем, прошло ли достаточно времени с момента запуска
  if (millis() - systemStartTime > startDelay) {
    if (!alarmActive && !sensorsBlocked) {
      checkSensors();
    } 

    if (alarmActive) {
      activateAlarm();

      if (millis() - alarmStartTime > 10000) { // Проверка, прошла ли минута
        alarmActive = false;
        sensorsBlocked = false; // Снимаем блокировку с датчиков
        digitalWrite(buzzerPin, LOW); // Выключение зумера
      } 
    }
  }
}

void reconnectToMQTT() {
  // Попытка подключения к MQTT
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(deviceId, deviceId, deviceKey)) {
      Serial.println("Connected to Azure IoT Hub");
    } else {
      Serial.print("Failed to connect, MQTT client state: ");
      Serial.println(mqttClient.state());
      delay(5000); // Ожидание перед следующей попыткой
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  Serial.println(message);

  if (String(topic) == "devices/esp32projectdavydenko/commands") {
    if (message == "activate_system") {
      sensorsBlocked = false;
      Serial.println("System activated");
    } else if (message == "deactivate_system") {
      sensorsBlocked = true;
      alarmActive = false;
      digitalWrite(buzzerPin, LOW); // Отключаем зумер
      Serial.println("System deactivated");
    } else if (message == "disable_alarm") {
      if (alarmActive) {
        alarmActive = false;
        digitalWrite(buzzerPin, LOW); // Отключаем зумер
        Serial.println("Alarm disabled");
      }
    }
  }
}

void publishTelemetry(const char* telemetryData) {
  if (mqttClient.connected()) {
    mqttClient.publish("devices/esp32projectdavydenko/messages/events/", telemetryData);
  }
}

void checkSensors() {
  if (digitalRead(motionSensorPin) == HIGH || digitalRead(soundSensorPin) == HIGH) {
    alarmActive = true;
    sensorsBlocked = true; // Блокируем датчики во время работы сигнализации
    alarmStartTime = millis();
  }
}

void activateAlarm() {
  digitalWrite(buzzerPin, HIGH); // Включение зумера
  // Мигание светодиодами по очереди
  digitalWrite(ledPin1, HIGH);
  delay(250);
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, HIGH);
  delay(250);
  digitalWrite(ledPin2, LOW);
}
