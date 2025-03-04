const int trigPin = 13;
const int echoPin = 12;

//define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

#include <WiFi.h>
#include <HTTPClient.h>

#include <SPI.h>
#include <MFRC522.h>

#include <ESP32Servo.h>

#define SS_PIN 5
#define RST_PIN 0

#define servoPin 14

#define LOOP_DELAY 1000

long reqDelay;
long openTime;

Servo servo1;

// conectivitate
const char* ssid = "UPB-Guest";       // Nombre de tu red Wi-Fi
const char* password = NULL; // Contraseña de tu red Wi-Fi
const char* serverAddress = "10.41.189.14:5200"; // Cambia la dirección IP con la de tu PC

HTTPClient http;

// distanta
long duration;
float distanceCm;
float distanceInch;

// rfid
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;

// Define a structure to hold card UID and name
struct Card {
  byte uid[4];
  const char* name;
};

// List of valid cards
Card validCards[] = {
  {{0xDA, 0x27, 0x34, 0x16}, "Mihai"},
  {{0xA1, 0xB2, 0xC3, 0xD4}, "Jane Smith"}, // Add more cards as needed
  // Add more valid cards here
};
// Number of valid cards
const int numValidCards = sizeof(validCards) / sizeof(validCards[0]);

// Init array that will store new NUID 
byte nuidPICC[4];

void setup() {
  Serial.begin(115200); // Starts the serial communication
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  reqDelay = 0;

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..."); // poate stergem 
  }

  Serial.println("Connected to WiFi");
  http.begin(serverAddress);

  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scans the MIFARE Classic NUID."));
  Serial.print(F("Using the following key: "));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

  servo1.attach(servoPin);
  openTime = 0;
}

void loop() {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  if (openTime > 0) {
    openTime -= LOOP_DELAY;
  } else {
    openTime = 0;
    servo1.write(0);
  }
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;
  
  // Convert to inches
  distanceInch = distanceCm * CM_TO_INCH;

  
  
  // Prints the distance in the Serial Monitor
  if (distanceCm <= 150) {
    Serial.print("Distance (cm): ");
    Serial.println(distanceCm);
    Serial.print("Distance (inch): ");
    Serial.println(distanceInch);
  }

  if (distanceCm <= 150)
    if (reqDelay == 0) {
      sendDistance(distanceCm);
      reqDelay = 10000;
    } else {
      reqDelay -= LOOP_DELAY;
    }

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Verify if the NUID has been read
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check if the PICC is of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && 
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  // Check if the card UID matches any valid card
  int validIndex = isCardValid(rfid.uid.uidByte, rfid.uid.size);
  if (validIndex != -1) {
    Serial.print(F("Access granted for: "));
    Serial.println(validCards[validIndex].name);
    servo1.write(180);
    openTime = 50 * LOOP_DELAY;
  } else {
    Serial.println(F("Access denied."));
  }

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();

  delay(LOOP_DELAY);
}

// Helper function to check if the card UID matches any valid card
int isCardValid(byte *cardUID, byte length) {
  for (int i = 0; i < numValidCards; i++) {
    if (length == sizeof(validCards[i].uid)) {
      boolean match = true;
      for (byte j = 0; j < length; j++) {
        if (cardUID[j] != validCards[i].uid[j]) {
          match = false;
          break;
        }
      }
      if (match) {
        return i; // Return the index of the valid card
      }
    }
  }
  return -1; // No match found
}

/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
  Serial.println();
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
  Serial.println();
}



void sendDistance(long distance) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverPath = String("http://10.41.189.14:5200/distance?value=") + String(distance);

    Serial.print("Sending distance to: ");
    Serial.println(serverPath);

    http.begin(serverPath.c_str());
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending GET: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}