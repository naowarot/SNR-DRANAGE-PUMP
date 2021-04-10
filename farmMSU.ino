#include <Wire.h> 
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <Keypad_I2C.h>

#define SIM800L_IP5306_VERSION_20200811
#include "utilities.h"
#define Serial Serial
#define SerialAT  Serial1

const char apn[]      = "internet";
const char gprsUser[] = ""; 
const char gprsPass[] = ""; 
const char simPIN[]   = ""; 

#include "HX711.h"
HX711 scale(14,25);

// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800     
#define TINY_GSM_RX_BUFFER      1024  

LiquidCrystal_I2C lcd(0x27, 20, 4);
byte buzzer = 15;

float calibration_factor =  34779.00; 
#define zero_factor 164631

// HTTPS Clients
const char server[] = "api.pushingbox.com";
const int  port = 80;
String deviceID = "v06BE1D72E80E703";

//data_base
String data_id[11] = {"","C001","C002","C003","C004","C005","C006","C007","C008","C009","C010"};
float values[11] = {0,0,0,0,0,0,0,0,0,0,0};
int cow_index;

//keypad config  
#define I2CADDR 0x20
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {3, 2, 1, 0};
byte colPins[COLS] = {7, 6, 5, 4};
Keypad_I2C keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR); 

#include <TinyGsmClient.h>
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

void setupModem()
{
#ifdef MODEM_RST
    // Keep reset high
    pinMode(MODEM_RST, OUTPUT);
    digitalWrite(MODEM_RST, HIGH);
#endif

    pinMode(MODEM_PWRKEY, OUTPUT);
    pinMode(MODEM_POWER_ON, OUTPUT);
    digitalWrite(MODEM_POWER_ON, HIGH);

    // Pull down PWRKEY for more than 1 second according to manual requirements
    digitalWrite(MODEM_PWRKEY, HIGH);
    delay(100);
    digitalWrite(MODEM_PWRKEY, LOW);
    delay(1000);
    digitalWrite(MODEM_PWRKEY, HIGH);

    // Initialize the indicator as an output
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, LED_OFF);
    
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    //SIM800L init()
    modem.restart();
    String modemInfo = modem.getModemInfo();
    Serial.print("Modem: ");
    Serial.println(modemInfo);

    // Unlock your SIM card with a PIN if needed
    if (strlen(simPIN) && modem.getSimStatus() != 3 ) {
        modem.simUnlock(simPIN);
    }

    Serial.print("Waiting for network...");
    if (!modem.waitForNetwork(240000L)) {
        Serial.println(" fail");
        delay(5000);
        return;
    }
    Serial.println(" OK");

    // When the network connection is successful, turn on the indicator
    digitalWrite(LED_GPIO, LED_ON);

    if (modem.isNetworkConnected()) {
        Serial.println("Network connected");
    }
    //connect network
    Serial.print(F("Connecting to APN: "));
    Serial.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        Serial.println(" fail");
        delay(10000);
        return;
    }
    Serial.println(" OK");
    delay(2000);
}
void setup() {
  Serial.begin(9600);
  Wire.begin( ); 
  lcd.begin();
  delay(500);
  show("-STARTING-",0);
  setupModem();
  //modem.gprsDisconnect();
  keypad.begin( makeKeymap(keys) ); 
  pinMode(buzzer,OUTPUT);
  loadcell_init();
  lcd.clear();
  //pushData(1,2,3,4,5,6,7,8,9,10);
  modem.gprsDisconnect();
}
void loop() {
  menu_page();
  //first_page();
}
void loadcell_init(){
  scale.set_scale(calibration_factor); 
  scale.set_offset(zero_factor);     
}

