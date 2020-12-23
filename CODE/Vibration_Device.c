#include <SPI.h>
#include "I2Cdev.h"
#include "Wire.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "MPU6050.h"
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <nRF24L01p.h>

WiFiClient client;
LiquidCrystal_I2C lcd(0x27, 16, 2);
MPU6050 mpu;
nRF24L01p radio(15,2);

//WIFI SECURE
char ssid[] = "SNR-WIFI";
char pass[] = "";

//GOOGLE SHEET! HTTPS CLIENT!
const char* host = "api.pushingbox.com";
String deviceID = "v84321DAB2FEC5FC";
const int httpPort = 80;

// I/O PIN
byte spPin = 16;
byte spSw;

//CURRENT AND VIBRATION VARIBLES
int16_t ax, ay, az, gx, gy, gz;
uint8_t valx , valy , valz;
uint8_t spx, spy, spz;
uint8_t peakx,peaky,peakz;
uint8_t xMax, yMax, zMax, ampMax;
uint8_t maxAmp, maxX, maxY, maxZ;
uint8_t avgAmp, avgX, avgY, avgZ;
int totalAmp, totalX, totalY, totalZ;
uint8_t current = 0;
int avgIndex;

//FUNCTION STATE CONTROL VARIBLES
unsigned long samplingTime, time2;
unsigned long runTimeBegin = 0;
unsigned long runTime;
bool startState = false;

//DISPLAY VARIBLE
String msg1,msg2,msg3,msg4,msg5,spMSG;
String msgRadio;
int val;

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(4);
  Wire.begin();
  lcd.begin();
  lcdSingle("    STARTING    ",0,0);
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  pinMode(spPin,INPUT);
  lcdSingle("    SNR WiFi    ",0,1);
  WiFiInit();
  Serial.println(F("Initialize MPU"));
  mpu.initialize();
  Serial.println(mpu.testConnection() ? "Connected" : "Connection failed");  

  // READ SETPOINT VALUE FROM MEMORY
  spx = EEPROM.read(0);
  spy = EEPROM.read(1);
  spz = EEPROM.read(2);
  Serial.println(spx);
  Serial.println(spy);
  Serial.println(spz);
  
  // RADIO CONFIG
  radio.channel(90);
  radio.RXaddress("SNR");
  radio.init();
  Serial.println(F("DEVICE READY!"));
  lcdSingle("    STARTED!   ",0,0);
  delay(1000);
  lcd.clear();
  lcd.noBacklight();
  
}

void loop()
{
  if(radio.available()){
    radio.read();                                                   // สั่งให้เริ่มอ่าน
    radio.rxPL(msgRadio);                                           // สั่งใหอ่านเก็บไว้ที่ตัวแปร
    current = msgRadio.toInt();
    msgRadio = "";
  }
  
  setPoint();                                                       //SETPIONT!
  
  if(current > 25 && current < 9999){                               // 9999 is returned value when RF was disconnected!
    lcd.backlight();
    startState = true;
    runTimeBegin = millis();
    avgIndex = 0;
    Serial.println(F("MOTOR START!"));
    delay(1000);                                                     // Delay for Start Current 7Sec
    
    while(startState == true)                                        //start State at RATED current
    {                        
      if(radio.available()){
        radio.read();                                                // สั่งให้เริ่มอ่านกระแส
        radio.rxPL(msgRadio);                                        // สั่งใหอ่านเก็บไว้ที่ตัวแปร
        current = msgRadio.toInt();                                  // แปลง String เป็น Int
        msgRadio = "";                                               // reset ค่าที่อ่านได้ เพื่อรอรับค่าใหม่
      }
/*                                                                    
      mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);                   //อ่านค่า Vibration
      valx = map(ax, -17000, 17000, 0, 179);
      valy = map(ay, -17000, 17000, 0, 179);
      valz = map(az, -17000, 17000, 0, 179);
      
      
      //GET X Y Z ERROR
      peakx = getPeak(valx,spx);                                     //หาค่าเบี่ยงเบนจาก  SETPOINT
      peaky = getPeak(valy,spy);
      peakz = getPeak(valz,spz);
*/     
   
      noSensor();                                                    //Vibration Test สำหรับไม่ได้ต่อ Sensor
      
      //MAX VALUES
      ampMax = getAmpMax(current);                                   //หาค่าสูงสุด
      xMax = max(xMax,peakx);
      yMax = max(yMax,peaky);
      zMax = max(zMax,peakz);

    //GET AVERAGE VALUE                                             //หาค่าเฉลี่ยของ Vibration และ Current
      if(millis() - samplingTime >= 1000){
        samplingTime = millis();
    
        totalAmp  = totalAmp + current;
        totalX = totalX + peakx;
        totalY = totalY + peaky;
        totalZ = totalZ + peakz;
    
        avgIndex = avgIndex + 1;

        avgAmp = totalAmp / avgIndex;
        avgX = totalX / avgIndex;
        avgY = totalY / avgIndex;
        avgZ = totalZ / avgIndex;
      }
  
        setPoint();                                                 //กำหนด SETPOINT
        Display();                                                  //แสดงจอ LCD
      
        //EXIT WHILE LOOP FUNC
        if(current < 20){                                           //ออกจาก  While loop เมื่อกระแสต่ำกว่า 20A
          startState =  false; 
        }
      
    }
      
      runTime = millis() - runTimeBegin;                            //หาค่าเวลามอเตอร์ทำงาน
      runTime = runTime / 1000;                                     //แปลง MicroSec เป็น Sec

      
      lcd.clear();
      lcdSingle("  PUSHING DATA!   ",0,0);
      
      Serial.print(F("PUMP PERIOD : "));
      Serial.print(runTime);
      Serial.println(F(" Sec"));
      
      pushData(avgAmp,ampMax,avgX,xMax,avgY,yMax,avgZ,zMax,runTime);  //เรียกใช้ฟังก์ชัน pushData(); 

      //RESET VALUE                                                   //รีเซ็ตค่า เพื่อรอรับค่าใหม่
      totalAmp = 0;
      totalX = 0;
      totalY = 0;
      totalZ = 0;
      
      xMax = 0;
      yMax = 0;
      zMax = 0;
      ampMax = 0;
       
      delay(1000);
      lcd.clear();
      lcd.noBacklight();
   }
  delay(100);
}

