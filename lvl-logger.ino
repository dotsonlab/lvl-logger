#include<SPIFlash.h>
#include "LowPower.h"

#define SERIAL_BAUD      115200
#define LED           9 // Moteinos have LEDs on D9
#define FLASH_SS      8 // and FLASH SS on D8

unsigned long previousMillis = 0;     // will store last time counter was updated
const long interval = 5000;           // interval at which to sleep (milliseconds)

unsigned long USpreviousMillis = 0;     // will store last time counter was updated
const long warmupInterval = 40000;           // interval at which to sleep (milliseconds)


int USpower = 7;
int USoutput = 5;
boolean USread = false;

uint8_t batteryVoltage = 0;


SPIFlash flash(FLASH_SS, 0xEF30);
//Moteino R4 equipped with 2048 pages of 256 byte storage locations (0 - 255). Offsets from 0 - 524288 available
boolean DataStored = false;
boolean cmdAllow = true;

const int numReadings = 10;
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int prevTotal = 0;              // prior total
uint8_t average = 0;            // the average

uint8_t DataInterval=1;        //0 - 255 minutes (whole numbers only)
uint32_t DataOffset=0;          //need to initialize this in setup by looking in flash memory
uint32_t PrevUSDataOffset=0;      //need to initialize this in setup by looking in flash memory
uint32_t PrevBattDataOffset=0;      //need to initialize this in setup by looking in flash memory

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
  digitalWrite(USpower, LOW);
  getOffset();
  if(DataOffset > 0) {
     PrevUSDataOffset = DataOffset - 1;
     PrevBattDataOffset = DataOffset - 1;
  }

}

void loop() {
  unsigned long currentMillis = millis();
  
  if(USread == false && DataStored == false) {
    USreading();
    battVoltage();
    if(USread == true && DataStored == false) {
      StoreData();
      delay(100);
    } 
  }
  
  if(USread == true && DataStored == true && currentMillis - previousMillis >= interval) {
    Serial.print("Sleeping for ");Serial.print(DataInterval);Serial.print(" minutes.");
    delay(10);

    unsigned long SleepCycles = (DataInterval * 60000 - warmupInterval) / 8000;
    for(int i = 0; i < SleepCycles; i++) {
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    }
    Serial.flush();
    Serial.println("Awake for reading sensor!");
    USread = false;
    cmdAllow = true;
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

  unsigned long UScurrentMillis = millis();
  if(cmdAllow == true) {
     Serial.println("Ultrasonic sensor stabilizing."); 
     Serial.print("COMMANDS ACCEPTED for about ");Serial.print(warmupInterval/1000);Serial.print(" seconds...");
     cmdAllow = !cmdAllow;
  }
  if(UScurrentMillis - USpreviousMillis >= warmupInterval) {
    Serial.println("COMMANDS NO LONGER ACCEPTED.");
    Serial.print("Averaging ");Serial.print(numReadings);Serial.println(" ultrasonic readings.");
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
    USpreviousMillis = UScurrentMillis;
  }
}

void battVoltage() {
  float pinValue = analogRead(A7);  //usually first reading is bad
  delay(10); //used for stabilization
  float batteryByte = (float)analogRead(A7)/1024*5*100/2;  //battery voltage converted to 5v multipled by 100 and divided by 2 to get into 0-255 range
  batteryVoltage = (byte)batteryByte;
}

void StoreData() {
    
    Serial.print("Preparing to write average of ");Serial.print(numReadings);Serial.println(" ultrasonic readings for flash memory.");
    flash.writeByte(DataOffset, average);
    PrevUSDataOffset = DataOffset;
    DataOffset++;
    flash.writeByte(DataOffset, batteryVoltage);
    PrevBattDataOffset = DataOffset;
    DataOffset++;
    Serial.print("Writing ultrasonic reading ");Serial.print(average);Serial.print(" to offset ");Serial.print(DataOffset-2);Serial.print(".  ");
    Serial.print("Last ultrasonic reading data was ");Serial.print(flash.readByte(PrevUSDataOffset));Serial.print(" stored at offset ");Serial.print(PrevUSDataOffset);Serial.println(".");
    Serial.print("Writing battery voltage reading ");Serial.print(batteryVoltage);Serial.print(" to offset ");Serial.print(DataOffset-1);Serial.print(".  ");
    Serial.print("Last ultrasonic reading data was ");Serial.print(flash.readByte(PrevBattDataOffset));Serial.print(" stored at offset ");Serial.print(PrevBattDataOffset);Serial.println(".");
      
    Serial.println("Data Written!");
    Serial.print("Preparing to Sleep...");
    DataStored = !DataStored;
//    PrevDataOffset = DataOffset;
//    DataOffset++;

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
      if(DataOffset > 0) {
        PrevUSDataOffset = DataOffset - 1;
        PrevBattDataOffset = DataOffset - 1;
      }
    }
    
    if (incomingCMD == String("ReadChip")) { //d=dump flash area
      Serial.println("Reading flash memory content:");
      getOffset();
      long timestamp = 0;
      long counter = 0;
      while(counter<DataOffset){
        Serial.print("Time Stamp (min):\t");Serial.print(timestamp);Serial.print("\t");
        Serial.print("Volume (gal):\t"); Serial.print(flash.readByte(counter++), DEC);;Serial.print("\t");
        float batteryReport = (float)flash.readByte(counter++) * 2 / 100;
        Serial.print("Battery Voltage (V):\t"); Serial.println(batteryReport, 2);
        timestamp = timestamp + DataInterval;
      }
      Serial.print("Data read from flash memory until offset ");
      Serial.print(DataOffset);
      Serial.println(".");
    }
      
  }
}
 
