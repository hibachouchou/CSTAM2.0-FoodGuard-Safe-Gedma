#include <Arduino.h>
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// --- Pin Definitions ---
#define LED_GREEN 25
#define LED_YELLOW 26
#define LED_RED 27
#define BUTTON_PIN 5
#define MQ135_PIN 34
#define DHT_PIN 4
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

// --- FreeRTOS Semaphores ---
SemaphoreHandle_t xMutex;
SemaphoreHandle_t semLED;

// --- System Variables ---
volatile bool running = false;
volatile bool inSequence = false;
float baselineMQ = 1.0;
const uint32_t CALIB_MS = 5000;

// --- Thresholds and Factors ---
const float RATIO_YELLOW = 1.20f;
const float RATIO_RED    = 1.50f;
const int   DELTA_YELLOW = 150;
const int   DELTA_RED    = 400;

// Food types
enum FoodType { GENERIC=0, POULTRY, DAIRY, COOKED, FRUITS, VEG, SALAD };
float foodFactor = 1.0;

// --- WiFi & MQTT ---
const char* ssid = "TT_2850";
const char* password = "Fake2024#-World2024#WithAllMyHate2024";
const char* mqtt_server = "192.168.1.13";
const int mqtt_port = 1883;
const char* topic = "food/monitor";

WiFiClient espClient;
PubSubClient client(espClient);

// --- LED Functions ---
void allOff() {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
}
void setLEDGreen()  { digitalWrite(LED_GREEN, HIGH);  digitalWrite(LED_YELLOW, LOW); digitalWrite(LED_RED, LOW); }
void setLEDYellow() { digitalWrite(LED_GREEN, LOW);   digitalWrite(LED_YELLOW, HIGH); digitalWrite(LED_RED, LOW); }
void setLEDRed()    { digitalWrite(LED_GREEN, LOW);   digitalWrite(LED_YELLOW, LOW); digitalWrite(LED_RED, HIGH); }

// --- Button ISR ---
void IRAM_ATTR buttonISR() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(semLED, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

// --- WiFi & MQTT ---
void setup_wifi() {
  Serial.println();
  Serial.print("Connecting to WiFi: "); Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(); Serial.print("WiFi connected. IP: "); Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if(client.connect("ESP32_FoodMonitor")) Serial.println("connected");
    else { Serial.print("failed, rc="); Serial.print(client.state()); Serial.println(" try again in 2s"); delay(2000); }
  }
}

// --- LED Task ---
void taskLED(void *pvParameters) {
  for(;;) {
    if(xSemaphoreTake(semLED, portMAX_DELAY) == pdTRUE) {
      running = !running;

      if(running) {
        Serial.println(">>> System ON");

        // Calibration
        if(xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
          Serial.println(">> Calibrating ambient MQ135 (5s)...");
          xSemaphoreGive(xMutex);
        }

        unsigned long start = millis();
        uint32_t sum=0, count=0;
        while(millis()-start<CALIB_MS){
          sum += analogRead(MQ135_PIN); count++;
          vTaskDelay(100/portTICK_PERIOD_MS);
        }
        if(count>0) baselineMQ = (float)sum/count;

        if(xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
          Serial.print("Calibration done: baselineMQ = "); Serial.println((int)baselineMQ);
          Serial.println(">> Approach sensor to food. LED sequence starts.");
          xSemaphoreGive(xMutex);
        }

        // LED Sequence
        inSequence = true;
        setLEDGreen();  vTaskDelay(2000/portTICK_PERIOD_MS);
        setLEDYellow(); vTaskDelay(2000/portTICK_PERIOD_MS);
        setLEDRed();    vTaskDelay(2000/portTICK_PERIOD_MS);
        allOff();
        inSequence = false;

      } else {
        Serial.println(">>> System OFF");
        allOff();
      }
    }
  }
}

