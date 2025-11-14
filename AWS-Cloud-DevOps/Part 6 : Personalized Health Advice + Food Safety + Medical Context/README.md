# ✅ AWS Personalized Health Advice + Food Safety + Medical Context

---

## 1️⃣ User Authentication (Cognito)

- User signs up / logs in.
- Cognito User Pool stores identity + basic profile.
- Cognito Identity Pool provides temporary IAM credentials for access (S3, API Gateway, etc.).

---

## 2️⃣ Medical Reports Upload + AI Analysis

### A. Upload Medical Report

- React Native App → API Gateway → Lambda → S3 (UserMedicalReports)

### B. Automatic AI Processing

- Triggered by S3 Event → EventBridge → Lambda MedicalAnalyzer
- Lambda MedicalAnalyzer uses AWS services:
  - Textract → extract text from scanned medical PDF/JPG
  - Comprehend Medical → extract:
    - medical conditions (diabetes, anemia, allergies…)
    - medications
    - symptoms
    - risk factors

### C. Store Processed Data

- DynamoDB: MedicalReports
  - userId (PK)
  - reportId (SK)
  - originalFileURL
  - extractedMedicalText
  - medicalEntities (from Comprehend Medical)
  - aiCategory (diagnosis summary)
  - aiRiskScore
  - timestamp

### D. Update User Personal Health Profile

- Lambda UpdateUserHealthProfile aggregates all reports → stores a synthesized health profile:

- DynamoDB: UserHealthProfile
  - userId (PK)
  - allergies: ["lactose", "peanuts"]
  - chronicDiseases: ["asthma", "diabetes"]
  - dietaryRestrictions: ["low sugar", "low sodium"]
  - medications
  - riskScores
  - lastUpdated

---

## 3️⃣ Food Data (Device + Images)

### A. IoT Device Sends Sensor Data

- IoT Core → Rule Engine → Lambda → DynamoDB FoodSensorData

### B. Mobile App Uploads Food Images

- React Native App → API Gateway → S3 (FoodImages) → EventBridge → Lambda ImageAnalyzer
- Lambda ImageAnalyzer uses:
  - Rekognition → labels (meat, chicken, dairy…), quality, mold detection (custom model)
  - SageMaker Endpoint (Custom Model) → “spoiled / not spoiled / warning”

- Store in DynamoDB FoodImageMetadata:
  - productId
  - category (milk, chicken…)
  - aiState (fresh, warning, spoiled)
  - aiConfidence
  - allergensDetected
  - nutritionalTags
  - timestamp

---

## 4️⃣ Health-Aware Personalization Logic

- Lambda PersonalizedRiskEngine is triggered when:
  - New food scan result arrives
  - User’s medical reports update

**Inputs:**

- User health profile (DynamoDB UserHealthProfile)
- Food scan or food image result
- Allergens / nutrition info (from Rekognition + SageMaker)

**Decision Examples:**

- If user is lactose intolerant → warn about milk scan even if fresh
- If user has diabetes → warn about sugary food scans
- If user has allergy → warn about food ingredients
- If food is fresh but humidity is high → give "consume soon" advice

**Output Stored in DynamoDB: UserAlerts**

- userId (PK)
- alertId (SK)
- type: "health" | "food" | "combined"
- message: "Fresh but contains high sugar — not recommended for diabetics"
- severity: "info" | "warning" | "critical"
- timestamp

---

## 5️⃣ Amazon Lex Chatbot (Personalized Recommendations)

- Lex → Lambda ChatbotLogic → DynamoDB
- Chatbot answers:
  - "Is this food safe for me?"
  - "What should I avoid based on my medical reports?"
  - "Why did I get this warning?"
  - "Give me daily dietary tips."

**Lambda ChatbotLogic Tasks:**

- Fetch UserHealthProfile
- Fetch latest food scans
- Fetch UserAlerts
- Generate contextual advice

**Example response:**

- "This yogurt is marked fresh, but you have lactose intolerance. Choose the dairy-free version instead."

---

## 6️⃣ DynamoDB: ChatHistory Table (Recommended)

**Best for:**

- Showing conversation history to the user
- Storing last interactions for context
- Auditing and debugging the assistant
- Reusing chat history to personalize future answers

**Schema (recommended):**

- Table: ChatHistory
- PK: userId
- SK: timestamp
- messageId: uuid
- sender: "user" | "assistant"
- message: text
- intentDetected: (from Lex)
- sessionId
- contextState: optional (mood, warnings, health flags…)

**Why it’s the best:**

- Fast (single-digit ms latency)
- Cheap
- Scales automatically
- Easy to query by user

---

## Final AWS Schema (Unified)

[User Mobile App]  
 |  
 +-----------------------------+  
 |           Cognito           |  
 +-----------------------------+  

┌──────────────┐         ┌──────────────────┐  
│ Food Images  │         │ Medical Reports  │  
└──────┬───────┘         └──────────┬──────┘  
       |                             |  
API Gateway                       API Gateway  
       |                             |  
   S3 (Food)                      S3 (Medical)  
       |                             |  
  EventBridge                     EventBridge  
       |                             |  
Lambda ImageAnalyzer        Lambda MedicalAnalyzer  
(Rekognition + ML)          (Textract + Comprehend Medical)  
       |                             |  
DynamoDB FoodImageMetadata    DynamoDB MedicalReports  
       |                             |  
       +----------+  +-----------------+  
                  |  |  
                  v  v  
        Lambda PersonalizedRiskEngine  
                  |  
        DynamoDB UserAlerts  
        DynamoDB UserHealthProfile  
                  |  
                  v  
              Amazon Lex  
                  |  
            Lambda ChatbotLogic  
                  |  
        Personalized Health Advice
