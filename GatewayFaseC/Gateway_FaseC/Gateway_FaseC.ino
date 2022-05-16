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
boolean authenticated = false;
boolean BeginByte_R = false;
boolean ENDByte_R = false;
unsigned long lasttime_connected = 0;

unsigned long current_epoch;

char value_pacote[18];
byte timestamp_HMS[9];
byte timestamp_HMS_data[20];
uint8_t pacote_dados[38];
uint8_t pacote_auth[27];
uint8_t pacote_erro[12];
byte pacote_comando[2];
int test_speak = 0;

WiFiClient client;

//-------------------------------
void make_timestamp();
void SendError(int tipo, byte Level);
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("Device Connected");
      deviceConnected = true;
    }
    void onDisconnect(BLEServer* pServer) {
      time_configure_sent = false;
      Serial.println("Device Disconnected");
      byte temp = (int) 1;
      SendError(1,temp);
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
      if ((pCharacteristic->getUUID().equals(UUID_TEMP_CHARACTERISTIC)) && authenticated) {
        caracteristica_temp = true;
        value = pCharacteristic->getValue();
        //Serial.println("HELLO");
      }
      if (value.length() > 0)
      {
        memcpy(pacote_dados, &DATAbyte, sizeof(DATAbyte));
        for (int i = 0; i < value.length(); i++)
        {
          if (caracteristica_temp) {
            pacote_dados[i + 20] = value[i];
            if (value[i] == NULL) {
              pacote_dados[i + 20] = (char)0;
            }
            //Serial.print(value[i]);
            sensor_temp_enviar = true;
          }
        }
        pacote_dados[38] = '\0';
        if ((sensor_temp_enviar)) {
          Serial.println();
          //temp_send = atof(value_temp);
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
//EFETUA A CONEXÃO DA SOCKET AO SERVIDOR / GESTOR DE SERVIÇOS
boolean connectTCP() {
  boolean temp = false;
  if (!client.connect("192.168.43.85", 4444)) { // É NECESSÁRIO VERIFICAR SE O PORT E O IP CORRESPONDEM AO DA MÁQUINA QUE ESTÁ A CORRER O GESTOR DE SERVIÇOS
    Serial.println("connection failed");
  } else {
    temp = true;
  }
  return temp;
}

void send_time_sensor() { //PARA ENVIAR O TEMPO PARA FAZER SYNC DO SISTEMA SENSOR, APENAS ENVIAMOS O TEMPO NO FORMATO HH/MM/SS 
  time_t now;
  struct tm timestamp2;
  while (!getLocalTime(&timestamp2)) {
    Serial.println("Failed to obtain time");
  }
  time(&now);
  byte time_sync_send[12];
  char time_sync[12];
  sprintf(time_sync , "%u", now);
  Serial.println(time_sync);
  time_sync[12] = '\0';
  for (int i = 0; i < 11 ; i++) {
    time_sync_send[i + 1] = time_sync[i];
  }
  String time_test = time_sync;
  //Serial.println(time_test);
  time_sync_send[0] = 'T';
  time_sync_send[12] = '\0';
  Gateway_Received_Charac->setValue(time_sync_send, 12);
  Gateway_Received_Charac->notify();
}

void sendAuthentication() {//PARA UMA QUESTÃO DE EFICIÊNCIA O LOCAL É ENVIADO NA AUTENTICAÇÃO DE MODO A FICAR PERMANENTEMENTE ASSOCIADO A UM GATEWAY
  char *latitude = "41.3083"; //A latitude e longitude vão ter 6 digitos, 2 unidades e 4 decimais o que confere uma precisão de cerca de +/- 11.1 m, foi também assumido que este sistema irá apenas ser usado em Portugal Continental
  char *longitude = "-8.0800"; //As posições são manuais para não se ter de pagar por uma API da google ou um módulo GPS de quase 15 €
  char *password = "TRY1";
  pacote_auth[0] = Authenticationbyte;
  memmove(pacote_auth + 1, password, 4);
  memmove(pacote_auth + 5, client_name , 7);
  memmove(pacote_auth + 12, latitude, 7);
  memmove(pacote_auth + 19, longitude, 7);
  pacote_auth[27] = '\0';
  client.write((int)26);
  delay(50);
  client.write(pacote_auth, 26);
}

void SendError(int level, byte tipo) {
  time_t now;
  char temp_tempo[9];
  struct tm timestamp; //tm é uma struct que guarda os tempos em ints consoante o tipo (anos, meses, dias, etc)
  while (!getLocalTime(&timestamp)) {
    Serial.println("Falha na obtenção do tempo");
  }
  strftime(temp_tempo, sizeof(temp_tempo), "%H:%M:%S", &timestamp);
  for (int i = 0; i < 9; i++) {
    timestamp_HMS[i] = (byte)temp_tempo[i];
  }
  pacote_erro[0] = ERRORbyte;
  if (level == 1) {
    memmove(pacote_erro + 1, timestamp_HMS, 8);
    pacote_erro[9] = tipo;
    pacote_erro[10] = 'Y';
    client.write((int)12);
    delay(50);
    client.write(pacote_erro, 12);
  } else if (level == 0) {
    memmove(pacote_erro + 1, timestamp_HMS, 8);
    pacote_erro[10] = tipo;
    pacote_erro[11] = 'N';
    client.write((int)12);
    delay(50);
    client.write(pacote_erro, 12);
  } else {
    Serial.println("Nível de erro inválido");
  }
}

void SendPacket() {
  time_t now;
  char temp_tempo[20];
  struct tm timestamp; //tm é uma struct que guarda os tempos em ints consoante o tipo (anos, meses, dias, etc)
  while (!getLocalTime(&timestamp)) {
    Serial.println("Failed to obtain time");
  }
  strftime(temp_tempo, sizeof(temp_tempo), "%d/%m/%Y %H:%M:%S", &timestamp);
  for (int i = 0; i < 20; i++) {
    timestamp_HMS_data[i] = (byte)temp_tempo[i];
    //Serial.print(temp_tempo[i]);
  }
  memmove(pacote_dados + 1, timestamp_HMS_data, 19);
  time(&now);
  //Serial.print(&timestamp, "%H:%M:%S");
  pacote_dados[0] = DATAbyte;
  //int array_length = pacote_dados.length;
  //client.write(27);
  client.write((int)37);
  delay(50);
  client.write(pacote_dados, 37);
}

void make_timestamp() { //Faz um timestamp através do servidor NTP,
  time_t now;
  char temp_tempo[9];
  struct tm timestamp; //tm é uma struct que guarda os tempos em ints consoante o tipo (anos, meses, dias, etc)
  while (!getLocalTime(&timestamp)) {
    Serial.println("Failed to obtain time");
  }
  strftime(temp_tempo, sizeof(temp_tempo), "%H:%M:%S", &timestamp);
  for (int i = 0; i < 9; i++) {
    timestamp_HMS[i] = (byte)temp_tempo[i];
  }
  memmove(pacote_dados + 1, timestamp_HMS, 8);
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

  //WiFi.mode(WIFI_STA);

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
  TCP_Stay_Connected = connectTCP();
  while (!TCP_Stay_Connected) {
    TCP_Stay_Connected = connectTCP();
    delay(5000);
  }
}

void loop() {
  String toSend;
  int i;
  if (!TCP_Stay_Connected) {
    if(ENDByte_R){
      Serial.println("Conexão encerrada pelo Gestor de Serviços");
    }
    TCP_Stay_Connected = connectTCP();
    delay(2500);
  }
  if (TCP_Stay_Connected && !authenticated) {
    sendAuthentication();
    while (client.available() == 0) {
      Serial.println("Awaiting");
    }
    while (client.available()) { //Apesar de haver uma conexão o gateway fica à espera de uma mensagem de begin
      byte begin_byte = client.read(); //O que quer dizer que ele foi autenticado e pode começar a enviar dados
      if (begin_byte == BEGINbyte) { //Caso o mesmo não aconteça o Gestor invalida a conexão
        BeginByte_R = true;
        authenticated = true;
        client.write((int)1);
        delay(50);
        client.write(STARTbyte);
        lasttime_connected = millis(); //Tiramos esta medida para dar uma medida o mais próxima possível de 15 segundos
      }
    }
  }
  while (TCP_Stay_Connected && authenticated) {
    check_input();
    if (!client.connected()){ //Caso seja verificado que o Sistema Central perdeu a sua conexão com o gateway, é necessário tentar realizar uma reconexão.
        Serial.println("CLIENTE FOI DISCONECTADO");
        TCP_Stay_Connected = false;
        authenticated = false;
      }
    if (client.available() > 0) { //Quando o gateway recebeu uma autenticação e um byte de begin ele pode receber um byte END pelo que fica à espera de uma possível mensagem
        int i = 0;
        char tamanho_mensagem = client.read();
        int tamanho = (int)tamanho_mensagem;
        byte byte_recebido[tamanho];
        while (client.available() > 0) {
          byte_recebido[i] = client.read();
          
         /*if (byte_recebido[i] == BEGINbyte) {
            Serial.println("Begin byte recebido");
            BeginByte_R = true;
          }*/
          if (byte_recebido[i] == ENDbyte) {//Se O gateway receber um byte de END por parte do Gestor, ele invalida a sua conexão e termina-a
            BeginByte_R = false;
            TCP_Stay_Connected = false;
            ENDByte_R = true;
            Serial.println("End byte recebido");
          }
          i++;
        }
      }
    if (Gateway_Server_BLE->getConnectedCount() <= 0) { //Se o gateway não tiver nenhum sistema sensor ligado a ele por mais do que 15 segundos
      time_configure_sent = false; // Ele mesmo envia uma mensagem de erro de nível alto de modo a terminar a conexão
      Serial.println("SEM SENSORES LIGADOS");
      sensor_temp_enviar = false;
      BLEAdvertising *Ad_Servidor = BLEDevice::getAdvertising();
      Ad_Servidor->addServiceUUID(UUID_SERVER_SERVICE);
      Ad_Servidor->setScanResponse(true);
      Ad_Servidor->setMinPreferred(0x12);
      BLEDevice::startAdvertising();
      delay(500);
      if((millis()-lasttime_connected)>27500){
        char error_temp = '1';
        SendError(1,(byte)error_temp);
      }
    }
    else{
      if (!time_configure_sent) { //Caso haja um sistema sensor conectado enviamos o tempo com o valor de epoch()
        delay(1000);
        send_time_sensor(); //Deste modo o sistema sensor é capaz de sincronizar o seu tempo com o gateway
        time_configure_sent = true;
      }
      while (sensor_temp_enviar && BeginByte_R) { //Se houver um sistema sensor ligado ao gateway e o sistema central tiver autorizado o envio de dados, o gateway começa a enviar dados
        make_timestamp();
        SendPacket();
        Serial.print("Dados Enviados");
        sensor_temp_enviar = false;
      }
      lasttime_connected = millis(); //Enquanto houver um sistema sensor ligado ao gateway este continua atualizar este valor
    }
  }
}
