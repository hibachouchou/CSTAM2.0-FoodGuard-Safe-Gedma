# Part 2: Image Scan from Mobile App Camera 📸

This section outlines the AWS Architecture required to integrate a **React Native Mobile App** for scanning and managing food product images and metadata.

---

## ☁️ AWS Architecture with Mobile App Integration

### Goal

Your React Native application will be able to:

* Scan a **QR code**, **barcode**, or take an **image of a food product**.
* Query AWS services to retrieve **freshness/spoilage information** related to the product.

### DynamoDB: `FoodImageMetadata` Table

This table stores the searchable metadata for every image upload, allowing for easy querying without handling large image files.

| Key Type | Attribute | Type | Description |
| :--- | :--- | :--- | :--- |
| **Partition Key** | `productId` | String | Unique identifier for the food product. |
| **Sort Key** | `timestamp` | Number | UNIX time (ms or seconds) of the scan/upload. |
| Attribute | `s3Url` | String | URL of the image object stored in S3. |
| Attribute | `type` | String | Type of scan: `"photo"`, `"qr"`, or `"barcode"`. |
| Attribute | `deviceId` | String | Optional link to the relevant ESP32 sensor device ID. |
| Attribute | `category` | String | Product type: `"milk"`, `"chicken"`, `"meat"`, `"fruits"`, etc. |
| Attribute | `notes` | String | Optional additional information or description. |

#### Example JSON Item

```json
{
  "productId": "milk-12345",
  "timestamp": 1731582200,
  "s3Url": "[https://mybucket.s3.amazonaws.com/food-images/milk-12345/1731582200.jpg](https://mybucket.s3.amazonaws.com/food-images/milk-12345/1731582200.jpg)",
  "type": "photo",
  "deviceId": "foodguard-01",
  "category": "milk",
  "notes": "Front label scanned"
}
```

## 🔄 Workflow with Category

1. **Mobile App Scan**
   - The mobile app scans an image or barcode and selects or detects the product category.

2. **Upload to S3**
   - The app uploads the image file directly to an S3 bucket.

3. **Store Metadata**
   - The app stores the corresponding metadata record in the `FoodImageMetadata` table.

✅ This architecture ensures:
- Sensor data is kept separate from image data.
- Large binary files are not stored inside DynamoDB, which is optimized for small, fast key-value data.

---

### S3 Object Path Structure

Using a structured path makes querying and managing files simpler.

- **Bucket Name:** `foodguard-images`
- **Folder Structure:** `food-images/{category}/{productId}/`
- **Filename:** `{timestamp}.jpg`

### Full Object Path Examples

| Description       | Example Path                                                        |
|------------------|--------------------------------------------------------------------|
| Milk Products     | `s3://foodguard-images/food-images/milk/milk-12345/1731582200.jpg` |
| Chicken Products  | `s3://foodguard-images/food-images/chicken/chicken-98765/1731583200.jpg` |

---
