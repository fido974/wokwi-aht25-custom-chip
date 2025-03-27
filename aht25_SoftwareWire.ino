// Test for SoftwareWire library.
// Tested with an Arduino Uno connected to an Arduino Uno.
// This is the sketch for the Master Arduino using the software i2c.
//
// Fred HOARAU - march 2025

// Use define to switch between the Arduino Wire and the SoftwareWire.
#define TEST_SOFTWAREWIRE


#ifdef TEST_SOFTWAREWIRE

#include "SoftwareWire.h"

// SoftwareWire constructor.
// Parameters:
//   (1) pin for the software sda
//   (2) pin for the software scl
//   (3) use internal pullup resistors. Default true. Set to false to disable them.
//   (4) allow the Slave to stretch the clock pulse. Default true. Set to false for faster code.

// This stress test uses A4 and A5, that makes it easy to switch between the (hardware) Wire
// and the (software) SoftwareWire libraries.
// myWire: sda = A4, scl = A5, turn on internal pullups, allow clock stretching by Slave
SoftwareWire myWire( A0, A1);
SoftwareWire myWire2( A4, A5);


#else

// Make code work with normal Wire library.
#include <Arduino.h>
#include <Wire.h>
#define myWire Wire         // use the real Arduino Wire library instead of the SoftwareWire.
#define myWire2 Wire         


#endif

uint32_t tempdata;
float temp1;
uint32_t humdata;
float hum1;

uint32_t tempdata2;
float temp2;
uint32_t humdata2;
float hum2;

int alim1Pin = 2;  // Detection alim 1 = digital pin D2
int alim2Pin = 3;  // Detection alim 1 = digital pin D3 
int led1Pin = 4; // Led indication mesure AHT25-1
int led2Pin = 5; // Led indication mesure AHT25-2
int ledAlim1Pin = 6; // Led indication mesure alim-1
int ledAlim2Pin = 7; // Led indication mesure alim-2

int val1 = 0;
int val2 = 0;


void setup()
{
  Serial.begin(9600);      // start serial port
  Serial.println(F("\nPITON-VOLCAN"));
  Serial.println(F("\nMesure Humidite et Temperature\n"));

  myWire.begin();          // join i2c bus as master
  myWire2.begin();   
 
  pinMode(alim1Pin, INPUT);    // sets the digital pin 2 as input
  pinMode(alim2Pin, INPUT);    // sets the digital pin 3 as input

  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);

  pinMode(ledAlim1Pin, OUTPUT);
  pinMode(ledAlim2Pin, OUTPUT);

}


