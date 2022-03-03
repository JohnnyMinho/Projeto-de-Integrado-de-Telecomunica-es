#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "ThingSpeak.h"
#include <WiFi.h>

#define UUID_SERVER_SERVICE "72add083-b931-4efc-ba71-4eaf935e0465"
#define UUID_Characteristic_Server "cf15383a-afe7-4d44-b24d-a6363e93793e"
#define UUID_TEMP_CHARACTERISTIC "eea1aa7d-b9b9-4ddf-a575-05b7a37b139c"
#define UUID_HUMD_CHARACTERISTIC "9d9031b5-191f-4e2d-9791-1c2240e74a8d"
#define UUID_PRESS_CHARACTERISTIC "01bcb788-7258-43b3-be93-f35dad0ac2f4"
#define Server_Name "GATEWAY_SENSORES"

const char* ssid = "Mi 10T Pro";
const char* password = "gusmaogusmao";

WiFiClient cliente_internet;

unsigned long myChannelnum = X;
const char MyApiKey = " "; // Substituir por chave do canal do thingspeak

unsigned long lastTime = 0;
unsigned long Delay_time = 1000;

float temp;
float humd;


void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Server!");

  BLEDevice::init("Rugby_Pescoço"); // BLEDevice::init("GATEWAY_SENSORES");
  BLEServer *Servidor_BLE = BLEDevice::createServer();
  BLEService *Servico1_Server = Servidor_BLE->createService(UUID_SERVER_SERVICE);
  BLECharacteristic *Characteristic_Server = Servico1_Server->createCharacteristic(UUID_Characteristic_Server,
                                                                                   BLECharacteristic::PROPERTY_READ | 
                                                                                   BLECharacteristic::PROPERTY_WRITE);
  Characteristic_Server -> setValue("BANDOLHERO BANDOLHERA SE QUI SE NO PUEDE VIVER ASI.");
  Servico1_Server->start();
  BLEAdvertising *Ad_Servidor = BLEDevice::getAdvertising();
  Ad_Servidor->addServiceUUID(UUID_SERVER_SERVICE);
  Ad_Servidor->setScanResponse(true);
  Ad_Servidor->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Starting to Search Clients Now");

  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(cliente_internet);
  
}

void loop() {
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Attempting connecting to ThingSpeak);
    WiFi.begin(ssid, password);
    delay(2500);
  }
  Serial.println("Conected to ThingSpeak");
  delay(1000);
  // put your main code here, to run repeatedly:

}
