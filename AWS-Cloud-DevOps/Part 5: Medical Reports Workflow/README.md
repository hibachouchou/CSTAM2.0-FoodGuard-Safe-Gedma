#  Medical Reports & Health Notes Workflow

---

## 1️⃣ Uploading Medical Reports (PDF/Image/DOCX)

**AWS Services:**

- S3 → Stockage des fichiers
- API Gateway + Lambda → Réception des uploads depuis l’app mobile
- DynamoDB → Stockage des métadonnées des fichiers
- Cognito → Authentification via JWT

**Workflow:**

[Mobile App]  
 |  
 |--- Upload medical report (PDF/Image/DOCX)  
 v  
API Gateway: POST /uploadMedicalReport  
 - Authenticated via Cognito JWT  
 |  
 v  
Lambda: UploadMedicalReportHandler  
 - Store file in S3 bucket: "foodguard-medical-reports"  
  - Object path: medical-reports/{userId}/{timestamp}-{filename}  
 - Store metadata in DynamoDB: MedicalReports  
  - PK: userId  
  - SK: timestamp  
  - Attributes: s3Url, fileType, description, notes  

**Example DynamoDB item:**

userId: cognito-123456789  
timestamp: 1731582200  
s3Url: https://s3.amazonaws.com/foodguard-medical-reports/user123/1731582200-bloodtest.pdf  
fileType: pdf  
description: Blood test report  
notes: Vitamin D deficiency  

---

## 2️⃣ Writing Health Notes (Text)

**AWS Services:**

- DynamoDB → Stockage des notes  
- Optional S3 → Si les notes sont volumineuses  

**Workflow:**

[Mobile App]  
 |  
 |--- Write health note  
 v  
API Gateway: POST /addHealthNote  
 - Authenticated via Cognito JWT  
 |  
 v  
Lambda: AddHealthNoteHandler  
 - Store note in DynamoDB: UserHealthNotes  
  - PK: userId  
  - SK: timestamp  
  - Attributes: noteText, relatedReportId (optional)  

**Example DynamoDB item:**

userId: cognito-123456789  
timestamp: 1731582300  
noteText: Feeling nauseous after consuming dairy products  
relatedReportId: 1731582200  

---

## 3️⃣ Optional ML / Medical Analysis

**AWS Services:**

- Textract → Extraction du texte des PDFs/images  
- Comprehend Medical → Extraction des entités médicales (conditions, médicaments, labs, dosages, unités)  
- Lambda → Orchestration et stockage des résultats dans DynamoDB  

**Workflow:**

S3 Upload → Lambda: ProcessMedicalReport  
 |  
 v  
Textract → Extract text from document  
 |  
 v  
Comprehend Medical → Analyze text  
 - Identify: conditions, medications, labs, dosages  
 |  
 v  
Lambda → Update DynamoDB: MedicalAnalysis  
 - Attributes: aiCategory, aiRiskScore, extractedMedicalText, medicalEntities  

**Example DynamoDB item (MedicalAnalysis):**

userId: cognito-123456789  
reportId: 1731582200  
conditions: Vitamin D deficiency, Hypertension  
medications: Vitamin D supplement  
labs: Cholesterol 220 mg/dL, Vitamin D 15 ng/mL  
recommendations: Monitor vitamin D intake and follow-up blood test in 3 months  

---

## 4️⃣ Summary Architecture Diagram (Logical Flow)

[Mobile App]  
 |  
 |--- Upload Medical Report  
 v  
API Gateway → Lambda: UploadMedicalReportHandler  
 |  |  
 |  → S3: "foodguard-medical-reports"  
 |  → DynamoDB: MedicalReports  
 |  
 |--- Write Health Note  
 v  
API Gateway → Lambda: AddHealthNoteHandler  
 → DynamoDB: UserHealthNotes  

Optional ML Flow:  
S3 Upload → Lambda: ProcessMedicalReport → Textract → Comprehend Medical → DynamoDB: MedicalAnalysis
