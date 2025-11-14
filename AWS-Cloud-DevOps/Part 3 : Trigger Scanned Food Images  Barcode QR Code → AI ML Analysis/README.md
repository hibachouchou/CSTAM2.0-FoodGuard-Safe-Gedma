# Part 3 : Trigger Scanned Food Images  Barcode QR Code → AI ML Analysis

## 1️⃣ Image Upload & S3 Event Trigger
- Configure your S3 bucket (e.g., `foodguard-images`) to trigger a Lambda function whenever a new object (image) is created.
- This ensures analysis starts **immediately after upload**.

---

## 2️⃣ Lambda Function: `ImageAnalyzer`
The Lambda function performs the following steps:

### Read Image
- Fetch the uploaded image from S3.

### Analyze Image Using AI

**Option 1: AWS Rekognition Custom Labels**
- Train a model to classify images as `FRAIS`, `ATTENTION`, or `SPOILED`.
- Returns a confidence score (0–100).

```json
{
  "label": "SPOILED",
  "confidence": 92
}

```
**Option 2: AWS SageMaker**
- Use a custom ML model for spoilage detection if you need more advanced analysis.

## Optional: Extract Text Using AWS Textract
- Extract expiry dates, product names, ingredients, or other label info.
- Only needed if your workflow requires reading product labels.

---

## 3️⃣ Store Analysis Results in DynamoDB
Lambda writes results to **DynamoDB**, either:

### Option A: Update the same `FoodImageMetadata` table
- Add new attributes:  
  - `aiState`: `"FRAIS" | "ATTENTION" | "SPOILED"`  
  - `confidence`: `0–100`  

**Example Updated Item:**
```json
{
  "productId": "milk-12345",
  "timestamp": 1731582200,
  "s3Url": "https://mybucket.s3.amazonaws.com/food-images/milk/milk-12345/1731582200.jpg",
  "type": "photo",
  "category": "milk",
  "deviceId": "foodguard-01",
  "aiState": "SPOILED",
  "confidence": 92
}
```

### Option B:  Store in a separate table

Keep raw metadata and AI results separate if desired.

## 4️⃣ AWS Services Overview
### AWS Rekognition Custom Labels

*Purpose:* Analyze the visual content of the image.

*Output:* Predicts freshness state (FRAIS, ATTENTION, SPOILED) with confidence score.

*Why necessary:* Detects spoilage automatically using visual cues (color, texture, mold, etc.).

### AWS Textract (Optional)

*Purpose:* Extract text from images (expiry dates, product names, ingredients).

*Output:* Strings you can store in DynamoDB (expiryText, ingredients, etc.).

*Why optional:* Only needed if your workflow requires reading product labels.

## 5️⃣ Workflow Summary

S3 Image Upload  
↓  
Lambda Function: `ImageAnalyzer`  
↓  
1️⃣ **Rekognition** → predicts `FRAIS` / `ATTENTION` / `SPOILED`  
2️⃣ **Textract (optional)** → extracts label text  
↓  
Update DynamoDB → store `aiState`, `confidence`, optional text  

**Required:** Rekognition  
**Optional:** Textract