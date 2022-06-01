#include <SPI.h>;
#include <printf.h>;
#include <math.h>;
#include "DHT.h";
#include <chrono>;
#include <ctime>;
#include <Wire.h>;
#include <Adafruit_BMP280.h>;
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <stdio.h>
#include <BLEServer.h>
#include "INTINFO.h"

#define Sensor_Name "Gateway_Sensor"
#define BMP_MOSI (21)//pin do SDA/SDI (Pin D21)
#define BMP_SCK  (22)//pin do SCL/SCK (Pin D22)
#define BMP_MISO (12)
#define BMP_CS   (10)

#define DHTPIN 17 //Define o pino a que o DHT vai transmitir os dados na placa ESP32, neste caso é o RX logo é o pin 40
#define TIPODHT DHT11 //Define o tipo de DHT usado neste caso o DHT11
#define Tempo_Amostra 1000 //Define o tempo máximo entre a recolha de amostras 

boolean doConnect = false;
boolean IMConnected = false;
boolean Scan_BLE = false;
boolean dados_novos = false;
boolean send_new = false; //Para permitir que apenas uma amostra seja enviada, usamos esta variável
boolean can_start = false;
boolean lock_dht = false;
boolean lock_bmp = false;
boolean lock_geral = false;
boolean erro_bmp = false;
boolean erro_dht = false;
boolean valid_status = false; //Esta variável serve para confirmar que todos os dados dos sensores são válidos
boolean WiFi_Connected = false;
boolean time_configured;
unsigned status_bmp;
unsigned status_dht;
unsigned long time_start = 0; //timer para apresentar dados
unsigned long time_start_send = 0; // timer para enviar para o gateway
unsigned long time_error_sent = 0; //timer para enviar o proximo erro (para não dar overwelm ao sistema)
String toSend_Temp;
String toSend_Humd;
String toSend_Press;
static BLEUUID UUID_SERVER_SERVICE("72add083-b931-4efc-ba71-4eaf935e0465");
static BLEUUID UUID_Characteristic_Server("cf15383a-afe7-4d44-b24d-a6363e93793e");
static BLEUUID UUID_TEMP_CHARACTERISTIC("eea1aa7d-b9b9-4ddf-a575-05b7a37b139c");
static BLEUUID UUID_ERROR_CHARACTERISTIC("9d9031b5-191f-4e2d-9791-1c2240e74a8d");
static BLEUUID UUID_START_CHARACTERISTIC ("01bcb788-7258-43b3-be93-f35dad0ac2f4");
static BLERemoteCharacteristic* Remote_ERROR_Characteristic;
static BLERemoteCharacteristic* Remote_Temp_Characteristic;
static BLERemoteCharacteristic* Remote_Temp_SERVER;
static BLERemoteCharacteristic* Remote_START_Characteristic;
static BLEAdvertisedDevice* Gateway_Usado; //Servidor Basicamente
char pacote_temp[6];
char pacote_humd[6];
char pacote_press[6];
char pacote_completo[18];
char pacote_sensores[2]; //Vai dizer quais os sensores que estão a funcionar na primeira vez em que este se conecta ao gateway (por uma questão de melhor funcionamento este é enviado aquando da primeira tentativa de leitura de dados)
boolean start_enviado = false;
char* Temp_Data;
char* Humd_Data;
char* Pressure_Data;

/* Variáveis WIFI*/
char* ssid = ssidhotspot;
char* password = passwordhotspot;

DHT dht(DHTPIN, TIPODHT);
Adafruit_BMP280 bmp;
//Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO,  BMP_SCK);
Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();

long int BLE_interval_time = 1000; //1000 micros de espera entre janelas de scan
long int BLE_Window_Time = 1000; //1000 micros de tempo de scan por janela
float temp = 0; //guardar temperatura
float humd = 0; //guardar humidade
float pressure = 0; //guardar pressão atmosférica

