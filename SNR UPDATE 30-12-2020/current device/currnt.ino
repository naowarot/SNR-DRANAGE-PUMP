#include <SPI.h>
#include <nRF24L01p.h>

const int sensorIn = A0;
int mVperAmp = 66;
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;
double TrueAmpsRMS = 0;
double AMP;

nRF24L01p radio(15,2);//CSN,CE
String msg;
int current = 0;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  radio.channel(90);
  radio.TXaddress("SNR");
  radio.init();


}

void loop() {
 Voltage = getVPP();
 VRMS = (Voltage/2.0) *0.707;  //root 2 is 0.707
 AmpsRMS = (VRMS * 1000)/mVperAmp;
 AMP = AmpsRMS*20;
 
 Serial.print(AmpsRMS);
 Serial.println("AMP!");
 Serial.print(AMP);
 Serial.println("ampRMS");
 
 Serial.println(AMP);
 radio.txPL(String(AMP)); // ค่าที่ต้องการส่ง
 radio.send(FAST); // สั่งให้ส่งออกไป
 delay(1);
}

float getVPP()
{
  float result;
  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
  int minValue = 1024.0;          // store min value here
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue = analogRead(sensorIn);
       if (readValue > maxValue) 
       {
           maxValue = readValue;
       }
       if (readValue < minValue) 
       {
           minValue = readValue;
       }
   }
   result = ((maxValue - minValue) * 5)/1024.0;  
   return result;
 }
