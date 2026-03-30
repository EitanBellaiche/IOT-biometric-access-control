#define BLYNK_TEMPLATE_ID "TMP_PLACEHOLDER"
#define BLYNK_TEMPLATE_NAME "Fingerprint Lock"
#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_Fingerprint.h>

#define FINGERPRINT_RX 17
#define FINGERPRINT_TX 16
#define RELAY_PIN 18



Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);

unsigned long unlockUntil = 0;
bool lockIsOpen = false;

const char* getUserName(int id) {
  switch (id) {
    case 1: return "Eitan";
    default: return "Unknown";
  }
}

void openLock() {
  digitalWrite(RELAY_PIN, HIGH);
  lockIsOpen = true;
  unlockUntil = millis() + 3000;
  Serial.println("Lock OPEN");
}

void closeLock() {
  digitalWrite(RELAY_PIN, LOW);
  lockIsOpen = false;
  Serial.println("Lock CLOSED");
}

int getFingerprintID() {
  uint8_t p = finger.getImage();

  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      return -2;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return -2;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return -2;
    default:
      Serial.println("Unknown error");
      return -2;
  }

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return -2;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return -2;
    case FINGERPRINT_FEATUREFAIL:
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return -2;
    default:
      Serial.println("Unknown error");
      return -2;
  }

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
    return finger.fingerID;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return -1;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return -2;
  } else {
    Serial.println("Unknown error");
    return -2;
  }
}

BLYNK_WRITE(V0) {
  int value = param.asInt();

  if (value == 1) {
    Serial.println("Opening from phone...");
    openLock();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Smart Lock Startup");

  pinMode(RELAY_PIN, OUTPUT);
  closeLock();

  Serial2.begin(57600, SERIAL_8N1, FINGERPRINT_RX, FINGERPRINT_TX);
  finger.begin(57600);
  delay(100);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (true) {
      delay(1000);
    }
  }

  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());

  Blynk.begin(auth, ssid, pass);

  Serial.println("System ready");
}

void loop() {
  Blynk.run();

  int id = getFingerprintID();

  if (id >= 0) {
    Serial.print("Access granted for ");
    Serial.println(getUserName(id));
    openLock();
    delay(1200);
  } else if (id == -1) {
    Serial.println("Access denied - no match");
    delay(800);
  }

  if (lockIsOpen && millis() > unlockUntil) {
    closeLock();
  }

  delay(50);
}
