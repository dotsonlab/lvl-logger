#include<SPIFlash.h>
#include "LowPower.h"

#define SERIAL_BAUD      115200
#define LED           9 // Moteinos have LEDs on D9
#define FLASH_SS      8 // and FLASH SS on D8

unsigned long previousMillis = 0;     // will store last time counter was updated
const long interval = 5000;           // interval at which to sleep (milliseconds)


int USpower = 7;
int USoutput = 5;
boolean USread = false;


SPIFlash flash(FLASH_SS, 0xEF30);
//Moteino R4 equipped with 2048 pages of 256 byte storage locations (0 - 255). Offsets from 0 - 524288 available
boolean DataStored = false;

const int numReadings = 10;
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int prevTotal = 0;              // prior total
uint8_t average = 0;            // the average

uint8_t DataInterval=15;        //0 - 255 minutes (whole numbers only)
uint32_t DataOffset=0;          //need to initialize this in setup by looking in flash memory
uint32_t PrevDataOffset=0;      //need to initialize this in setup by looking in flash memory

String incomingCMD;

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.flush();
  Serial.print("Start...");

  if (flash.initialize())
  {
    Serial.println("Init OK!");
  }
  else
    Serial.println("Init FAIL!");

  pinMode(USpower,OUTPUT);
  getOffset();
  if(DataOffset > 0) {
     PrevDataOffset = DataOffset - 1;
  }

}

void loop() {
  unsigned long currentMillis = millis();
  
  if(USread == false && DataStored == false) {
    USreading();
    delay(500);
    if(USread == true && DataStored == false) {
      StoreData();
      delay(500);
    } 
  }
  
  if(USread == true && DataStored == true && currentMillis - previousMillis >= interval) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    USread = false;
    DataStored = false;
    previousMillis = currentMillis;
  }

}

void getOffset() {
    byte unwritten = 255; 
    long i = 0;
    while(flash.readByte(i) != unwritten) {
      i++;
      }
    DataOffset = i;
    i = 0;
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
    for (int i = 0; i < numReadings; i++) {
      total = prevTotal + readings[i];
      prevTotal = total;
    }
    // calculate the average:;
    average = total / numReadings / 4;  //divided by 4 to make value a 1 byte number (0-255)
    //Serial.println(average);
    
    // ...wrap around to the beginning and restart average:
    readIndex = 0;
    total=0;
    prevTotal=0;
  }
  digitalWrite(USpower, LOW);
  USread = !USread;
}

void StoreData() {


    
    flash.writeByte(DataOffset, average);
    
    Serial.print("Writing ");
    Serial.print(average);
    Serial.print(" to offset ");
    Serial.print(DataOffset);
    Serial.println(".");

    Serial.print("Last stored data was ");
    Serial.print(flash.readByte(PrevDataOffset));
    Serial.print(" stored at offset ");
    Serial.print(PrevDataOffset);    
    Serial.println(".");
    DataStored = !DataStored;
    PrevDataOffset = DataOffset;
    DataOffset++;

}

void serialEvent() {   //This interrupt will trigger when the data coming from the serial monitor(pc/mac/other) is received
  if (Serial.available() > 0) {
    incomingCMD = Serial.readStringUntil('\n');

    if (incomingCMD == String("FormatChip")) {    //erases chip
      Serial.print("Formatting flash chip ... ");
      flash.chipErase();
      while(flash.busy());
      Serial.println("Flash formatted for datalogging.");
      getOffset();
      }

    if (incomingCMD == String("WriteData")) { //0-9
      average = random(0,254);  //for testing purposes
      getOffset();
      Serial.print(average); Serial.print(" written to offset ");Serial.println(DataOffset);
      flash.writeByte(DataOffset,average);
      PrevDataOffset=DataOffset;
      DataOffset++;
    }

    if (incomingCMD == String("ReadChip")) { //d=dump flash area
      Serial.println("Reading flash memory content:");
      getOffset();
      long timestamp = 0;
      long counter = 0;
      while(counter<DataOffset){
        Serial.print("Time Stamp (min):\t");
        Serial.print(timestamp);Serial.print("\t");
        Serial.print("Volume (gal):\t"); Serial.println(flash.readByte(counter++), DEC);
        timestamp = timestamp + DataInterval;
      }
      Serial.print("Data read from flash memory until offset ");
      Serial.print(DataOffset);
      Serial.println(".");
    }
      
  }
}
 
