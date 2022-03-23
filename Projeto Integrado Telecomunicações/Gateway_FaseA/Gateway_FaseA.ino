#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "ThingSpeak.h"
#include <stdlib.h>
#include <WiFi.h>
#include <stdio.h>
#include <chrono>;
#include <ctime>;
#include "INTINFO.h"

#define Server_Name "GATEWAY_SENSORES"
#define MAX_TIME_SEND 20000
//Variáveis ligadas ao BLE

static BLEUUID UUID_SERVER_SERVICE ("72add083-b931-4efc-ba71-4eaf935e0465");
static BLEUUID UUID_Characteristic_Server ("cf15383a-afe7-4d44-b24d-a6363e93793e");
static BLEUUID UUID_TEMP_CHARACTERISTIC ("eea1aa7d-b9b9-4ddf-a575-05b7a37b139c"); //Tentar receber todos os dados na forma de uma só caracteristica de modo a poupar trabalho
static BLEUUID UUID_HUMD_CHARACTERISTIC ("9d9031b5-191f-4e2d-9791-1c2240e74a8d");
static BLEUUID UUID_PRESS_CHARACTERISTIC ("01bcb788-7258-43b3-be93-f35dad0ac2f4");

BLEServer *Gateway_Server_BLE;
BLEService *Gateway_Service;
BLECharacteristic *Gateway_Received_Charac;
BLECharacteristic *Gateway_Characteristic_TEMP;
BLECharacteristic *Gateway_Characteristic_HUMD;
BLECharacteristic *Gateway_Characteristic_PRESS;
//------------------------

// Variáveis ligadas à WiFi


char* ssid = ssidhotspot;
char* password = passwordhotspot;


WiFiClient cliente_internet;


unsigned long myChannelnum = 1666652;
const char *MyApiKey = "DZCFYQTOXYIUTK2J"; // Substituir por chave do canal do thingspeak

const char* ntpServer = "pool.ntp.org";// Para obter a data e  tempo usamos um server que nos dá estes dados automáticamente
const long gmtOffset_sec = 0; //Offset da zona temporal, como estamos em Portugal, o valor é de 0
const int daylightOffset_sec = 0; //Tem a haver com o horário de inverno / verão

unsigned long timer_delay = 0;
// -------------------------------

//Variáveis gerais
String MSG_STOP = "STOP"; //Como o BLE facilita o envio de Strings , usamos uma String como meio de fazer STOP ou START dos sensores
String MSG_START = "START";
String MSG_CHANGETIME = "DELAY";

boolean deviceConnected = false;
boolean sensor_temp_enviar = false;
boolean sensor_humd_enviar = false;
boolean sensor_press_enviar = false;
boolean caracteristica_pressure = false;
boolean caracteristica_temp = false;
boolean caracteristica_humd = false;
boolean WiFi_Connected = false;
char value_temp[6];
char value_humd[6];
char value_pressure[6];
byte pacote_comando[2];
float temp;
float humd;
float pressure;
float humd_send;
float temp_send;
float pressure_send;
int test_speak = 0;

//-------------------------------
void make_timestamp();
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("Device Connected");
      deviceConnected = true;
    }
    void onDisconnect(BLEServer* pServer) {

    }
};