void loop()
{
  String jsonOutput;
  String errorMessage = "";
  float temp1 = NAN, hum1 = NAN, temp2 = NAN, hum2 = NAN;
  int val1 = 0, val2 = 0;
  bool aht1_ok = true;
  bool aht2_ok = true;

  //////////
  // CAPTEUR SUR PORTS A0 (SDA) et A1 (SCL)
  /////////
  ///////////////////////////////////////////// 1 //////////////////////////////////
  byte initie[3] = { 0xBE, 0x08, 0x00 };
  myWire.beginTransmission(0x38);
  myWire.write(initie, 3);
  if (myWire.endTransmission() != 0) {
    errorMessage += "Erreur Init AHT25-1,";
    aht1_ok = false;
  }
  delay(20);

  byte mesure[3] = { 0x00, 0xAC, 0x33 };
  myWire.beginTransmission(0x38);
  myWire.write(mesure, 3);
  if (myWire.endTransmission() != 0) {
    errorMessage += "Erreur Mesure AHT25-1,";
    aht1_ok = false;
  }
  delay(80);

  int n = myWire.requestFrom(0x38, 7);

  byte buffer[7];
  myWire.readBytes(buffer, n);

  if (n == 7) {
    uint32_t rawHum = ((uint32_t)buffer[1] << 12) | ((uint32_t)buffer[2] << 4) | ((buffer[3] & 0xF0) >> 4);
    hum1 = ((float)rawHum * 100) / 1048576;

    uint32_t rawTemp = ((uint32_t)(buffer[3] & 0x0F) << 16) | ((uint32_t)buffer[4] << 8) | buffer[5];
    temp1 = ((float)rawTemp * 200 / 1048576) - 50;
  } else {
    errorMessage += "Erreur lecture AHT25-1,";
    aht1_ok = false;
  }

  byte raz[1] = { 0xBA };
  myWire.beginTransmission(0x38);
  myWire.write(raz, 1);
  if (myWire.endTransmission() != 0) {
    errorMessage += "Erreur Reset AHT25-1,";
    aht1_ok = false;
  }

  if (aht1_ok) {
    digitalWrite(led1Pin, HIGH);
  } else {
    digitalWrite(led1Pin, LOW);
  }

  //////////
  // CAPTEUR SUR PORTS A4 (SDA) et A5 (SCL)
  /////////
  ///////////////////////////////////////////// 2 //////////////////////////////////
  byte initie2[3] = { 0xBE, 0x08, 0x00 };
  myWire2.beginTransmission(0x38);
  myWire2.write(initie2, 3);
  if (myWire2.endTransmission() != 0) {
    errorMessage += "Erreur Init AHT25-2,";
    aht2_ok = false;
  }
  delay(20);

  byte mesure2[3] = { 0x00, 0xAC, 0x33 };
  myWire2.beginTransmission(0x38);
  myWire2.write(mesure2, 3);
  if (myWire2.endTransmission() != 0) {
    errorMessage += "Erreur Mesure AHT25-2,";
    aht2_ok = false;
  }
  delay(80);

  int n2 = myWire2.requestFrom(0x38, 7);

  byte buffer2[7];
  myWire2.readBytes(buffer2, n2);

  if (n2 == 7) {
    uint32_t rawHum2 = ((uint32_t)buffer2[1] << 12) | ((uint32_t)buffer2[2] << 4) | ((buffer2[3] & 0xF0) >> 4);
    hum2 = ((float)rawHum2 * 100) / 1048576;

    uint32_t rawTemp2 = ((uint32_t)(buffer2[3] & 0x0F) << 16) | ((uint32_t)buffer2[4] << 8) | buffer2[5];
    temp2 = ((float)rawTemp2 * 200 / 1048576) - 50;
  } else {
    errorMessage += "Erreur lecture AHT25-2,";
    aht2_ok = false;
  }

  byte raz2[1] = { 0xBA };
  myWire2.beginTransmission(0x38);
  myWire2.write(raz2, 1);
  if (myWire2.endTransmission() != 0) {
    errorMessage += "Erreur Reset AHT25-2,";
    aht2_ok = false;
  }

  if (aht2_ok) {
    digitalWrite(led2Pin, HIGH);
  } else {
    digitalWrite(led2Pin, LOW);
  }

  // Mesure des capteurs de tension USB
  val1 = digitalRead(alim1Pin);   // Lecture de Alim 1
  val2 = digitalRead(alim2Pin);   // Lecture de Alim 2
  // Mise a jour de l'etat des LED Alim
  if (val1 == 1) {
    digitalWrite(ledAlim1Pin, HIGH);
  } else {
    digitalWrite(ledAlim1Pin, LOW);
  }
  if (val2 == 1) {
    digitalWrite(ledAlim2Pin, HIGH);
  } else {
    digitalWrite(ledAlim2Pin, LOW);
  }

  // Create JSON object
  jsonOutput += "{";
  jsonOutput += "\"errors\":\"" + errorMessage + "\",";
  jsonOutput += "\"temp1\":" + String(temp1) + ",";
  jsonOutput += "\"hum1\":" + String(hum1) + ",";
  jsonOutput += "\"temp2\":" + String(temp2) + ",";
  jsonOutput += "\"hum2\":" + String(hum2) + ",";
  jsonOutput += "\"val1\":" + String(1 - val1) + ","; // Inversion car resist pullup
  jsonOutput += "\"val2\":" + String(1 - val2);
  jsonOutput += "}";

  Serial.println(jsonOutput);

  delay(2000);
  digitalWrite(led1Pin, LOW);
  digitalWrite(led2Pin, LOW);
  delay(2000);

}
