#include "LowPower.h"
#include <RFM69.h>
#include <SPI.h>
#include<SPIFlash.h>

#define NODEID      99
#define NETWORKID   100
#define GATEWAYID   1
#define FREQUENCY   RF69_433MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define KEY         "sampleEncryptKey" //has to be same 16 characters/bytes on all nodes, not more not less!
#define LED           9 // Moteinos have LEDs on D9
#define SERIAL_BAUD      115200
#define ACK_TIME    30  // # of ms to wait for an ack

#define FLASH_SS      8 // and FLASH SS on D8

//int TRANSMITPERIOD = 1000; //transmit a packet to gateway so often (in ms)

byte sendSize=0;
boolean requestACK = false;
SPIFlash flash(FLASH_SS, 0xEF30);
RFM69 radio;

uint32_t prevOffset=0;
uint32_t Offset=0;


int USpower = 7;
int USoutput = 5;

const int numReadings = 25;

int i;
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int prevTotal = 0;              // prior total
uint8_t average = 0;                // the average

typedef struct {
  char           cmd; //store this nodeId
  //unsigned long uptime; //uptime in ms
  byte         volume;   //volume
} Payload;
Payload theData;

String incomingCMD;

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.flush();
  Serial.print("Starting...");
  
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  //radio.setHighPower(); //uncomment only for RFM69HW!
  radio.encrypt(KEY);
  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  
  if (flash.initialize())
    Serial.println("SPI Flash Init OK!");
  else
    Serial.println("SPI Flash Init FAIL! (is chip present?)");

  pinMode(USpower,OUTPUT);
  flash.chipErase();

}

long lastPeriod = -1;

void loop() {
   
  //check for any received packets
  if (radio.receiveDone())
  {
    Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");
    for (byte i = 0; i < radio.DATALEN; i++)
      Serial.print((char)radio.DATA[i]);
    Serial.print("   [RX_RSSI:");Serial.print(radio.readRSSI());Serial.print("]");

    if (radio.ACKRequested())
    {
      radio.sendACK();
      Serial.print(" - ACK sent");
      delay(10);
    }
    Blink(LED,5);
    Serial.println();
  }
  
//  int currPeriod = millis()/TRANSMITPERIOD;
//  if (currPeriod != lastPeriod)
//  {
//    //fill in the struct with new values
//    theData.cmd = NODEID;
//    //theData.uptime = millis();
//    theData.volume = average;
//    
//    Serial.print("Sending struct (");
//    Serial.print(sizeof(theData));
//    Serial.print(" bytes) ... ");
//    if (radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData)))
//      Serial.print(" ok!");
//    else Serial.print(" nothing...");
//    Serial.println();
//    Blink(LED,3);
//    lastPeriod=currPeriod;
//  }
   
   USreading();
   //StoreData();
   // Enter power down state for 8 s with ADC and BOD module disabled
   // LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
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

void serialEvent() {   //This interrupt will trigger when the data coming from the serial monitor(pc/mac/other) is received
  if (Serial.available() > 0) {
    incomingCMD = Serial.readStringUntil('\n');

    if (incomingCMD == String("dumpReg")) //dump all register values
      radio.readAllRegs();

    if (incomingCMD == String("enEncrypt")) //enable encryption
      radio.encrypt(KEY);
  
    if (incomingCMD == String("disEncrypt")) //disable encryption
      radio.encrypt(null);
     
    if (incomingCMD == String("dumpFlash")) //dump flash area
      {
        Serial.println("Flash content:");
        int counter = 0;
  
         while(counter<=256){
          Serial.print(flash.readByte(counter++), HEX);
          Serial.print('.');
        }
        while(flash.busy());
        Serial.println();
      }

    if (incomingCMD == String("eraseChip")) //erase flash chip
      {
        Serial.print("Deleting Flash chip content... ");
        flash.chipErase();
        while(flash.busy());
        Serial.println("DONE");
       }

    if (incomingCMD == String("ID"))//flash ID
      {
        Serial.print("DeviceID: ");
        word jedecid = flash.readDeviceId();
        Serial.println(jedecid, HEX);
      }
    }


    if (incomingCMD == String("XMIT")) //transmit
      {
      //fill in the struct with new values
      theData.cmd = 'W';
      //theData.uptime = millis();
      theData.volume = average;
      
      Serial.print("Sending struct (");
      Serial.print(sizeof(theData));
      Serial.print(" bytes) ... ");
      if (radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData)))
        Serial.print(" ok!");
      else Serial.print(" nothing...");
      Serial.println();
      Blink(LED,3);
      }

}