class MyCallbacks: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
      std::string value;
      if ((pCharacteristic->getUUID().equals(UUID_TEMP_CHARACTERISTIC)) && !caracteristica_temp) {
        caracteristica_temp = true;
        value = pCharacteristic->getValue();
      }
      if ((pCharacteristic->getUUID().equals(UUID_HUMD_CHARACTERISTIC)) && !caracteristica_humd) {
        caracteristica_humd = true;
        value = pCharacteristic->getValue();
      }
      if ((pCharacteristic->getUUID().equals(UUID_PRESS_CHARACTERISTIC)) && !caracteristica_pressure) {
        caracteristica_pressure = true;
        value = pCharacteristic->getValue();
      }
      if (value.length() > 0)
      {
        for (int i = 0; i < value.length(); i++)
        {
          // Serial.print((char)value[i]);
          if (caracteristica_temp) {
            value_temp[i] = value[i];
            //Serial.print("temp" );
           // Serial.print((char)value_temp[i]);
            sensor_temp_enviar = true;
          }
          if (caracteristica_humd) {
            value_humd[i] = value[i];
           // Serial.print("humd" );
           // Serial.print((char)value_humd[i]);
            sensor_humd_enviar = true;
          }
          if (caracteristica_pressure) {
            value_pressure[i] = value[i];
            //Serial.print("press" );
           // Serial.print((char)value_pressure[i]);
            sensor_press_enviar = true;
          }
        }
        value_temp[5] = '\0';
        value_humd[5] = '\0';
        value_pressure[5] = '\0';
        // Serial.println(" ");
        if ((sensor_temp_enviar)) {
          temp_send = atof(value_temp);
          make_timestamp();
          Serial.print(" Temperatura Recebida: ");
          Serial.println(temp_send);
        }
        if ((sensor_humd_enviar)) {
          humd_send = atof(value_humd);
          make_timestamp();
          Serial.print(" Humidade Recebida: ");
          Serial.println(humd_send);
        }
        if ((sensor_press_enviar)) {
          pressure_send = atof(value_pressure);
          make_timestamp();
          Serial.print(" Pressão Recebida: ");
          Serial.println(pressure_send);
        }
        caracteristica_temp = false;
        caracteristica_humd = false;
        caracteristica_pressure = false;
        /*Serial.println(temp_send);
        Serial.println(humd_send);
        Serial.println(pressure_send);*/
      }
    }
};

void check_input(){
   if(Serial.available()>0){
    char temp = Serial.read();
    if(temp == '1'){
      Serial.println("STOP");
      char temp2[2] = "N";
      temp2[2] = '\0';
      //Gateway_Received_Charac->setValue((char)byteSTOP);
      Gateway_Received_Charac->setValue(temp2);
      Gateway_Received_Charac->notify();
      /*while(Serial.available()<0);
      pacote_comando[2] = (byte)(int)(Serial.read());*/
    }
    if(temp == '2'){
      Serial.println("START");
      char temp2[2] = "Y";
      temp2[2] = '\0';
      //Gateway_Received_Charac->setValue((char)byteSTART);
      Gateway_Received_Charac->setValue(temp2);
      Gateway_Received_Charac->notify();
      /*delay(100);
      while(Serial.available()<0);
      pacote_comando[2] = (byte)(int)(Serial.read());*/
    }
    if(temp == '3'){
      Serial.println("TEMPO ENTRE AMOSTRAGENS NOS SENSORES");
     /* delay(100);
      while(Serial.available()<0);
      pacote_comando[2] = (byte)(int)(Serial.read());*/
    }
    }
}

