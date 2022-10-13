/*
  ModbusRTU ESP8266/ESP32
  Read multiple coils from slave device example
  (c)2019 Alexander Emelianov (a.m.emelianov@gmail.com)
  https://github.com/emelianov/modbus-esp8266
  modified 13 May 2020
  by brainelectronics
  This code is licensed under the BSD New License. See LICENSE.txt for more info.
*/
//watchdog lib
//#include 
//---------------------
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//------------------------
#include <ModbusRTU.h>
#if defined(ESP8266)
#include <SoftwareSerial.h>
char data[80];
String topicName;
//const int deviceId = 500;
char topic[30];
 
 // SoftwareSerial S(D1, D2, false, 256);
 // receivePin, transmitPin, inverse_logic, bufSize, isrBufSize
 // connect RX to D2 (GPIO4, Arduino pin 4), TX to D1 (GPIO5, Arduino pin 4)
 SoftwareSerial S(D2, D1);
#endif
union fsend_t{
 float f;
 uint16_t u[2];
};
union fsend_t floatConvert;
const int TXE_Pin =  D4; // D4 / GPIO2
float tmp;
uint16_t reg_bufer[4]; // Hold 8 regs
//-----------------------
const char* ssid = ""; //Enter your WiFi name         //---------------------------------------
const char* password =  ""; // Enter WiFi password              //---------------------------------------
const char* mqttServer = "192.168.0.0";
//const char* mqttServer = "localhost";
const int mqttPort = 1883;
const char* mqttUser = "SET YOUR USERNAME HERE";                                   //---------------------------------------
const char* mqttPassword = "SET YOUR PASSWORD HERE";
const int keepAlive = 0;
int restartFlag = 0;
WiFiClient espClient;
PubSubClient client(espClient);
String temp_str; //see last code block below use these to convert the float that you get back from DHT to a string =str
char temp[50];
//---------------------------
ModbusRTU mb;
//#define PARAM_NUM   4
#define PARAM_NUM   5
float params[PARAM_NUM] = {0};
/* Addresses
0 - V1N
2 - V2N
4 - V3N
56 - Frequency
*/
uint16_t offsets[PARAM_NUM] = {0, 2, 4, 56};
//uint16_t offsets[PARAM_NUM] = {24, 26, 28, 88, 104};
//uint16_t offsets[PARAM_NUM] = {53, 65, 67, 69, 79};
uint8_t offset_index = 0;


bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void* data) 
{
  //Serial.printf_P("Request result: 0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
  if(event == 0x00)
  {
    floatConvert.u[1] = reg_bufer[1];
    floatConvert.u[0] = reg_bufer[0];
    tmp = floatConvert.f;
    params[offset_index] = floatConvert.f;
    //Serial.printf_P("[%d]V1: %0.2f\n", offset_index, tmp);
    offset_index++;
    if(offset_index >= PARAM_NUM)
    {
      offset_index = 0;
    }
  } 
  else 
  {
    // On not success
    Serial.printf_P("Request result: 0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
  }
  return true;
}

void callback(char* topic, byte* payload, unsigned int length) 
{
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (int i = 0; (unsigned)i < length; i++) 
  {
    Serial.print((char)payload[i]);
  }
 
  Serial.println();
  Serial.println("-----------------------");
}

void setup() 
{
  Serial.begin(115200);
  int wifiFlag = 0;
  int mqttFlag = 0;
  //-----------------
 WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.println("Connecting to WiFi..");
    wifiFlag++;
      if (wifiFlag > 1200) 
      {
      ESP.restart();
      }
  }
  Serial.println("Connected to the WiFi network");
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
    while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    client.setKeepAlive(300);
    if (client.connect(mqttUser, mqttUser, mqttPassword )) 
    {                                
    //if (client.connect("ESP8266Client")) { change client name to connect different devices
    // with same internet.
 
      Serial.println("connected");
      mqttFlag = 0;
 
    } 
    else 
    {
      
      Serial.print("failed with state ");
      Serial.print(client.state());
      mqttFlag++;
      Serial.println(mqttFlag);
      delay(2000);
      if (mqttFlag > 300)
      {
        ESP.restart();
      }
    }
  }
  //client.publish("esp/test", String(params[0])); //Topic name
  //client.subscribe("esp/test")
 
  //-------------------
  S.begin(9600, SWSERIAL_8N1);
  mb.begin(&S, TXE_Pin);
  mb.master();
}

//---------------
void loop() 
{
  //String myStringe = "";
  //myStringe.concat(params[0]);
  //Serial.println(myStringe);
  if (!mb.slave())
  {
    // Print after collect all params
    if(offset_index == 0)
    {
     Serial.printf_P("P1: %0.2f", params[0]);
     Serial.printf_P(" P2: %0.2f", params[1]);
     Serial.printf_P(" P3: %0.2f", params[2]);
     Serial.printf_P(" TotalKwh: %0.1f", params[3]);
     Serial.printf_P(" TotalkVarh: %0.1f\n", params[4]);
     client.loop();
    }
    // Params:
    // Device ID, register, result buffer, count, callback
    mb.readIreg(1, offsets[offset_index], &reg_bufer[0], 2, cbWrite);
  }
  mb.task();
  yield();
  //delay(100);
  //String value = "test";
  //topicName = "solarity/apc_" + String(deviceId) + "/chargers/powermeter";
  //Serial.println(topicName);
  const int meter = 1;
  String meterId = "\"MeterID\": " + String(meter);
  String param0 = ", \"ActPowPh1kW\": " + String(params[0]);
  String param1 = ", \"ActPowPh2kW\": " + String(params[1]);
  String param2 = ", \"ActPowPh3kW\": " + String(params[2]);
  String param3 = ", \"TotActEnerImpkWh\": " + String(params[3]);
  String param4 = ", \"TotReactEnerImpkVARh\": " + String(params[4]);
  
  temp_str = meterId + param0 + param1 + param2 + param3 + param4; //converting ftemp (the float variable above) to a string 
  //temp_str = String(params[0]); //converting ftemp (the float variable above) to a string 
  //temp_str.toCharArray(temp, temp_str.length() + 1); //packaging up the data to publish to mqtt whoa...
  String payloadMsg = "{" + temp_str + "}";
  payloadMsg.toCharArray(data, (payloadMsg.length() + 1));
  //topicName  = "solarity/apc_" + String(deviceId) + "/chargers/powermeter";
  //String payloadTopic = topicName;
  
  //payloadTopic.toCharArray(topic, (payloadTopic.length() + 1));
  delay(5000);
  //client.publish(topic, data);
  client.publish("solarity/apc_130/power/meters/data", data);                                                       //---------------------------------------------------------------------
  //delay(5000);
  //client.publish("esp/test", temp); //Topic name
  Serial.println("Data send to broker!");
  Serial.println(client.state());
  delay(1000);
  if (client.state() == -3)
  {
    ESP.restart();
  }
  restartFlag++;
  if (restartFlag >= 2100) 
  {
    ESP.restart();
  }
}
