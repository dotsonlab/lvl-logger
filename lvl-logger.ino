#include "LowPower.h"
#include<SPIFlash.h>

#define SERIAL_BAUD      115200
#define LED           9 // Moteinos have LEDs on D9
#define FLASH_SS      8 // and FLASH SS on D8

uint32_t prevOffset=0;
uint32_t Offset=0;

SPIFlash flash(FLASH_SS, 0xEF30);

int USpower = 7;
int USoutput = 5;

const int numReadings = 25;

int i;
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int prevTotal = 0;              // prior total
uint8_t average = 0;                // the average

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.print("Start...");

  if (flash.initialize())
  {
    Serial.println("Init OK!");
    Blink(LED, 20, 10);
  }
  else
    Serial.println("Init FAIL!");
  
  delay(1000);

  pinMode(USpower,OUTPUT);
  flash.chipErase();

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
    average = total / numReadings / 4;
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
    
    flash.writeByte(Offset, average);
    
    Serial.print("Writing ");
    Serial.print(average);
    Serial.print(" to offset ");
    Serial.print(Offset);
    Serial.println(".");

    Serial.print("Last stored data was ");
    Serial.print(flash.readByte(Offset));
    Serial.print(" stored at offset ");
    Serial.print(Offset);    
    Serial.println(".");

}

void Blink(byte PIN, int DELAY_MS, byte loops) {
  pinMode(PIN, OUTPUT);
  while (loops--)
  {
    digitalWrite(PIN,HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN,LOW);
    delay(DELAY_MS);  
  }
}