void setPoint(){                                                              //ฟังก์ชั่นกำหนด SetPoint
  spSw = digitalRead(spPin);
  if(spSw == 0 || (spx + spy + spz) == 0){                                    //กดปุ่มเพื่อกำหนด SETPOINT  หรือ (SetPoint XYZ = 0)
    bool state1 = HIGH;
    
    Serial.print("CONFIRM SETPOINT ? ");
    lcdSingle("    CONFIRM ?    ",0,1);
    spMSG = "X:" + String(valx) +" Y:"+ String(valy) +" Z:"+  String(valz);
    lcdSingle(spMSG,0,0);
    
    delay(2000);
    while(state1 == HIGH){
      Serial.println(spMSG);
      spSw = digitalRead(spPin);
      if(spSw == 0){
        
        state1 = LOW;
        Serial.println("EEPROME SAVED!");
        lcd.clear();
        lcdSingle("CONFIRM!",4,0);
        
        //SAVE Set Point                                           //บันทึก SETPOINT ไว้ที่ Memory
        spx = valx;
        spy = valy;
        spz = valz;
        EEPROM.write(0,spx);
        EEPROM.write(1,spy);
        EEPROM.write(2,spz);
        EEPROM.commit();
        
        delay(1000);
        lcd.clear();
      }
    }
  }
}

// GET DEVIATION VALUE FROM SETPOINT!
int getPeak(int val,int sp){
  return abs(val-sp);
}

//GET MAX CURRENT VALUE!
int getAmpMax(int val){
  if(val > ampMax && val < 999){
    ampMax = val;
  }
  return ampMax;
}

//FOR TESTING THIS DEVICE WITHOUT SENSOR
void noSensor(){                                                          
  unsigned long ranTime;
  if(millis() - ranTime >= 1000){
    
    //DUMMPY VALUE FOR TEST!
    ranTime = millis();
    valx = random(0,10);
    valy = random(0,10);
    valz = random(0,10);

    peakx = getPeak(valx,0);
    peaky = getPeak(valy,0);
    peakz = getPeak(valz,0);  
    delay(10);
  }
}

//FOR CONTROL LCD EASIER!
void lcdSingle(String msg, int x, int y){
  lcd.setCursor(x,y);
  lcd.print(msg);
}

//LCD DISPLAY
void Display(){
  if(millis() - time2 > 300){
    time2 = millis();
    msg1 = "ERRO : " + String(peakx) + " " + String(peaky) + " " + String(peakz) + "      ";      //ค่าเบี่ยงเบนจาก set point
    msg2 = "AMP :" + String(current) + "/" + String(avgAmp) + "/" + String(ampMax) + " A    ";    //ค่ากระแส กระแสปัจจุบัน/เฉลี่ย/สูงสุด
    msg3 = "X:Y:Z: " + String(valx)  +" "+ String(valy) +" "+ String(valz);                       //ค่าจาก sensor
    msg4 = "PEAK : " + String(xMax) + " " + String(yMax) + " " + String(zMax) + "         ";      // Vibration Peak
    msg5 = "AVG :" + String(avgX) +  " "  + String(avgY) + " "  + String(avgZ);                   // Vibration Average Value
    
    //Serial Monitor
    Serial.println(msg1);
    Serial.println(msg4);
    Serial.println(msg3);
    Serial.println(msg2);
    Serial.println(msg5);
    Serial.print(F("Time : "));
    Serial.print((millis() - runTimeBegin) / 1000);
    Serial.println(F("  Sec"));
    Serial.println(F("----------"));

    //LCD DISPLAY
    lcdSingle(msg5,0,0);
    lcdSingle(msg2,0,1);
  }
}

//WiFi SETTING FUNC!
void WiFiInit(){
  Serial.println();
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(2000);
}


//PUSH DATA TO GOOGLE SHEET!
void pushData(float avgCur, float peakCur,  float avgX,  float peakX,  float avgY,  float peakY,  float avgZ,  float peakZ,  float duration){
    if (!client.connect(host, 80)) {
      Serial.println("connection failed");
      return;
    }
    if (client.connect(host, 80)) {
      String url = "/pushingbox?devid=" + deviceID
                   +"&value1=" + String(avgCur) + "&value2=" + String(peakCur) + "&value3=" + String(peakX) + "&value4=" + String(avgX) 
                   +"&value5=" + String(avgY) + "&value6=" + String(peakY) + "&value7=" + String(avgZ) + "&value8=" + String(peakZ) +"&value9=" + String(duration);
                   
      Serial.println("Requesting URL");           
      client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
      client.println();
      client.flush();
      }
      while(!client.connect(host, 80)) {
        Serial.println("connection failed");
        return;
      }
      unsigned long timeout = millis();
      while (client.available() == 0) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
        if (millis() - timeout > 5000) {
          Serial.println(">>> Client Timeout !");
        }
        client.stop();
        return;
    }
}
