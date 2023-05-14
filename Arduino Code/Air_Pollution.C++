#include <SoftwareSerial.h>
#include <FirebaseArduino.h>
#include <EEPROM.h>

#define PM_SERIAL_RX_PIN 2
#define PM_SERIAL_TX_PIN 3
#define PM_DATA_LENGTH 9 // data length for PM2.5 and PM10

SoftwareSerial pmSerial(PM_SERIAL_RX_PIN, PM_SERIAL_TX_PIN);

unsigned int pm25 = 0; // stores PM2.5 value
unsigned int pm10 = 0; // stores PM10 value

// Set up the Firebase Realtime Database credentials
#define FIREBASE_HOST "firebase-hostname"
#define FIREBASE_AUTH "firebase-secret"

void setup() {
  Serial.begin(9600);
  pmSerial.begin(9600);

  // Connect to the Firebase Realtime Database
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  // read previous data from EEPROM
  pm25 = EEPROM.read(0);
  pm10 = EEPROM.read(2);
}

void loop() {
  // request data from the sensor
  pmSerial.write(0x42);
  pmSerial.write(0x4d);
  pmSerial.write(0xe1);
  pmSerial.write(0x00);
  pmSerial.write(0x00);
  pmSerial.write(0x01);
  pmSerial.write(0x70);

  // wait for data
  delay(1000);

  if (pmSerial.available() > 0) {
    // read data from the sensor
    byte buffer[PM_DATA_LENGTH];
    pmSerial.readBytes(buffer, PM_DATA_LENGTH);

    // verify checksum
    if (verifyChecksum(buffer, PM_DATA_LENGTH)) {
      // extract PM2.5 and PM10 values
      pm25 = (buffer[2] << 8) | buffer[3];
      pm10 = (buffer[4] << 8) | buffer[5];

      // store data in EEPROM
      EEPROM.write(0, pm25);
      EEPROM.write(2, pm10);

      // print data to serial monitor
      Serial.print("PM2.5: ");
      Serial.print(pm25);
      Serial.print(" ug/m3, PM10: ");
      Serial.print(pm10);
      Serial.println(" ug/m3");

      // update Firebase Realtime Database with the new values
      Firebase.setFloat("pm2.5", pm25);
      Firebase.setFloat("pm10", pm10);
    }
  }
}

bool verifyChecksum(byte* buffer, int length) {
  unsigned int sum = 0;
  for (int i = 0; i < length - 2; i++) {
    sum += buffer[i];
  }
  return (sum & 0xff) == buffer[length - 2] && (sum >> 8) == buffer[length - 1];
}