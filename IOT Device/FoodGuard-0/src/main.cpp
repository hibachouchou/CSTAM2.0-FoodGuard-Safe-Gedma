// Prototype Food Spoilage - FreeRTOS - ESP32
#include <Arduino.h>
#include <DHT.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// --- Broches ---
#define LED_GREEN 25
#define LED_YELLOW 26
#define LED_RED 27
#define BUTTON_PIN 5
#define MQ135_PIN 34
#define DHT_PIN 4
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

// --- FreeRTOS ---
SemaphoreHandle_t xMutex;
SemaphoreHandle_t semLED;      // donné par ISR du bouton

// --- Variables système ---
volatile bool running = false;      // true = système ON (séquence + monitoring)
volatile bool inSequence = false;   // séquence LED en cours
float baselineMQ = 1.0;             // baseline (valeur analogique, initialisée)
const uint32_t CALIB_MS = 5000;     // durée calibration en ms (5s)

// Seuils et facteurs (ajuste après tests)
const float RATIO_YELLOW = 1.20f;
const float RATIO_RED    = 1.50f;
const int   DELTA_YELLOW = 150;
const int   DELTA_RED    = 400;

// Sensibilité par aliment
enum FoodType { GENERIC=0, POULTRY, DAIRY, COOKED, FRUITS, VEG, SALAD };
float foodFactor = 1.0f;

// ISR bouton
void IRAM_ATTR buttonISR() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(semLED, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

void allOff() {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
}

void setLEDGreen() { digitalWrite(LED_GREEN, HIGH); digitalWrite(LED_YELLOW, LOW); digitalWrite(LED_RED, LOW); }
void setLEDYellow(){ digitalWrite(LED_GREEN, LOW);  digitalWrite(LED_YELLOW, HIGH); digitalWrite(LED_RED, LOW); }
void setLEDRed()   { digitalWrite(LED_GREEN, LOW);  digitalWrite(LED_YELLOW, LOW);  digitalWrite(LED_RED, HIGH); }

// --- Tâche LED & contrôle ---
void taskLED(void *pvParameters) {
  for (;;) {
    if (xSemaphoreTake(semLED, portMAX_DELAY) == pdTRUE) {
      running = !running;

      if (running) {
        // --- Calibration ambiante rapide ---
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
          Serial.println(">> ON : Calibration ambiante (5s) - ne pas approcher le capteur du plat maintenant");
          xSemaphoreGive(xMutex);
        }

        unsigned long start = millis();
        uint32_t count = 0;
        uint32_t sum = 0;
        while (millis() - start < CALIB_MS) {
          int v = analogRead(MQ135_PIN);
          sum += (uint32_t)v;
          count++;
          vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        if (count > 0) baselineMQ = (float)sum / (float)count;
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
          Serial.print("Calibration done. baseline_MQ = ");
          Serial.println((int)baselineMQ);
          Serial.println(">> Approchez maintenant les capteurs du produit. Séquence LED va démarrer.");
          xSemaphoreGive(xMutex);
        }

        // --- Séquence LED ---
        inSequence = true;
        setLEDGreen(); vTaskDelay(2000 / portTICK_PERIOD_MS);
        setLEDYellow(); vTaskDelay(2000 / portTICK_PERIOD_MS);
        setLEDRed();

        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
          Serial.println(">> LED rouge allumée : monitoring des capteurs déclenché");
          xSemaphoreGive(xMutex);
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
        allOff();
        inSequence = false;

      } else {
        allOff();
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
          Serial.println(">> OFF : arrêt du système");
          xSemaphoreGive(xMutex);
        }
      }
    }
  }
}

// --- Tâche capteurs ---
void taskSensors(void *pvParameters) {
  FoodType currentFood = GENERIC;

  for (;;) {
    if (!running) {
      vTaskDelay(200 / portTICK_PERIOD_MS);
      continue;
    }

    int mqValue = analogRead(MQ135_PIN);
    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();

    bool tempOk = !isnan(temp);
    bool humOk  = !isnan(hum);

    float mqRatio = 1.0f;
    float mqDelta = 0.0f;
    if (baselineMQ > 0.1f) {
      mqRatio = (float)mqValue / baselineMQ;
      mqDelta = (float)mqValue - baselineMQ;
    }

    switch (currentFood) {
      case POULTRY: foodFactor = 0.85f; break;
      case DAIRY:   foodFactor = 0.88f; break;
      case COOKED:  foodFactor = 0.9f;  break;
      case FRUITS:  foodFactor = 0.98f; break;
      case VEG:     foodFactor = 0.98f; break;
      case SALAD:   foodFactor = 0.98f; break;
      default:      foodFactor = 1.0f;  break;
    }

    float eff_yellow = RATIO_YELLOW * foodFactor;
    float eff_red    = RATIO_RED * foodFactor;
    int   eff_delta_y = (int)(DELTA_YELLOW * foodFactor);
    int   eff_delta_r = (int)(DELTA_RED * foodFactor);

    bool tempRisk = (tempOk && temp >= 8.0f);
    bool humRisk  = (humOk && hum >= 85.0f);

    bool isRed = false;
    bool isYellow = false;

    if (mqRatio >= eff_red || mqDelta >= eff_delta_r) isRed = true;
    else if (mqRatio >= eff_yellow || mqDelta >= eff_delta_y) isYellow = true;

    if (!isRed) {
      if ((tempRisk && (mqRatio >= (eff_yellow * 0.95f))) || (humRisk && (mqRatio >= (eff_yellow * 0.95f)))) {
        isYellow = true;
      }
      if (tempRisk && mqRatio >= (eff_red * 0.95f)) isRed = true;
    }

    // --- Affichage clair et stylé ---
    if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
      Serial.println("\n--------------------------------");
      Serial.print("📊 MQ135 Value : "); Serial.print(mqValue);
      Serial.print(" | Ratio: "); Serial.print(mqRatio, 2);
      Serial.print(" | Δ: "); Serial.print((int)mqDelta);

      Serial.print("\n🌡️ Temp: ");
      if (tempOk) { Serial.print(temp,1); Serial.print(" °C"); } else Serial.print("Error");
      Serial.print(" | 💧 Hum: ");
      if (humOk) { Serial.print(hum,1); Serial.print(" %"); } else Serial.print("Error");

      Serial.print("\n🔎 État du produit : ");

      if (isRed) { 
        Serial.println("💀 SPOILED (Rouge) ⚠️");
        setLEDRed(); 
      }
      else if (isYellow) { 
        Serial.println("⚠️ ATTENTION (Jaune)");
        setLEDYellow(); 
      }
      else { 
        Serial.println("🥦 FRAIS (Vert) ✅");
        setLEDGreen(); 
      }

      Serial.println("--------------------------------\n");
      xSemaphoreGive(xMutex);
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(MQ135_PIN, INPUT);
  dht.begin();

  allOff();

  xMutex = xSemaphoreCreateMutex();
  semLED = xSemaphoreCreateBinary();

  attachInterrupt(BUTTON_PIN, buttonISR, FALLING);

  xTaskCreatePinnedToCore(taskLED, "LED Task", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskSensors, "Sensors Task", 4096, NULL, 1, NULL, 0);

  Serial.println("=== 🍱 Food Spoilage Prototype READY ===");
  Serial.println("Procédure: placer l'ESP32 en air ambiant, appuyer ON, attendre calibration, rapprocher capteurs du produit.");
}

void loop() { /* vide : tout en FreeRTOS */ }
