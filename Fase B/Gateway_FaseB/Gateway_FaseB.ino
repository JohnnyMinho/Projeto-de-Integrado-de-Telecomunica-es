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
const int daylightOffset_sec = 3600; //Tem a haver com o horário de inverno / verão

unsigned long timer_delay = 0;
// -------------------------------

//Variáveis gerais
String MSG_STOP = "STOP"; //Como o BLE facilita o envio de Strings , usamos uma String como meio de fazer STOP ou START dos sensores
String MSG_START = "START";

boolean deviceConnected = false;
boolean sensor_temp_enviar = false;
boolean sensor_humd_enviar = false;
boolean sensor_press_enviar = false;
boolean caracteristica_pressure = false;
boolean caracteristica_temp = false;
boolean caracteristica_humd = false;
boolean WiFi_Connected = false;
boolean TCP_Stay_Connected = false;
boolean time_configure_sent = false;
unsigned long current_epoch;
/*char value_temp[6];
  char value_humd[6];
  char value_pressure[6];*/
char value_pacote[18];
byte current_timestamp_HMS[9];
uint8_t pacote_dados[27];
uint8_t pacote_auth[16];
byte pacote_comando[2];
/*float temp;
  float humd;
  float pressure;
  float humd_send;
  float temp_send;
  float pressure_send;*/
int test_speak = 0;
WiFiClient client;

//-------------------------------
void make_timestamp();
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("Device Connected");
      deviceConnected = true;
    }
    void onDisconnect(BLEServer* pServer) {
      time_configure_sent = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks
{
    void onConnect(BLEServer* pServer) {
      Serial.println("Device Connected");
      deviceConnected = true;
    }
    void onDisconnect(BLEServer* pServer) {

    }
    void onWrite(BLECharacteristic *pCharacteristic)
    {
      std::string value;
      if ((pCharacteristic->getUUID().equals(UUID_TEMP_CHARACTERISTIC))) {
        caracteristica_temp = true;
        value = pCharacteristic->getValue();
        Serial.println("HELLO");
      }
      if (value.length() > 0)
      {
        memcpy(pacote_dados, &DATAbyte,sizeof(DATAbyte));
        for (int i = 0; i < value.length(); i++)
        {
          if (caracteristica_temp) {
            pacote_dados[i + 9] = value[i];
            if (value[i] == NULL) {
              pacote_dados[i + 9] = 30;
            }
            Serial.println(value[i]);
            sensor_temp_enviar = true;
          }
        }
        pacote_dados[27] = '\0';
        if ((sensor_temp_enviar)) {
          //temp_send = atof(value_temp);
          make_timestamp();

          Serial.print(" -> Dados Recebidos por BLE");
        }
        caracteristica_temp = false;
      }
    }
};

void check_input() {
  if (Serial.available() > 0) {
    char temp = Serial.read();
    if (temp == '1') {
      Serial.println("STOP");
      char temp2[2] = "N";
      temp2[2] = '\0';
      Gateway_Received_Charac->setValue(temp2);
      Gateway_Received_Charac->notify();

    }
    if (temp == '2') {
      Serial.println("START");
      char temp2[2] = "Y";
      temp2[2] = '\0';
      //Gateway_Received_Charac->setValue((char)byteSTART);
      Gateway_Received_Charac->setValue(temp2);
      Gateway_Received_Charac->notify();
    }
  }
}

boolean connectTCP() {
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
  } else {
    TCP_Stay_Connected = true;
  }
}

void send_time_sensor() {
  struct tm timestamp;
  while (!getLocalTime(&timestamp)) {
    Serial.println("Failed to obtain time");
  }
  byte time_sync_send[8];
  char time_sync[8] = {'T'};
  sprintf(time_sync + 1, "%d", timestamp.tm_hour);
  sprintf(time_sync + 3, "%d", timestamp.tm_min);
  sprintf(time_sync + 5, "%d", timestamp.tm_sec);
  Serial.println(timestamp.tm_min);
  time_sync[8] = '\0';
  for (int i = 0; i < sizeof(time_sync) ; i++) {
    time_sync_send[i] = time_sync[i];
    Serial.print(time_sync[i]);
  }
  String time_test = time_sync;
  Serial.println(time_test);
  time_sync_send[8] = '\0';
  Gateway_Received_Charac->setValue(time_sync_send, 8);
  Gateway_Received_Charac->notify();
}

void make_timestamp() { //Faz um timestamp através do servidor NTP,
  time_t now;
  struct tm timestamp; //tm é uma struct que guarda os tempos em ints consoante o tipo (anos, meses, dias, etc)
  while (!getLocalTime(&timestamp)) {
    Serial.println("Failed to obtain time");
  }
  strftime(time_stamp_HMS, sizeof(time_stamp_HMS), "%H:%M:%S", &timestamp);
  memmove(pacote_dados + 1, time_stamp_HMS, 8);
  time(&now);
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
  // Serial.println("Starting to Search Clients Now");

  WiFi.mode(WIFI_STA);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connnect to network");
    WiFi.begin(ssid, password);
    delay(5000);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    WiFi_Connected = true;
  }
  boolean TCPConnection = connectTCP();
    if (!TCPConnection) {
    return;
    }
}

void loop() {
  String toSend;
  int i;
  if(TCP_Stay_Connected){
    
  }
  while (TCP_Stay_Connected) {
    check_input();
    
    if (Gateway_Server_BLE->getConnectedCount() <= 0) {
      Serial.println("SEM SENSORES LIGADOS");
      sensor_temp_enviar = false;
      BLEAdvertising *Ad_Servidor = BLEDevice::getAdvertising();
      Ad_Servidor->addServiceUUID(UUID_SERVER_SERVICE);
      Ad_Servidor->setScanResponse(true);
      Ad_Servidor->setMinPreferred(0x12);
      BLEDevice::startAdvertising();
      delay(1000);
    }
    else {
      if (!time_configure_sent) {
        send_time_sensor();
        time_configure_sent = true;
      }
      while (sensor_temp_enviar) {
        make_timestamp();
        Serial.print("Dados Enviados");
        sensor_temp_enviar = false;
      }
    }
  }
}
