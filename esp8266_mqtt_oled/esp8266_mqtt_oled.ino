/*
 * Simple sketch for displaying tempature and humidity on a ebay-special .96 OLED display
 * connected to a NodeMCU with base shield via I2C.  The data is being pulled from MQTT
 * server hosted by cloudmqtt.com.
 * 
 * The data is from 4 Arduino based sensors - 3x indoor and 1x outdoor.  The data is being
 * published as a JSON object:
 * 6164360a-XXXX-XXXX-XXXX-132579b1a97b/XX/JSON/{"Device":22,"SensorType":1,"RSSI":-56,"Temp":65.07,"DewPoint":49.84,"Humidity":57.74,"Volts":3.363}
 * 
 * Must modify PubSubClient.h and increase MQTT_MAX_PACKET_SIZE to 256, otherwise the MQTT payload publish will fail.
 * 
 * Usine Arduino IDE 1.9.6 to get Subscribing via MQTTS to work.
 */

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "wifi_name"
#define WLAN_PASS       "wifi_password"

/*************************   Other Setup    *********************************/

#define NETWORKID   33              //  network id from RFM69 network
#define displayID   "Office"        //  dispaly name for mqtt check-in
#define NTPHOST     "pool.ntp.org"
#define TZOFFSET    -25200          //  offset from GMT in MS (-7 * 60 * 60)
#define NTPINTERVAL 60000           //  refresh time in MS

/*************************    MQTT Setup    *********************************/

#define MQTT_SERVER      "some_cloudmqtt_host"
#define MQTT_SERVERPORT  mqtts_port                   // use 8883 for SSL
#define MQTT_USERNAME    "some_username"
#define MQTT_PASSWORD    "some_userpassword"
#define MQTT_UUID        "some_uuid_that_you_generated"
#define MQTT_SOCKET_TIMEOUT 60

/************************* Library Includes *********************************/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <ArduinoJson.h>

/************ Global State (you don't need to change this!) ******************/

WiFiClientSecure ipStack;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTPHOST, TZOFFSET, 60000);
//
byte mac[6];
long rssi = WiFi.RSSI();
char IP[20];
char MAC[20];
char RSSI[5];
char DateTime[20];
char mqttTopic[64];
char mqttSub[64];
char Payload[256];
char message_buff[100];
char msgBuf[100];
char macBuf[20];
char ipBuf[17];
char Row1[17] = "";
char Row2[17] = "";
char Row3[17] = "";
char Row4[17] = "";

/************************* Payload Callback *********************************/

//
// handles message arrived on subscribed topic(s)
void callback(char* mqttTopic, byte* mqttPayload, unsigned int length) {
  //
  StaticJsonBuffer<200> jsonBuffer;
  //
  int i = 0;
  //
  // create character buffer with ending null terminator (string)
  for (i = 0; i < length; i++) {
    message_buff[i] = mqttPayload[i];
  }
  message_buff[i] = '\0';
  //
  String msgString = String(message_buff);
  Serial.println("Payload: " + msgString);
  msgString.toCharArray(msgBuf, 20);
  JsonObject& root = jsonBuffer.parseObject(message_buff);
  //
  //  check buffer for which device
  if (strstr(msgBuf, "\"Device\":11"))
  {
    const char* device1 = root["Device"];
    float temp1 = root["Temp"];
    float humidity1 = root["Humidity"];
    char tempbuf1[6];
    char humbuf1[6];
    dtostrf(temp1, 3, 0, tempbuf1);
    dtostrf(humidity1, 3, 0, humbuf1);
    //  dtostrf handles the rounding
    sprintf(Row1, "%s: %sF %s%%", device1, tempbuf1, humbuf1);
  }
  //
  if (strstr(msgBuf, "\"Device\":22"))
  {
    const char* device2 = root["Device"];
    float temp2 = root["Temp"];
    float humidity2 = root["Humidity"];
    char tempbuf2[6];
    char humbuf2[6];
    dtostrf(temp2, 3, 0, tempbuf2);
    dtostrf(humidity2, 3, 0, humbuf2);
    //  dtostrf handles the rounding
    sprintf(Row2, "%s: %sF %s%%", device2, tempbuf2, humbuf2);
  }
  //
  if (strstr(msgBuf, "\"Device\":33"))
  {
    const char* device3 = root["Device"];
    float temp3 = root["Temp"];
    float humidity3 = root["Humidity"];
    char tempbuf3[6];
    char humbuf3[6];
    dtostrf(temp3, 3, 0, tempbuf3);
    dtostrf(humidity3, 3, 0, humbuf3);
    //  dtostrf handles the rounding
    sprintf(Row3, "%s: %sF %s%%", device3, tempbuf3, humbuf3);
  }
  //
  if (strstr(msgBuf, "\"Device\":4"))
  {
    const char* device4 = root["Device"];
    float temp4 = root["Temp"];
    float humidity4 = root["Humidity"];
    char tempbuf4[6];
    char humbuf4[6];
    dtostrf(temp4, 3, 0, tempbuf4);
    dtostrf(humidity4, 3, 0, humbuf4);
    //  dtostrf handles the rounding
    sprintf(Row4, " %s: %sF %s%%", device4, tempbuf4, humbuf4);
  }
  //
  //  print datetime header
  if (hour() > 11)
  {
    sprintf(DateTime, "%2d/%02d/%2d %2d:%02dPM", month(), day(), year() - 2000, hourFormat12(), minute());
  }
  else
  {
    sprintf(DateTime, "%2d/%02d/%2d %2d:%02dAM", month(), day(), year() - 2000, hourFormat12(), minute());
  }
  //
  Serial.println(DateTime);
  Serial.println(Row3);
  Serial.println(Row2);
  Serial.println(Row1);
  Serial.println(Row4);
  ClearDisplay();
  sendStrXY(DateTime, 0, 0);
  sendStrXY(Row3, 1, 0);
  sendStrXY(Row2, 3, 0);
  sendStrXY(Row1, 5, 0);
  sendStrXY(Row4, 7, 0);
}
//
PubSubClient mqttClient(MQTT_SERVER, MQTT_SERVERPORT, callback, ipStack);

