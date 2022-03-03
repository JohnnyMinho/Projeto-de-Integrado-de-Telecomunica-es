#include "RF24.h"
#include "nRF24L01.h"
#include <SPI.h>;
#include <printf.h>;
#include "CRCx.h";
#include "DHT.h";
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <stdio.h>
#include <BLEServer.h>

#define Sensor_Name "DHT11-TEMP&HUMD"
#define DHTPIN 17 //Define o pino a que o DHT vai transmitir os dados na placa ESP32, neste caso é o RX logo é o pin 40
#define TIPODHT DHT11 //Define o tipo de DHT usado neste caso o DHT11

boolean doConnect = false;
boolean IMConnected = false;
boolean Scan_BLE = false;
boolean temperatura_nova = false;
boolean humidade_nova = false;
static BLEUUID UUID_SERVER_SERVICE("72add083-b931-4efc-ba71-4eaf935e0465");
static BLEUUID UUID_Characteristic_Server("cf15383a-afe7-4d44-b24d-a6363e93793e"); 
static BLEUUID UUID_TEMP_CHARACTERISTIC("eea1aa7d-b9b9-4ddf-a575-05b7a37b139c");
static BLEUUID UUID_HUMD_CHARACTERISTIC("9d9031b5-191f-4e2d-9791-1c2240e74a8d");
static BLERemoteCharacteristic* Remote_Humd_Characteristic;
static BLERemoteCharacteristic* Remote_Temp_Characteristic;
//static BLERemoteCharacteristic* Remote_Press_Characteristic;
static BLEAdvertisedDevice* Gateway_Usado; //Servidor Basicamente
char* Temp_Data;
char* Humd_Data;

DHT dht(DHTPIN, TIPODHT);

long int BLE_interval_time = 1000; //1000 micros de espera entre janelas de scan 
long int BLE_Window_Time = 1000; //1000 micros de tempo de scan por janela
float temp = 0; //guardar temperatura
float humd = 0; //guardar humidade

class MyClientCallback : public BLEClientCallbacks{
  void onConnect(BLEClient* DHT11_SENSOR_temp){
    Serial.println("SENSOR CONECTADO AO GATEWAY , A INICIAR CONTROLOS");
  }
  void onDisconnect(BLEClient* DHT11_SENDOR_temp){
    Serial.println("SENSOR DISCONECTADO");
    IMConnected = false;
  }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
 /* Called for each advertising BLE server. */
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    /* We have found a device, let us now see if it contains the service we are looking for. */
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(UUID_SERVER_SERVICE))
    {
      BLEDevice::getScan()->stop();
      Gateway_Usado = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      Scan_BLE = true;

    }
  }
};

static void notifyCallBack_TEMP(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){
  
}

static void notifyCallBack_HUMD(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){
  
}

bool conectar_servidor(){ //Criamos um cliente e conectamos ao servidor
 
  BLEClient* DHT11_SENSOR = BLEDevice::createClient(); //Criamos um cliente com o nome DHT11_SENSOR

  DHT11_SENSOR->setClientCallbacks(new MyClientCallback());

  DHT11_SENSOR->connect(Gateway_Usado); //Conecta o sensor ao gateway detectado 
  
  BLERemoteService* Temp_Servico = DHT11_SENSOR->getService(UUID_SERVER_SERVICE);
  //Basicamente se o serviço apontar para um null, ou seja não for detectado, deitamos as informações obtidas fora e fazemos um disconect do sensor 
  if(Temp_Servico == nullptr) 
  {
    DHT11_SENSOR->disconnect();
    Serial.println("Service Not Found");
    return false;
  }
  
  Remote_Humd_Characteristic = Temp_Servico -> getCharacteristic(UUID_HUMD_CHARACTERISTIC); //Obtemos as caracteristicas do Gateway e seguidamente verificamos se as mesmas são válidas 
  Remote_Temp_Characteristic = Temp_Servico -> getCharacteristic(UUID_TEMP_CHARACTERISTIC);
  
  if(Remote_Humd_Characteristic == nullptr || Remote_Temp_Characteristic == nullptr){ //Só verificamos os casos em que falhou já que se tudo correr bem, não é necessário uma alteração do progresso
    DHT11_SENSOR->disconnect();
    Serial.println("Characteristic Error");
    return false;
  }
  
  if(Remote_Humd_Characteristic->canRead()){
    std::string mostrar = Remote_Humd_Characteristic->readValue();
    Serial.println(mostrar.c_str());
  }
  if(Remote_Temp_Characteristic->canRead()){
    std::string mostrar = Remote_Temp_Characteristic->readValue();
    Serial.println(mostrar.c_str());
  }

  if(Remote_Humd_Characteristic->canNotify() && Remote_Temp_Characteristic->canNotify()){
    Remote_Humd_Characteristic->registerForNotify(notifyCallBack_HUMD);
    Remote_Temp_Characteristic->registerForNotify(notifyCallBack_TEMP);
  }

  IMConnected = true; //Como todos os passos foram validados, podemos considerar que o Sensor está conectado ao GATEWAY logo o booleano que assim o define é dado o valor de true
  return true;
  
}

void setup() {
  Serial.begin(115200);
  BLEDevice::init(Sensor_Name);
  BLEScan* Scanner_BLE = BLEDevice::getScan();
  Scanner_BLE -> setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  Scanner_BLE -> setInterval(BLE_interval_time);
  Scanner_BLE -> setWindow(BLE_Window_Time);
  Scanner_BLE -> setActiveScan(true);
  Scanner_BLE -> start(30);
  dht.begin();
}

void loop(){
  //String toSend_Humd;
  //String toSend_Temp;
 if(doConnect){
  if(conectar_servidor()){
    Serial.println("Estou conectado ao gateway");
  }
  else{
    Serial.println("Não estou conectado. Morri.");
  }
  doConnect = false;
 }
 if(IMConnected){
    humd = dht.readHumidity();
    temp = dht.readTemperature();
    if(isnan(temp) || isnan(humd)){
      Serial.println("Failed to read temperature or humidity");
    }
    else{

      String toSend_Temp = ("Temp: " + String(temp));
      
      String toSend_Humd = ("Humd: " + String(humd));
    Remote_Temp_Characteristic->writeValue(toSend_Temp.c_str(), toSend_Temp.length());
    Remote_Humd_Characteristic->writeValue(toSend_Humd.c_str(), toSend_Humd.length());
    }
    
  } 
  else if(Scan_BLE){
    BLEDevice::getScan() -> start(0); 
  }
  
  /*humd = dht.readHumidity();
  temp = dht.readTemperature();
  if(isnan(temp) || isnan(humd)){
    Serial.println("Failed to read temperature or humidity");
  }
  else{
    Serial.print("Nivel de Humidade: ");
    Serial.print(humd);
    Serial.println("%");
    Serial.print("Temperatura: ");
    Serial.print(temp);
    Serial.println(" ºC");
  }*/
  delay(1000);
  
}
