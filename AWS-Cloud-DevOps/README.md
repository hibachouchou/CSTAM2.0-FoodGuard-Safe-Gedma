# FoodGuard AWS Architecture Diagram

![General](full.jpg)
![Best Practice -1](BP2.jpg)
![Best Practice -2](BP1.jpg)


# Part 1 : AWS IoT Integration
This guide explains how to connect the **ESP32 FoodGuard Device** to **AWS IoT Core**, process sensor data using **Lambda**, and store records inside **DynamoDB**.

It covers:

* Creating the IoT Thing + certificates
* Setting up DynamoDB
* Creating the Lambda function
* Creating the IoT Rule
* Publishing MQTT sensor data securely from the ESP32
* Final expected JSON payload + DynamoDB record format

---

## Step 1 — AWS IoT Core Configuration

### 1. Create an IoT Thing
AWS Console → **IoT Core** → Manage → **Things** → **Create Thing**

* **Thing Name:** `FoodGuardDevice`
* **Type:** Single device
* **Create certificates (Download ALL files):**
    * `device-certificate.pem.crt`
    * `private.pem.key`
    * `public.pem.key`
    * `Amazon Root CA`

### 2. Attach an IoT Policy
Create a policy allowing the ESP32 to communicate with AWS IoT:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "iot:Publish",
        "iot:Subscribe",
        "iot:Receive",
        "iot:Connect"
      ],
      "Resource": "*"
    }
  ]
}
```
Attach this policy to your certificate.

### 3. Note Your MQTT Endpoint
AWS Console → **IoT Core** → **Settings**

**Example:**
```
a3k7odshaiipe6-ats.iot.us-east-1.amazonaws.com
```
---

## Step 2 — DynamoDB Setup 💾

Go to → **DynamoDB** → **Create Table**

* **Table name:** `SensorFoodSpoilageData`
* **Partition Key:** `deviceId` (String)
* **Sort Key:** `timestamp` (Number)

### Recommended Attributes

| Attribute | Type | Description |
| :--- | :--- | :--- |
| `deviceId` | String | Unique ESP32 device |
| `timestamp` | Number | UNIX time (seconds or ms) |
| `temperature` | Number | Temperature reading |
| `humidity` | Number | Humidity reading |
| `gas` | Number | MQ sensor value |
| `state` | String | FRESH / WARNING / SPOILED |
| `foodType` | String | Optional category |
| `alertSent` | Bool | Tracks if user alert was sent |
| `notes` | String | Optional notes |

---

## Step 3 — Lambda Function (`ProcessFoodSensorData`) ☁️

Create Lambda
AWS → **Lambda** → **Create Function**

* **Name:** `ProcessFoodSensorData`
* **Runtime:** `Python 3.12`
* **Permissions:** Allow **DynamoDB write access**

### Example Lambda Code

```python
import json
import boto3

dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('SensorFoodSpoilageData')

def lambda_handler(event, context):
    for record in event["records"]:
        payload = json.loads(record["value"])

        table.put_item(Item=payload)

    return {"status": "ok"}
```

## Step 4 — IoT Rule (`ForwardSensorDataToLambda`) 🚦

AWS **IoT Core** → Message Routing → **Create Rule**

* **Rule name:** `ForwardSensorDataToLambda`
* **SQL Query:**

    ```sql
    SELECT * FROM 'food/monitor'
    ```
* **Action:** Invoke **Lambda Function**
    * **Choose** → `ProcessFoodSensorData`

### What Is an IoT Rule? (Quick Explanation)

An **IoT Rule** acts like a traffic director. Your ESP32 publishes messages → AWS IoT receives them → the rule decides where they go:

* To Lambda
* To DynamoDB
* To SNS (alerts)
* To S3
* etc.

**Without the rule:** Your ESP32 can still publish, but AWS does nothing with the data. It stays inside IoT Core and is never processed.

---

## Step 5 — ESP32 Code (MQTT Publish to AWS IoT) 💻

Use `WiFiClientSecure` + certificates

```cpp
#include <WiFiClientSecure.h>
WiFiClientSecure espClient;
PubSubClient client(espClient);
```
### AWS MQTT Endpoint

```cpp
const char* mqtt_server = "YOUR_ENDPOINT.amazonaws.com";
const int mqtt_port = 8883;
```
### Final JSON Payload Sent to AWS 📡

Your topic is:
food/monitor


**ESP32 Payload Code**

```cpp
String payload = "{";
payload += "\"deviceId\":\"foodguard-01\",";
payload += "\"timestamp\":" + String(millis()/1000) + ",";
payload += "\"temperature\":" + String(temp) + ",";
payload += "\"humidity\":" + String(hum) + ",";
payload += "\"gas\":" + String(mqValue) + ",";
payload += "\"state\":\"" + stateStr + "\"";
payload += "}";

```
### MQTT Publish
```cpp
client.publish("food/monitor", payload.c_str(), true);

```
### MQTT Publish
```cpp

client.publish("food/monitor", payload.c_str(), true);
```
Topic: food/monitor

Retained = true keeps last value accessible for new subscribers.

### Final Expected DynamoDB Item (Example)
```JSON

{
  "deviceId": "foodguard-01",
  "timestamp": 1731582200,
  "temperature": 12.4,
  "humidity": 76,
  "gas": 430,
  "state": "SPOILED",
  "foodType": "DAIRY",
  "alertSent": false,
  "notes": ""
}
```