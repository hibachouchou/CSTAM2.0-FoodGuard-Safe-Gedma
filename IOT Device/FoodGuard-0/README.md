## Version V0 – Local Prototype (ESP32 + FreeRTOS)

**Description:**  
Initial prototype focused on local monitoring of food spoilage with LED indicators.

**Features:**
- Reads MQ135 (air quality) and DHT11 (temperature & humidity) sensors.
- Quick ambient calibration at startup (5s).
- LED sequence: Green → Yellow → Red.
- Product status display:
  - 🥦 Green → Fresh
  - ⚠️ Yellow → Attention
  - 💀 Red → Spoiled
- Clear serial output.
- Button controlled via ISR + FreeRTOS semaphore to toggle system.
- Food type adjustment (POULTRY, DAIRY, COOKED, FRUITS, VEG, SALAD) for sensitivity.

**Limitations:**
- No network communication (local only).
- Continuous sensor reading → higher power consumption.
- Red LED does not trigger notifications externally.

**Improvements introduced:**  
- Fully functional FreeRTOS tasks for LEDs and sensors.
- Base for adding network features and energy optimization.

---
