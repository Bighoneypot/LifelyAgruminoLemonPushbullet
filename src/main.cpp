/*LifelyAgruminoLemonPushbullet(With automatic watering)-
Created by Gabriele Foddis on 04/2023 -  Last update 19/04/2023 -
gabriele.foddis@lifely.cc
This firmware has been developed to send only important notifications for the irrigation and battery status, maximum attention to energy saving.
I used pushbullet excellent platform to send and receive notifications. Ppossible to have a free or premium account at a very low cost. Visit https://www.pushbullet.com/
the device sends messages only when necessary according to the configuration thresholds.
You need to use pushbullet app,available for different platforms
Find Lifely Agrumino Lemon (REV4 AND REV5) in www.lifely.cc 

*/

#include <Arduino.h>
#include <Agrumino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#define DEBUG_MSG_INFO true
#define DEBUG_INFO_MSG if(DEBUG_MSG_INFO)Serial
#define DEBUG_MSG_WARNING true
#define DEBUG_WARNING_MSG if(DEBUG_MSG_WARNING)Serial
#define MIN_TO_MS (1000000 * 60)
#define BAUD_RATE 115200
unsigned int SLEEP_TIME_MIN = 60; ///Max 60 minutes

const char* networkName = "PlumCake";
const char* networkPassword = "ImVeryHappy";
const char* apiPushbullet = "api.pushbullet.com";
const char* yourTokenForPushbullet = "o.NvLgQzsDvGAmGlSJzBYAZg"; //From your https://www.pushbullet.com/ account

int soilMoisture, batteryLevel, tryConnect;
float temperature, lux, batteryVoltage;
bool isBatteryCharging ;
String agruminoName, msgDynamic, qD;

bool enable_watering = true ; //set true or false to Enable/Disable watering
unsigned int wateringTime = 5000; ///time in ms Milliseconds ( 1 seconds = 1000 ms) default 5000


const char* thumbPrint = "D7:60:F4:84:EF:C4:25:4E:14:B7:5A:4B:4C:2E:F6:FD:53:5E:BF:8E"; 
String pushBulletV2 = "/v2/pushes";
int soilMoistureWarning = 30; ///set your value 
int batteryWarning = 25; //set your value
int maxConnAttempts= 100;  ///maximum connection attempts
const int securePort = 443;

String msgWarningSoilM = "Hi Dear, I need water... NOW!!!!!"; //customize as you like
String msgBatteryWarning = "Hi dear, I need Power, recharge my battery";//customize as you like
String msgBatterySoilM ="Hi Dear, I need water and power for my battery... NOW";//customize as you like

Agrumino agrumino;

void watering() {
  if (enable_watering == true){
  agrumino.turnWateringOn();
  DEBUG_INFO_MSG.println("Watering On");
  delay(wateringTime);
  agrumino.turnWateringOff();
  DEBUG_INFO_MSG.println("Watering Off");
  }
}


void readDataFromDevice(){
  DEBUG_INFO_MSG.println("...read data");
  agruminoName = "Agrumino-"+String(ESP.getChipId());
  isBatteryCharging = agrumino.isBatteryCharging();
  temperature = agrumino.readTempC();
  soilMoisture = agrumino.readSoil();
  lux = agrumino.readLux();
  batteryVoltage = agrumino.readBatteryVoltage();
  batteryLevel = agrumino.readBatteryLevel();
  delay(200);
  DEBUG_INFO_MSG.println("...END data");
}

void checkData(){

  if (soilMoisture<soilMoistureWarning && batteryLevel>batteryWarning){  
  msgDynamic=msgWarningSoilM;
  DEBUG_WARNING_MSG.println("Warning soilMoisture");
  watering();
  }
  else if(soilMoisture<soilMoistureWarning && batteryLevel<batteryWarning){
  msgDynamic=msgBatterySoilM;
  DEBUG_WARNING_MSG.println("Warning soilMoisture and battery");
  watering();
  }
  else if(soilMoisture>soilMoistureWarning && batteryLevel<batteryWarning){
  msgDynamic=msgBatteryWarning;
  DEBUG_WARNING_MSG.println("Warning battery");
  }
  else{  ESP.deepSleep(MIN_TO_MS * SLEEP_TIME_MIN);
  DEBUG_INFO_MSG.println("No warning");
  }
}

void setup() {
  Serial.begin(BAUD_RATE);
  agrumino.setup();
  agrumino.turnBoardOn();
  delay(500);
  Serial.println();
  readDataFromDevice();
  checkData();
  DEBUG_INFO_MSG.print("I'm try to connect with ");
  DEBUG_INFO_MSG.println(networkName);
  WiFi.mode(WIFI_STA);
  WiFi.begin(networkName, networkPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tryConnect++;
    if (tryConnect > maxConnAttempts){
        DEBUG_WARNING_MSG.print("..problem with the connection, I will try again in "+ String(SLEEP_TIME_MIN)+" minutes");
        ESP.deepSleep(MIN_TO_MS * SLEEP_TIME_MIN);
    }
  }
  DEBUG_INFO_MSG.println("");
  DEBUG_INFO_MSG.println("WiFi connected");
  DEBUG_INFO_MSG.println("IP address: ");
  DEBUG_INFO_MSG.println(WiFi.localIP());
  WiFiClientSecure AgruminoPushClient;
  AgruminoPushClient.setInsecure(); //don't move
  DEBUG_INFO_MSG.print("Connecting to ");
  DEBUG_INFO_MSG.println(apiPushbullet);
  if (!AgruminoPushClient.connect(apiPushbullet, securePort)) {
    DEBUG_WARNING_MSG.println("Failed to connect");
    return;
  }

  if (AgruminoPushClient.verify(thumbPrint, apiPushbullet)) {
    DEBUG_INFO_MSG.println("The certificate matches, great");
  } else {
    DEBUG_WARNING_MSG.println("The certificate does not match, check it");
  }

  DEBUG_INFO_MSG.println("Dynamic message is: " + msgDynamic);
  qD = "\""; //I need this to semplify escape sequence and add Dynamic title and body from var(Escape sequence for double quote)

  String popUpMessage = "{\"type\": \"note\", \"title\":" ;
  popUpMessage += qD + agruminoName +qD +"," + qD+"body"+qD +":"+qD+msgDynamic+qD+"}\r\n";
  DEBUG_INFO_MSG.println(popUpMessage);
  DEBUG_INFO_MSG.print("Send data in URL: ");
  DEBUG_INFO_MSG.println(pushBulletV2);

  AgruminoPushClient.print(String("POST ") + pushBulletV2 + " HTTP/1.1\r\n" +
               "Host: " + apiPushbullet + "\r\n" +
               "Authorization: Bearer " + yourTokenForPushbullet + "\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length: " +
               String(popUpMessage.length()) + "\r\n\r\n");
  AgruminoPushClient.print(popUpMessage);
  DEBUG_INFO_MSG.println("Request send....");
  while (AgruminoPushClient.available() == 0);
  while (AgruminoPushClient.available()) {
    String line = AgruminoPushClient.readStringUntil('\n');
    DEBUG_INFO_MSG.println(line);
  }
}

void loop() {
  WiFi.disconnect();
  DEBUG_INFO_MSG.println("End loop and sleep");
  ESP.deepSleep(MIN_TO_MS * SLEEP_TIME_MIN);
}