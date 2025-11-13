## Version V2 – Energy-Optimized Prototype with MQTT

**Description:**  
Optimized for **lower energy consumption**. Sensors and MQTT publishing only trigger once per button activation.

**New Features & Improvements:**
- Reads sensors **once per activation**.
- Publishes MQTT message **once per activation**.
- LED sequence unchanged.
- FreeRTOS task structure retained.
- Ideal for battery-powered setups or low-power operation.

**Comparison with V1:**

| Feature                  | V1           | V2                   |
|--------------------------|--------------|--------------------|
| Sensor reading           | Continuous   | Once per activation |
| MQTT publishing          | Continuous   | Once per activation |
| Power consumption        | High         | Optimized          |
| LED & sequence           | Unchanged    | Unchanged          |

**Limitations:**
- Single measurement per button press → no continuous monitoring.

---