void make_timestamp() { //Faz um timestamp através do servidor NTP, 
  struct tm timestamp; //tm é uma struct que guarda os tempos em ints consoante o tipo (anos, meses, dias, etc)
  while(!getLocalTime(&timestamp)){
    //Serial.println("Failed to obtain time");
    return;
  }
  Serial.print(&timestamp, "%H:%M:%S");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Server!");
  WiFi.begin(ssid, password);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  BLEDevice::init("Gateway_Sensor");
  Gateway_Server_BLE = BLEDevice::createServer();
  Gateway_Service = Gateway_Server_BLE->createService(UUID_SERVER_SERVICE);
  
  Gateway_Received_Charac = Gateway_Service->createCharacteristic(UUID_Characteristic_Server,
                            BLECharacteristic::PROPERTY_READ |
                            BLECharacteristic::PROPERTY_WRITE |
                            BLECharacteristic::PROPERTY_NOTIFY );
  Gateway_Characteristic_TEMP = Gateway_Service -> createCharacteristic(UUID_TEMP_CHARACTERISTIC,
                                BLECharacteristic::PROPERTY_READ |
                                BLECharacteristic::PROPERTY_WRITE);
  Gateway_Characteristic_HUMD = Gateway_Service -> createCharacteristic(UUID_HUMD_CHARACTERISTIC,
                                BLECharacteristic::PROPERTY_READ |
                                BLECharacteristic::PROPERTY_WRITE);
  Gateway_Characteristic_PRESS = Gateway_Service -> createCharacteristic(UUID_PRESS_CHARACTERISTIC,
                                BLECharacteristic::PROPERTY_READ |
                                BLECharacteristic::PROPERTY_WRITE);
                                
  Gateway_Received_Charac->setCallbacks(new MyCallbacks());
  Gateway_Characteristic_TEMP -> setCallbacks(new MyCallbacks());
  Gateway_Characteristic_HUMD -> setCallbacks(new MyCallbacks());
  Gateway_Characteristic_PRESS -> setCallbacks(new MyCallbacks());
  Gateway_Received_Charac -> setValue("GateWay_Sensor_UC_PIT_G1");
  Gateway_Characteristic_TEMP -> setValue(".");
  Gateway_Characteristic_HUMD -> setValue(".");
  Gateway_Characteristic_PRESS -> setValue(".");
  Gateway_Service->start();
  BLEAdvertising *Ad_Servidor = BLEDevice::getAdvertising();
  Ad_Servidor->addServiceUUID(UUID_SERVER_SERVICE);
  Ad_Servidor->setScanResponse(true);
  Ad_Servidor->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Starting to Search Clients Now");

  WiFi.mode(WIFI_STA);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Attempting connecting to ThingSpeak");
    WiFi.begin(ssid, password);
    delay(4000);
  }
  if (WiFi.status() == WL_CONNECTED) {
    WiFi_Connected = true;
  }
  ThingSpeak.begin(cliente_internet);
  Serial.println("Conected to ThingSpeak");

}

void loop() {
  String toSend;
  int i;
  check_input();
  if(Gateway_Server_BLE->getConnectedCount() <= 0) {
    Serial.println("SEM SENSORES LIGADOS");
    sensor_temp_enviar = false;
    sensor_humd_enviar = false;
    BLEAdvertising *Ad_Servidor = BLEDevice::getAdvertising();
    Ad_Servidor->addServiceUUID(UUID_SERVER_SERVICE);
    Ad_Servidor->setScanResponse(true);
    Ad_Servidor->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    delay(5000);
  }
  else {
    while (sensor_temp_enviar) {
      test_speak = ThingSpeak.writeField(myChannelnum, 1, value_temp, MyApiKey);
      if (test_speak == 200){
        make_timestamp();
        timer_delay = millis();
        Serial.print(" -> Temperatura Enviada: ");
        Serial.println(value_temp);
        sensor_temp_enviar = false;
      }
    }
    while(millis()-timer_delay < 20000){
      check_input();
    }
    while (sensor_humd_enviar) {
      test_speak = ThingSpeak.writeField(myChannelnum, 2, value_humd, MyApiKey);
      if(test_speak == 200){
        timer_delay = millis();
        make_timestamp();
        Serial.print(" -> Humidade Enviada: ");
        Serial.println(value_humd);
        sensor_humd_enviar = false;
      }
    }
    while(millis()-timer_delay < 20000){
      check_input();
    }
    while (sensor_press_enviar){
      test_speak = ThingSpeak.writeField(myChannelnum, 3, value_pressure, MyApiKey);
      if (test_speak == 200) {
        timer_delay = millis();
        make_timestamp();
        Serial.print(" -> Pressão atmosférica enviada: ");
        Serial.println(value_pressure);
        sensor_press_enviar = false;
      }
    }
    while(millis()-timer_delay < 20000){
      check_input();
    }
  }
  
  /*if(caracteristica_recebida.length() > 0){ // verifica que a string recebida a partir da central de sensor é superior a 0, logo recebeu algo
    Serial.println("A Receber Temperatura e Humidade");
    for(i = 0; i<caracteristica_recebida.length(); i++){
    Serial.print(caracteristica_recebida[i]);
    }*/

  /*else if(caracteristica_recebida == UUID_HUMD_CHARACTERISTIC){
    Serial.println("A Receber Humidade");
    }*/
  /* if(WiFi_Connected){
     ThingSpeak.writeField(myChannelnum  m);
    }*/
  // put your main code here, to run repeatedly:

}
