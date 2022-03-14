#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "ThingSpeak.h"
#include <stdlib.h>
#include <WiFi.h>
#include <stdio.h>
#include "INTINFO.h"

#define Server_Name "GATEWAY_SENSORES"
#define ThingSpeak_Timer
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

//------------------------

// Variáveis ligadas à WiFi


char* ssid = ssidhotspot;
char* password = passwordhotspot;


WiFiClient cliente_internet;

unsigned long myChannelnum = 1666652;
const char *MyApiKey = "DZCFYQTOXYIUTK2J"; // Substituir por chave do canal do thingspeak

unsigned long lastTime = 0;
unsigned long Delay_time = 1000;

// -------------------------------

//Variáveis gerais
byte HeaderTEMP = 0b00010000;
byte HeaderHUMD = 0b00010010;
byte HeaderPRES = 0b00010100;
boolean deviceConnected = false;
boolean sensor_temp_enviar = false;
boolean sensor_humd_enviar = false;
boolean caracteristica_temp = false;
boolean caracteristica_humd = false;
boolean WiFi_Connected = false;
char value_temp[6];
char value_humd[6];
char value_aux[6]; //ajuda na representação dos dados
float temp;
float humd;
float humd_send;
float temp_send;
int temp_test;
int humd_test;
int test_speak = 0;

//-------------------------------

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

      if (value.length() > 0)
      {
        for (int i = 0; i < value.length(); i++)
        {
          // Serial.print((char)value[i]);
          if (caracteristica_temp) {
            value_temp[i] = value[i];
            Serial.print("temp" );
            Serial.print((char)value_temp[i]);
            sensor_temp_enviar = true;
          }
          if (caracteristica_humd) {
            value_humd[i] = value[i];
            Serial.print("humd" );
            Serial.print((char)value_humd[i]);
            sensor_humd_enviar = true;
          }

        }
        value_temp[5] = '\0';
        value_humd[5] = '\0';
        Serial.println(" ");
        if ((sensor_temp_enviar)) {
          temp_send = atof(value_temp);
        }
        if ((sensor_humd_enviar)) {
          humd_send = atof(value_humd);
        }
        caracteristica_temp = false;
        caracteristica_humd = false;
        Serial.println(temp_send);
        Serial.println(humd_send);
      }
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Server!");
  WiFi.begin(ssid, password);
  BLEDevice::init("Gateway_Sensor"); // BLEDevice::init("GATEWAY_SENSORES");
  Gateway_Server_BLE = BLEDevice::createServer();
  Gateway_Service = Gateway_Server_BLE->createService(UUID_SERVER_SERVICE);
  Gateway_Received_Charac = Gateway_Service->createCharacteristic(UUID_Characteristic_Server,
                            BLECharacteristic::PROPERTY_READ |
                            BLECharacteristic::PROPERTY_WRITE);
  Gateway_Characteristic_TEMP = Gateway_Service -> createCharacteristic(UUID_TEMP_CHARACTERISTIC,
                                BLECharacteristic::PROPERTY_READ |
                                BLECharacteristic::PROPERTY_WRITE);
  Gateway_Characteristic_HUMD = Gateway_Service -> createCharacteristic(UUID_HUMD_CHARACTERISTIC,
                                BLECharacteristic::PROPERTY_READ |
                                BLECharacteristic::PROPERTY_WRITE);
  Gateway_Received_Charac->setCallbacks(new MyCallbacks());
  Gateway_Characteristic_TEMP -> setCallbacks(new MyCallbacks());
  Gateway_Characteristic_HUMD -> setCallbacks(new MyCallbacks());
  Gateway_Received_Charac -> setValue("GateWay_Sensor_UC_PIT_G1");
  Gateway_Characteristic_TEMP -> setValue(".");
  Gateway_Characteristic_HUMD -> setValue(".");
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
  //String caracteristica_recebida;

  //std::string temp = Gateway_Characteristic_TEMP->getValue();
  //std::string temp = Gateway_Characteristic_HUMD->getValue();*/
  //Gateway_Received_Charac->getValue();
  //Gateway_Characteristic_TEMP->getValue();
  // Gateway_Characteristic_HUMD->getValue();
  if (Gateway_Server_BLE->getConnectedCount() <= 0) {
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
      Serial.print("temp tent");
      test_speak = ThingSpeak.writeField(myChannelnum, 1, value_temp, MyApiKey);
      Serial.println(test_speak);
      if (test_speak == 200) {
        Serial.print("sucess temp");
        sensor_temp_enviar = false;
      }

    }
    delay(MAX_TIME_SEND);
    while (sensor_humd_enviar) {
      Serial.print("humd tent");
      test_speak = ThingSpeak.writeField(myChannelnum, 2, value_humd, MyApiKey);
      Serial.println(test_speak);
      if (test_speak == 200) {
        Serial.print("humd sucess");
        sensor_humd_enviar = false;
      }
    }
    delay(MAX_TIME_SEND);
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
