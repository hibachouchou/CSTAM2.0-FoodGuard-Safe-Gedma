## Version V1 – Prototype with MQTT

**Description:**  
Adds **WiFi connectivity** and **MQTT publishing**, enabling remote monitoring of food status.

**New Features:**
- Connects to WiFi and MQTT broker.
- Publishes sensor data (MQ value, temperature, humidity, product state) to MQTT topic `food/monitor`.
- FreeRTOS tasks for LED and sensor monitoring remain intact.
- Food type sensitivity adjustments still included.

**Improvements over V0:**
- Remote monitoring via MQTT.
- MQTT reconnection logic for network robustness.
- JSON structured data publishing.
- Modular code ready for additional sensors or network features.

**Limitations:**
- Continuous sensor reading → still high power consumption.
- Data published continuously even if state does not change.

---
![Output](output.png)