void initBLE();

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* SENSORES_temp) {
      Serial.println("SENSOR CONECTADO AO GATEWAY , A INICIAR CONTROLOS");
    }
    void onDisconnect(BLEClient* DHT11_SENSOR_temp) {
      Serial.println("SENSOR DISCONECTADO");
      can_start = false;
      IMConnected = false;
      doConnect = true;
      time_configured = false;
    }
    void onWrite(BLECharacteristic *pCharacteristic, BLEDevice *pDevice) {

    }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
    /* Called for each advertising BLE server. */
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertisedDevice.toString().c_str());

      /* Foi encontrado um cliente, mas é necessário verificar se este têm o servico correto, neste caso o UUID_SERVER_SERVICE */
      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(UUID_SERVER_SERVICE))
      {
        BLEDevice::getScan()->stop();
        Gateway_Usado = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        Scan_BLE = true;
        send_new = false;
        time_start = 0;
        time_start_send = 0;
        valid_status = 0;
      }
    }
};

static void notifyCallBack_SERVER(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  char *value = (char*)pData;
  for (int i = 0; i < length; i++) {
    Serial.print(value[i]);
  }
  Serial.println(value[0]);
  if (value[0] == 'N') {
    if (value[1] == '3') {
      lock_geral = true;
    }
    if (value[1] == '1') {
      lock_dht = true;
    }
    if (value[1] == '2') {
      lock_bmp = true;
    }
  }
  else if (value[0] == 'Y') {
    Serial.println("Entrei");
    if (value[1] == '3') {
      lock_geral = false;
      lock_dht = false;
      lock_bmp = false;
    }
    if (value[1] == '1') {
      lock_dht = false;
    }
    if (value[1] == '2') {
      lock_bmp = false;
    }
    if (value[1] == '4') {
      Serial.println("AQUI");
      can_start = true;
    }
  }
  else if (value[0] == 'T') {
    //Serial.println("CONFIGURATION TIME");
    struct tm timevalue;
    struct timeval time_new;
    time_t now_time;
    char int_time[10];
    long true_time;
    for (int i = 0; i < 11; i++) {
      if (value[i + 1] != NULL || value[i + 1] != '\0') {
        int_time[i] = value[i + 1];
      }
    }
    true_time = strtoul(int_time, NULL, 10);
    Serial.println(true_time);
    time_new.tv_sec = true_time;
    settimeofday(&time_new, NULL);
    setenv("TZ", "WET0WEST,M3.5.0/1,M10.5.0", 1);
    tzset();
    time(&now_time);
    localtime_r(&now_time, &timevalue);
    Serial.print(&timevalue, "%H:%M:%S");
    /* time(&now_time);
      localtime_r(&now_time, &timevalue);*/
    time_configured = true;
  }
}


