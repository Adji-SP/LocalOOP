# Firebase Cleanup Guide

## Problem
You're getting this error:
```
[ESP] STATUS:Upload error: Document already exists: projects/masgila
```

## Solution 1: Fixed ESP8266 Code (Automatic)

The ESP8266 code has been updated to use `patchDocument` instead of `createDocument`. This will:
- **Update** the document if it exists
- **Create** the document if it doesn't exist

**No action needed** - just re-upload the ESP8266 code:
```bash
pio run -e esp8266 -t upload
```

---

## Solution 2: Firebase Cleanup Utility (Manual Cleanup)

If you want to **delete existing documents** manually, use the cleanup utility.

### How to Use the Cleanup Utility

#### Step 1: Upload the Cleanup Program
```bash
pio run -e firebase_cleanup -t upload
```

#### Step 2: Open Serial Monitor
```bash
pio device monitor -e firebase_cleanup
```
Or use Arduino IDE Serial Monitor at **115200 baud**

#### Step 3: Use Commands

The utility will connect to WiFi and Firebase, then show:
```
====================================
Ready! Enter commands:
  DELETE_ALL       - Delete all documents
  DELETE:<path>    - Delete specific doc
  LIST             - List all documents
  HELP             - Show this help
====================================
```

#### Available Commands

**List all documents:**
```
LIST
```

**Delete specific document:**
```
DELETE:sensor_data/12345
```

**Delete ALL documents (use with caution!):**
```
DELETE_ALL
```

**Show help:**
```
HELP
```

### Example Session

```
> LIST
Listing documents in 'sensor_data' collection...
Found documents:
- sensor_data/1234567
- sensor_data/2345678

> DELETE:sensor_data/1234567
Deleting document: sensor_data/1234567
✓ Document deleted successfully!

> LIST
Listing documents in 'sensor_data' collection...
Found documents:
- sensor_data/2345678

> DELETE_ALL
WARNING: This action cannot be undone!
Proceeding in 3 seconds...
✓ Deleted: sensor_data/2345678
Cleanup complete!
```

---

## After Cleanup

Once you're done cleaning up, re-upload the normal ESP8266 program:

```bash
pio run -e esp8266 -t upload
```

---

## Firestore Document Structure

Your data is stored in Firestore like this:

```
Collection: sensor_data
├── Document: {timestamp1}
│   ├── temp: 25.5
│   ├── weight: 100.2
│   ├── relay1: 1
│   ├── relay2: 0
│   ├── status: 1
│   ├── device: "MEGA_DNA_LOGGER"
│   └── timestamp: 1234567
├── Document: {timestamp2}
│   └── ...
```

Each document is named with its timestamp (e.g., `12345678`).

---

## Troubleshooting

### Cleanup utility won't connect to WiFi
- Check WiFi credentials in `SystemConfig.h`
- Make sure ESP8266 is within WiFi range

### Cleanup utility can't authenticate to Firebase
- Verify `API_KEY` in `SystemConfig.h`
- Verify `FIREBASE_PROJECT_ID` in `SystemConfig.h`
- Check Firebase console for anonymous auth settings

### Documents not showing in LIST
- Documents might be in a different collection
- Check Firebase Console → Firestore Database

### Can't delete documents
- You need write permissions in Firebase Security Rules
- Default rules should allow read/write

---

## Firebase Security Rules

To allow the cleanup utility to delete documents, ensure your Firestore rules allow it:

```javascript
rules_version = '2';
service cloud.firestore {
  match /databases/{database}/documents {
    match /sensor_data/{document=**} {
      allow read, write: if true;  // Allow all (for testing)
      // For production, use proper authentication
    }
  }
}
```

**⚠️ WARNING:** The rule above allows anyone to read/write. Use proper authentication in production!

---

## Quick Reference

| Task | Command |
|------|---------|
| Upload cleanup utility | `pio run -e firebase_cleanup -t upload` |
| Upload normal ESP8266 code | `pio run -e esp8266 -t upload` |
| Upload Mega2560 code | `pio run -e megaatmega2560 -t upload` |
| Open serial monitor | `pio device monitor` |

---

## Need Help?

If you're still having issues:

1. Check the Serial Monitor output for error messages
2. Verify all credentials in `SystemConfig.h`
3. Check Firebase Console for existing documents
4. Try deleting documents manually in Firebase Console