float get_units_kg()
{
  return(scale.get_units()*0.453592);
}
void show(String msg, byte y){
  lcd.setCursor(0,y);
  lcd.print(msg);
}
void menu_page(){
  show("(1) Milk Meter",  0);
  show("(2) Setting", 1); 
  char key = keypad.getKey();;
  switch(key){
    case '1':
      sound_bepp();
      Serial.println(F("\nMeasure"));
      measure();
      break;
    case '2':
      sound_bepp();
      Serial.println(F("\nSETTING!"));
      setting();
      break;
    }
}
void setting(){
  lcd.clear();
  bool state = true;
  while(state == true){
    show("(1) WiFi",0);
    show("(2) Cellular",1);
    char key = keypad.getKey();
    switch(key){
      case '1':
        sound_bepp();
        lcd.clear();
        state = false;
        break;
      case '2':
        sound_bepp();
        lcd.clear();
        show("    Cellular Mode    ",0);
        delay(2000);
        lcd.clear();
        state = false;
        break;
      case '*':
        sound_bepp();
        state = false;
        lcd.clear();
        break;
    }
  }
}
void measure(){
  char txt[5] = "";
  byte index  = 0;
  lcd.clear();
  while(1){
    char key = keypad.getKey();
    if(key){
      sound_bepp();
      txt[index] = key;
      index++;
      Serial.println(txt);
    }
    if(key == 'D'){
      lcd.clear();
      show("  Sending Data  ", 0);
      pushData(values[1],  values[2],  values[3],  values[4],  values[5],  values[6],  values[7],  values[8],  values[9],  values[10]);
      delay(1000);
      lcd.clear();
      show("  DATA WAS SENT  ", 0);
      sound_long_bepp();
      lcd.clear();
      delay(2000);
      break;
    }
    show("      ENTER ID     ",0);
    show("    ->  " + String(txt) + "  ", 1);
    if(index >= 4){
      delay(500);
      lcd.clear();
      String x = txt;
      cow_index = 0;
      while(cow_index <= 10){
        Serial.println("Searching ID");
        if(x == data_id[cow_index]){
          scale.tare(); 
          Serial.println("ID detected");
          while(1){
            values[cow_index] = get_units_kg();
            show("CowID : " + String(data_id[cow_index]) + "", 0);
            show("Value : " + String(values[cow_index]) + "   KG ", 1);
            char x = keypad.getKey();
            if(x == 'D'){
              sound_bepp();
              lcd.clear();
              memset(txt, 0, 5);
              index = 0;
              lcd.clear();
              scale.tare(); 
              break;
            }
          }
        }
        cow_index++;  
      }
      memset(txt, 0,  5);
      index = 0;
    }//if(index>=4)
  }
}

void sound_bepp(){
  digitalWrite(buzzer,HIGH);
  delay(100);
  digitalWrite(buzzer,LOW);
}
void sound_long_bepp(){
  digitalWrite(buzzer,HIGH);
  delay(1000);
  digitalWrite(buzzer,LOW);
}

void pushData(float v1,float v2,float v3,float v4,float v5,float v6,float v7,float v8,float v9,float v10){
    if (client.connect(server, 80)) {
      String url = "/pushingbox?devid=" + deviceID
                   +"&value1=" + String(v1) + "&value2=" + String(v2) + "&value3=" + String(v3) + "&value4=" + String(v4) + "&value5=" + String(v5) +"&value6=" + String(v6)
                   +"&value7=" + String(v7) +"&value8=" + String(v8) +"&value9=" + String(v9) + "&value10=" + String(v10);
                   
      Serial.println("SENDING!");   
      Serial.println("Requesting URL");           
      client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + server + "\r\n" + "Connection: close\r\n\r\n");
      client.println();
      client.flush();
      }
    while(!client.connect(server, 80)) {
      Serial.println("connection failed");
      return;
    }
    unsigned long timeout = millis();
    while (client.available() >= 0) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
      }
      client.stop();
      return;
    }
    Serial.println(F("DATA WAS SENT!"));    
}

void isGprs(){
  Serial.print(F("GPRS Connection checking"));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
    Serial.println(" OK");
}