static void notifyCallBack_TEMP(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {

}

static void notifyCallBack_ERROR(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {

}

static void notifyCallBack_START(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {


}


bool conectar_servidor() { //Criamos um cliente e conectamos ao servidor

  BLEClient* SENSORES = BLEDevice::createClient(); //Criamos um cliente com o nome SENSORES

  SENSORES->setClientCallbacks(new MyClientCallback());

  SENSORES->connect(Gateway_Usado); //Conecta o sensor ao gateway detectado

  BLERemoteService* Temp_Servico = SENSORES->getService(UUID_SERVER_SERVICE);
  //Basicamente se o serviço apontar para um null, ou seja não for detectado, deitamos as informações obtidas fora e fazemos um disconect do sensor
  if (Temp_Servico == nullptr)
  {
    SENSORES->disconnect();
    Serial.println("Service Not Found");
    return false;
  }

  Remote_Temp_SERVER = Temp_Servico -> getCharacteristic(UUID_Characteristic_Server); //Obtemos as caracteristicas do Gateway e seguidamente verificamos se as mesmas são válidas

  Remote_ERROR_Characteristic = Temp_Servico -> getCharacteristic(UUID_ERROR_CHARACTERISTIC);

  Remote_Temp_Characteristic = Temp_Servico -> getCharacteristic(UUID_TEMP_CHARACTERISTIC);

  Remote_START_Characteristic = Temp_Servico -> getCharacteristic(UUID_START_CHARACTERISTIC);

  if (Remote_Temp_SERVER == nullptr) { //Só verificamos os casos em que falhou já que se tudo correr bem, não é necessário uma alteração do progresso
    SENSORES->disconnect();
    Serial.println("Characteristic Error");
    return false;
  }
  if (Remote_Temp_Characteristic == nullptr) { //Só verificamos os casos em que falhou já que se tudo correr bem, não é necessário uma alteração do progresso
    SENSORES->disconnect();
    Serial.println("Characteristic Error");
    return false;
  }
  if (Remote_ERROR_Characteristic == nullptr) { //Só verificamos os casos em que falhou já que se tudo correr bem, não é necessário uma alteração do progresso
    SENSORES->disconnect();
    Serial.println("Characteristic Error");
    return false;
  }
  if (Remote_START_Characteristic == nullptr) {
    SENSORES->disconnect();
    Serial.println("Characteristic Error");
    return false;
  }

  if (Remote_Temp_SERVER->canRead(), Remote_ERROR_Characteristic->canRead(), Remote_Temp_Characteristic->canRead(), Remote_START_Characteristic->canRead()) {
    std::string mostrar = Remote_Temp_SERVER->readValue();
    std::string mostrar2 = Remote_Temp_Characteristic->readValue();
    std::string mostrar3 = Remote_ERROR_Characteristic->readValue();
    std::string mostrar4 = Remote_START_Characteristic->readValue();
    Serial.println(mostrar.c_str());
    Serial.println(mostrar2.c_str());
    Serial.println(mostrar3.c_str());
    Serial.println(mostrar4.c_str());
  }

  if (Remote_Temp_SERVER->canNotify()) {
    Remote_Temp_SERVER->registerForNotify(notifyCallBack_SERVER);
  }
  if (Remote_ERROR_Characteristic->canNotify()) {
    Remote_ERROR_Characteristic->registerForNotify(notifyCallBack_ERROR);
  }
  if (Remote_Temp_Characteristic->canNotify()) {
    Remote_Temp_Characteristic->registerForNotify(notifyCallBack_TEMP);
  }
  if (Remote_START_Characteristic->canNotify()) {
    Remote_START_Characteristic->registerForNotify(notifyCallBack_START);
  }
  IMConnected = true; //Como todos os passos foram validados, podemos considerar que o Sensor está conectado ao GATEWAY logo o booleano que assim o define é dado o valor de true
  return IMConnected;
}

void initBLE() {
  BLEDevice::init(Sensor_Name);
  BLEScan* Scanner_BLE = BLEDevice::getScan();
  Scanner_BLE -> setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  Scanner_BLE -> setInterval(BLE_interval_time);
  Scanner_BLE -> setWindow(BLE_Window_Time);
  Scanner_BLE -> setActiveScan(true);
  Scanner_BLE -> start(0);
  dht.begin();
  status_bmp = bmp.begin(0x76);
  if (status_bmp) {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling, basicamente aumentamos o consumo de energia de modo a reduzir o ruido */
                    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
  }
  else {
    while(!(status_bmp = bmp.begin(0x76))){
      Serial.println("FAILED TO START BMP");
    }
  }
}

void make_timestamp() { //Faz um timestamp através do servidor NTP,
  time_t now;
  struct tm timestamp; //tm é uma struct que guarda os tempos em ints consoante o tipo (anos, meses, dias, etc)
  time(&now);
  localtime_r(&now, &timestamp);
  /*while (!getLocalTime(&timestamp)) {
    //Serial.println("Failed to obtain time");
    //return;
    }*/
  Serial.print(&timestamp, "%H:%M:%S");
}

void setup() {
  Serial.begin(115200);
  initBLE();
}

void loop() {
  //String toSend_Humd;
  //String toSend_Temp;
  if (!lock_geral) {
    if (doConnect) {
      if (conectar_servidor()) {
        Serial.println("Estou conectado ao gateway");
      }
      else {
        Serial.println("Não estou conectado. Morri.");
      }
      doConnect = false;
    }
    if (IMConnected && time_configured) {
      humd = dht.readHumidity();
      temp = dht.readTemperature();
      //Serial.println(humd);
      //Serial.println(temp);
      pressure = (bmp.readPressure() / 100);
      //Serial.println(pressure);
      if (isnan(pressure) || pressure < 0) {
        //Serial.println("Falha no BMP");
        valid_status = false;
        erro_bmp = true;
        //lock_bmp = true;
      } else {
        valid_status = true;
      }
      if (isnan(temp) || isnan(humd)) {
        //Serial.println("Falha no DHT11");
        valid_status = false;
        erro_dht = true;
        //lock_dht = true;
      }
      else {
        valid_status = true;
      }
      if (!start_enviado) {
        if (!lock_dht && !lock_bmp) {
          pacote_sensores[0] = '1';
          pacote_sensores[1] = '2';
        }
        if (lock_dht) {
          pacote_sensores[0] = '*';
        }
        if (lock_bmp) {
          pacote_sensores[1] = '*';
        }
        Remote_START_Characteristic -> writeValue(pacote_sensores, sizeof(pacote_sensores));
        start_enviado = true;
        Serial.println("Informações enviadas");
      }
      time_start = millis();
      if ((millis() - time_error_sent) > 5000) {
        if (erro_dht || erro_bmp) {
          char error = '1';
          //Serial.println("Failed to read temperature or humidity or pressure");
          if (erro_dht) {
            error = '1';
          }
          if (erro_bmp) {
            error = '2';
          }
          if (erro_bmp && erro_dht) {
            error = '3';
          }
          Remote_ERROR_Characteristic -> writeValue(error, sizeof(error));
          time_error_sent = millis();
        }
      }
      //if(can_start){
      valid_status = false;
      if ((millis() - time_start_send) >= 1000) {
        make_timestamp();
        if (!lock_dht) {
          toSend_Temp = (String(temp));
          //Serial.println(toSend_Temp);
          toSend_Humd = (String(humd));
          //Serial.println(toSend_Humd);
          Serial.print(" -> Temperatura: ");
          Serial.print(toSend_Temp);
          Serial.print(" ,Humd: ");
          Serial.print(toSend_Humd);
        }
        if (!lock_bmp) {
          toSend_Press = (String(pressure));
          //Serial.println(toSend_Press);
          Serial.print(" ,Presão atmosférica: ");
          Serial.print(toSend_Press);
        }
        //Serial.println("HELLO3");
        for (int i = 0; i < 7; i++) {
          /*if (i < 5) {
            pacote_completo[i] = toSend_Temp[i];
            pacote_completo[i + 5] = toSend_Humd[i];
            }
            pacote_completo[i + 10] = toSend_Press[i];*/
          if (i < 5 && !erro_dht) {
            pacote_completo[i] = toSend_Temp[i];
            pacote_completo[i + 5] = toSend_Humd[i];
          }
          if (i < 5 && erro_dht) {
            pacote_completo[i] = '*';
            pacote_completo[i + 5] = '*';
          }
          if (!erro_bmp) {
            pacote_completo[i + 10] = toSend_Press[i];
          }
          if (erro_bmp) {
            pacote_completo[i + 10] = '-';
          }
        }
        erro_dht = false;
        erro_bmp = false;
        pacote_completo[18] = '\0';
        //make_timestamp();
        if (!lock_geral) {
          Serial.println(" Amostra Enviada");
          Remote_Temp_Characteristic->writeValue(pacote_completo, sizeof(pacote_completo));
          time_start_send = millis();
        }
      }
    }
    //}
  }
  else if (Scan_BLE) {
    BLEDevice::getScan() -> start(0);
  } else if (!IMConnected) {
    //initBLE();
  }
}