// --- Sensor Task: read & publish only once per activation ---
void taskSensors(void *pvParameters) {
  FoodType currentFood = GENERIC;
  for(;;){
    if(!running){ vTaskDelay(200/portTICK_PERIOD_MS); continue; }

    // Read sensors once
    int mqValue = analogRead(MQ135_PIN);
    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();

    bool tempOk = !isnan(temp);
    bool humOk  = !isnan(hum);

    float mqRatio = (baselineMQ>0.1f)? (float)mqValue/baselineMQ : 1.0f;
    float mqDelta = mqValue - baselineMQ;

    // Adjust food factor
    switch(currentFood){
      case POULTRY: foodFactor=0.85f; break;
      case DAIRY:   foodFactor=0.88f; break;
      case COOKED:  foodFactor=0.9f;  break;
      case FRUITS:  foodFactor=0.98f; break;
      case VEG:     foodFactor=0.98f; break;
      case SALAD:   foodFactor=0.98f; break;
      default: foodFactor=1.0f; break;
    }

    float eff_yellow = RATIO_YELLOW*foodFactor;
    float eff_red    = RATIO_RED*foodFactor;
    int eff_delta_y  = (int)(DELTA_YELLOW*foodFactor);
    int eff_delta_r  = (int)(DELTA_RED*foodFactor);

    bool tempRisk = tempOk && temp>=8.0f;
    bool humRisk  = humOk && hum>=85.0f;
    bool isRed=false, isYellow=false;

    if(mqRatio>=eff_red || mqDelta>=eff_delta_r) isRed=true;
    else if(mqRatio>=eff_yellow || mqDelta>=eff_delta_y) isYellow=true;

    if(!isRed){
      if((tempRisk && mqRatio>=(eff_yellow*0.95f)) || (humRisk && mqRatio>=(eff_yellow*0.95f))) isYellow=true;
      if(tempRisk && mqRatio>=(eff_red*0.95f)) isRed=true;
    }

    // Critical section
    if(xSemaphoreTake(xMutex, portMAX_DELAY)==pdTRUE){
      String stateStr = isRed?"SPOILED":isYellow?"ATTENTION":"FRAIS";
      if(isRed) setLEDRed();
      else if(isYellow) setLEDYellow();
      else setLEDGreen();

      Serial.println("\n----- Food Measurement -----");
      Serial.print("MQ: "); Serial.print(mqValue);
      Serial.print(" | Temp: "); if(tempOk) Serial.print(temp,1); else Serial.print("Err");
      Serial.print(" | Hum: "); if(humOk) Serial.print(hum,1); else Serial.print("Err");
      Serial.print(" => "); Serial.println(stateStr);
      Serial.println("----------------------------\n");

      // Publish once
      if(client.connected()){
        String payload = String("{\"state\":\"")+stateStr+
                         String("\",\"mq\":")+mqValue+
                         String(",\"temp\":")+temp+
                         String(",\"hum\":")+hum+"}";
        client.publish(topic, payload.c_str(), true); // retained
      }

      xSemaphoreGive(xMutex);
    }

    // Stop sensor task until next activation
    running = false;
    vTaskDelay(portMAX_DELAY); // sleep indefinitely until next button press
  }
}

// --- Setup & Loop ---
void setup() {
  Serial.begin(115200);
  pinMode(LED_GREEN, OUTPUT); pinMode(LED_YELLOW, OUTPUT); pinMode(LED_RED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP); pinMode(MQ135_PIN, INPUT);
  dht.begin();

  allOff();
  xMutex = xSemaphoreCreateMutex();
  semLED = xSemaphoreCreateBinary();
  attachInterrupt(BUTTON_PIN, buttonISR, FALLING);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  xTaskCreatePinnedToCore(taskLED, "LED Task", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskSensors, "Sensor Task", 4096, NULL, 1, NULL, 0);

  Serial.println("=== 🍱 Food Spoilage Prototype (Energy-Optimized) READY ===");
}

void loop() {
  if(!client.connected()) reconnectMQTT();
  client.loop();
}
