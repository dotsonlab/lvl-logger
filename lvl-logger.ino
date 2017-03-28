#include "LowPower.h"
#include<SPIFlash.h>

int16_t DataPage = 0;                // initial SPI flash storage page
uint32_t prevOffset=0;
int PageCorrectedprevOffset;
uint32_t PageCorrectedprevOffsetB;
uint8_t Offset;
int PageCorrectedOffset;
uint32_t PageCorrectedOffsetB;

SPIFlash flash(8);

int USpower = 7;
int USoutput = 5;

const int numReadings = 25;

int i;
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int prevTotal = 0;              // prior total
short average = 0;                // the average

void setup() {
  Serial.begin(115200);
  pinMode(USpower,OUTPUT);
  flash.begin();
  flash.eraseChip();
  Offset = flash.getAddress(sizeof(short));
}

void loop() {
   Serial.flush();
   // Enter power down state for 8 s with ADC and BOD module disabled
   // LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
   USreading();
   StoreData();
}

void USreading() {
  //turn ultrasonic power ON
  digitalWrite(USpower, HIGH);
  delay(500);
    while (readIndex < numReadings) {
      // read from the sensor:
      readings[readIndex] = analogRead(USoutput);
      // advance to the next position in the array:
      readIndex = readIndex + 1;     
    }

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // calculate the average:
    for (i = 0; i < numReadings; i++) {
      total = prevTotal + readings[i];
      prevTotal = total;
    }
    // calculate the average:;
    average = total / numReadings;
    //Serial.println(average);
    
    // ...wrap around to the beginning and restart average:
    readIndex = 0;
    total=0;
    prevTotal=0;
  }
  delay(5);        // delay in between reads for stability
  digitalWrite(USpower, LOW);
}

void StoreData() {

    prevOffset = Offset;
    Offset++;
    Offset++;
    if(Offset > 252) {
      Offset = 0;
      DataPage = 1;
    }
    //Offset = flash.getAddress(sizeof(short));
    
    flash.writeShort(DataPage, Offset, average);
    
    Serial.print("Writing ");
    Serial.print(average);
    Serial.print(" to page ");
    Serial.print(DataPage);
    Serial.print(" and offset ");
    Serial.print(Offset);
    Serial.println(".");

    Serial.print("Last stored data was ");
    Serial.print(flash.readShort(DataPage,Offset));
    Serial.print(" stored at offset ");
    Serial.println(Offset);

    }
