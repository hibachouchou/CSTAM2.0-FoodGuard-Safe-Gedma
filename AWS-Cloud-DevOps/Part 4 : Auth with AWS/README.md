## Part 4 : AWS Services for User Management

## 1️⃣ Amazon Cognito
**Purpose:** Authentication, registration, and user management.  

**Features:**  
- Sign up / login / password reset  
- OAuth / social login (Google, Facebook, Apple)  
- JWT tokens for app authorization  

**Integration Flow:**  
- Mobile app uses Cognito SDK → authenticate → receive JWT token → call API Gateway + Lambda securely.

---

## 2️⃣ DynamoDB (User Profile Table)
**Purpose:** Store additional personal info beyond Cognito attributes.  

**Table Example:** `UserProfiles`  

- **Partition Key (PK):** `userId` (from Cognito)  
- **Attributes:**  
  - `name`  
  - `age`  
  - `country`  
  - `preferredLanguage`  
  - `lifestyle` (list: `["single", "student", …]`)  
  - `sex`  
  - `createdAt` / `updatedAt`  

**Example Item:**
```json
{
  "userId": "cognito-123456789",
  "name": "Hiba",
  "age": 23,
  "country": "Tunisia",
  "preferredLanguage": "fr",
  "lifestyle": ["student", "single"],
  "sex": "F",
  "createdAt": 1731582200,
  "updatedAt": 1731582200
}
```
# User Management Workflows

## 1️⃣ User Registration / Login
```text
[Mobile App]
    |
    |--- Register / Login (email/password or social)
    v
Amazon Cognito User Pool
    - Handles authentication and returns JWT tokens
    |
    v
Mobile App stores JWT
    - Use for authenticated requests to API Gateway
```

## 2️⃣ Adding / Updating Personal Info
```text
[Mobile App]
    |
    |--- Fill personal info form
    v
API Gateway: POST /updateProfile
    - Authenticated via JWT token from Cognito
    |
    v
Lambda: UpdateUserProfile
    - Writes user profile data to DynamoDB: UserProfiles
```