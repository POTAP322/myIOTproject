#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

const long measurementInterval = 30 * 1000; // Интервал измерений (в мс)
unsigned long lastMeasurementTime = 0;

int measurementCount = 0;

//Пины датчика дождя
#define RAIN_ANALOG_PIN 34
#define RAIN_DIGITAL_PIN 35

//Инициализация BME280
Adafruit_BME280 bme;

//Переменные для прогноза
float lastTemp = NAN;
float lastHumidity = NAN;
bool wasRaining = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RAIN_DIGITAL_PIN, INPUT);

  if (!bme.begin(0x76)) {
    Serial.println("Ошибка: не найден датчик BME280. Проверь подключение.");
    while (true); // Остановка
  }

  Serial.println("BME280 инициализирован успешно.");
}

void loop() {
  static unsigned long checkRainNow = millis();

  //Проверка дождя
  int rainAnalog = analogRead(RAIN_ANALOG_PIN);
  bool isRaining = rainAnalog < 1500;

  if (isRaining && !wasRaining) {
    Serial.println("🌧️ Дождь начался!");
  }
  wasRaining = isRaining;

  // Основные измерения
  if (millis() - lastMeasurementTime >= measurementInterval) {
    lastMeasurementTime = millis();
    measurementCount++;

    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F; // ГПа

    //Простой прогноз погоды
    String forecast = "Невозможно определить";
    String reason = "";

    if (!isnan(lastTemp) && !isnan(lastHumidity)) {
      float tempChange = temperature - lastTemp;
      float humChange = humidity - lastHumidity;

      if (tempChange < 0 && humChange > 5 || isRaining) {
        forecast = "Возможно дождь 🌧️";
        reason = "Похолодало + ↑влажность";
      } else if (humChange < 2 && abs(tempChange) < 0.5) {
        forecast = "Стабильно ☀️";
        reason = "Без изменений";
      } else {
        forecast = "Ясно ☀️";
        reason = "Нормально";
      }
    } else {
      forecast = "Недостаточно данных";
      reason = "Первое измерение";
    }

    //Сохраняем предыдущее значение для прогноза
    lastTemp = temperature;
    lastHumidity = humidity;

    //Формат времени
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char timeStr[9];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

    // Вывод в Serial Monitor
    Serial.printf("[Измерение #%d] Время: %s\n", measurementCount, timeStr);
    Serial.printf("Температура: %.1f°C\n", temperature);
    Serial.printf("Влажность: %.1f%%\n", humidity);
    Serial.printf("Давление: %.0f hPa\n", pressure);
    Serial.printf("Значение дождя (ADC): %d\n", rainAnalog);
    Serial.printf("Дождь: %s\n", isRaining ? "Да" : "Нет");
    if (reason.length() > 0) {
      Serial.printf("Причина: %s\n", reason.c_str());
    }
    Serial.printf("Прогноз: %s\n", forecast.c_str());
    Serial.println();

    delay(100);
  }

  delay(100); // Небольшая пауза между проверками дождя
}