/****************************************************************************/

void setup() {
  //
  Serial.begin(115200);
  delay(2000);
  Serial.flush();
  Serial.println();
  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
  Serial.print("Boot Vers: "); Serial.println(ESP.getBootVersion());
  Serial.print("CPU: "); Serial.println(ESP.getCpuFreqMHz());
  Serial.print("ChipId: "); Serial.println(ESP.getChipId());
  Serial.print("FlashChipId: "); Serial.println(ESP.getFlashChipId());
  Serial.println();
  //
  Serial.println("Setting up OLED...");
  Wire.begin(4, 5);
  StartUpOLED();
  ClearDisplay();
  Serial.println("Sending output to OLED...");
  sendStrXY(" NodeSync ", 0, 3); // 16 Character max per line with font set
  sendStrXY("MQTTS Display", 1, 2);
  sendStrXY("Starting up....  ", 3, 0);
  delay(5000);
  Serial.println("OLED Setup done...");
  //
  // Connect to WiFi access point.
  sendStrXY("WiFi init...", 4, 0);
  Serial.print("Connecting to: ");
  Serial.println(WLAN_SSID);
  //
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  //
  Serial.println("WiFi connected");
  WiFi.macAddress(mac);
  sprintf(macBuf, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print("MAC: "); Serial.println(macBuf);
  //
  Serial.print("RSSI: "); Serial.println(rssi);
  //
  IPAddress ip = WiFi.localIP();
  sprintf(ipBuf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  Serial.print("IP Address: "); Serial.println(ipBuf);
  //
  String rssiBuf = String(rssi);
  rssiBuf.toCharArray(RSSI, 5);
  //
  //  print wifi settings to oled
  ClearDisplay();
  sendStrXY("WiFi Started:", 0, 0);
  sendStrXY(WLAN_SSID, 1, 0);
  sendStrXY("IP: ", 3, 0);
  sendStrXY(ipBuf, 4, 0);
  sendStrXY("MAC:", 5, 0);
  sendStrXY(macBuf, 6, 0);
  //
  delay(5000);
  //
  //  NTP Related
  timeClient.begin();
  timeClient.update();
  setTime(timeClient.getEpochTime());
  ClearDisplay();
  sendStrXY("NTP Started:", 0, 0);
  // print timestamp
  if (hour() > 11)
  {
    sprintf(DateTime, "%2d/%02d/%2d %2d:%02dPM", month(), day(), year() - 2000, hourFormat12(), minute());
  }
  else
  {
    sprintf(DateTime, "%2d/%02d/%2d %2d:%02dAM", month(), day(), year() - 2000, hourFormat12(), minute());
  }
  //
  Serial.println(DateTime);
  sendStrXY(DateTime, 1, 0);
  //
  //  MQTT setup
  sendStrXY("MQTT pubsub:", 2, 0);
  sprintf(mqttTopic, "%s/%i/%s", MQTT_UUID, NETWORKID, displayID);
  sprintf(mqttSub, "%s/%i/JSON/#", MQTT_UUID, NETWORKID);
  //
  if (mqttClient.connect("NodeSync", MQTT_USERNAME, MQTT_PASSWORD)) {
    mqttClient.publish(mqttTopic, "Online");
    mqttClient.subscribe(mqttSub);
  }
}
//
//

/************************* Main *********************************/

void loop() {
  //
  //
  timeClient.update();
  setTime(timeClient.getEpochTime());
  //
  mqttClient.loop();
}
//
//

