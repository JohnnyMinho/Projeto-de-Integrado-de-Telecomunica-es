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
boolean temperatura_nova = false;
boolean humidade_nova = false;
boolean pressure_nova = false;
boolean send_new = false; //Para permitir que apenas uma amostra seja enviada, usamos esta variável
boolean lock = false;
boolean valid_status = false; //Esta variável serve para confirmar que todos os dados dos sensores são válidos
boolean WiFi_Connected = false;
unsigned status_bmp;
unsigned long time_start = 0; //timer para apresentar dados
unsigned long time_start_send = 0; // timer para enviar para o gateway
String toSend_Temp;
String toSend_Humd;
String toSend_Press;
static BLEUUID UUID_SERVER_SERVICE("72add083-b931-4efc-ba71-4eaf935e0465");
static BLEUUID UUID_Characteristic_Server("cf15383a-afe7-4d44-b24d-a6363e93793e");
static BLEUUID UUID_TEMP_CHARACTERISTIC("eea1aa7d-b9b9-4ddf-a575-05b7a37b139c");
static BLEUUID UUID_HUMD_CHARACTERISTIC("9d9031b5-191f-4e2d-9791-1c2240e74a8d");
static BLEUUID UUID_PRESS_CHARACTERISTIC ("01bcb788-7258-43b3-be93-f35dad0ac2f4");
static BLERemoteCharacteristic* Remote_Humd_Characteristic;
static BLERemoteCharacteristic* Remote_Temp_Characteristic;
static BLERemoteCharacteristic* Remote_Temp_SERVER;
static BLERemoteCharacteristic* Remote_Press_Characteristic;
static BLEAdvertisedDevice* Gateway_Usado; //Servidor Basicamente
char pacote_temp[6];
char pacote_humd[6];
char pacote_press[6];
/*byte pacote_comando[2]; //TRAMA DE COMANDO, 1 Byte Header, 1 Byte Dados
  byte byteSTOP = 0b00010000; //A trama de comando deve indicar o tempo pelo qual o sistema sensor deve desligar, caso seja 0 , desliga indifinitivamente
  byte byteSTART = 0b00010010;
  byte byteCHANGETIME = 0b00010100;*/
char* Temp_Data;
char* Humd_Data;
char* Pressure_Data;

/* Variáveis WIFI*/
char* ssid = ssidlab2;
char* password = passwordlab2;

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
      IMConnected = false;
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
        temperatura_nova = false;
        humidade_nova = false;
        pressure_nova = false;
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
  Serial.println(value);
  if (value[0] == 'N') {

    //IMConnected = false;
    //doConnect  = true;
    lock = true;
  }
  else if (value[0] == 'Y') {
    lock = false;
    //initBLE();
  }
  else if(value[0] == 'T'){
    struct tm timevalue;
    time_t now_time;
    char hours[2];
    char minutes[2];
    char seconds[2];
    for(int i=0;i<2;i++){
    hours[i] = value[i+1];
    minutes[i] = value[i+3];
    seconds[i] = value[i+5];
    }
    timevalue.tm_hour = atoi(hours);
    timevalue.tm_min = atoi(minutes);
    timevalue.tm_sec = atoi(seconds);
    time(&now_time);
    localtime_r(&now_time, &timevalue);
  }
}


static void notifyCallBack_TEMP(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {

}

static void notifyCallBack_HUMD(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {

}

static void notifyCallBack_PRESSURE(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {


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

  Remote_Humd_Characteristic = Temp_Servico -> getCharacteristic(UUID_HUMD_CHARACTERISTIC);

  Remote_Temp_Characteristic = Temp_Servico -> getCharacteristic(UUID_TEMP_CHARACTERISTIC);

  Remote_Press_Characteristic = Temp_Servico -> getCharacteristic(UUID_PRESS_CHARACTERISTIC);

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
  if (Remote_Humd_Characteristic == nullptr) { //Só verificamos os casos em que falhou já que se tudo correr bem, não é necessário uma alteração do progresso
    SENSORES->disconnect();
    Serial.println("Characteristic Error");
    return false;
  }
  if (Remote_Press_Characteristic == nullptr) {
    SENSORES->disconnect();
    Serial.println("Characteristic Error");
    return false;
  }

  if (Remote_Temp_SERVER->canRead(), Remote_Humd_Characteristic->canRead(), Remote_Temp_Characteristic->canRead(), Remote_Press_Characteristic->canRead()) {
    std::string mostrar = Remote_Temp_SERVER->readValue();
    std::string mostrar2 = Remote_Temp_Characteristic->readValue();
    std::string mostrar3 = Remote_Humd_Characteristic->readValue();
    std::string mostrar4 = Remote_Press_Characteristic->readValue();
    Serial.println(mostrar.c_str());
    Serial.println(mostrar2.c_str());
    Serial.println(mostrar3.c_str());
    Serial.println(mostrar4.c_str());
  }

  if (Remote_Temp_SERVER->canNotify()) {
    Remote_Temp_SERVER->registerForNotify(notifyCallBack_SERVER);
  }
  if (Remote_Humd_Characteristic->canNotify()) {
    Remote_Humd_Characteristic->registerForNotify(notifyCallBack_HUMD);
  }
  if (Remote_Temp_Characteristic->canNotify()) {
    Remote_Temp_Characteristic->registerForNotify(notifyCallBack_TEMP);
  }
  if (Remote_Press_Characteristic->canNotify()) {
    Remote_Press_Characteristic->registerForNotify(notifyCallBack_HUMD);
  }
  IMConnected = true; //Como todos os passos foram validados, podemos considerar que o Sensor está conectado ao GATEWAY logo o booleano que assim o define é dado o valor de true
  return true;
}

/*int roundvalue(float x){
  int y;
  if(x-(int)x<0.5){
    y = floor(x);
  }
  if(x-(int)x>0.5){
    y = ceil(x);
  }
  return y;
  }*/

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
    while (status_bmp = bmp.begin()) {
      Serial.println("FAILED TO START BMP");
      delay(5000);
    }
  }
}

void make_timestamp() { //Faz um timestamp através do servidor NTP,
  struct tm timestamp; //tm é uma struct que guarda os tempos em ints consoante o tipo (anos, meses, dias, etc)
  while (!getLocalTime(&timestamp)) {
    //Serial.println("Failed to obtain time");
    //return;
  }
  Serial.print(&timestamp, "%H:%M:%S");
}

void setup() {
  Serial.begin(115200);
  initBLE();
}

/*void get_data(unsigned long timer_receb){
  if(timer_receb-millis() >= 1000){
  humd = dht.readHumidity();
  temp = dht.readTemperature();
  pressure = (bmp.readPressure()/100);
  if(isnan(temp) || isnan(humd) || isnan(pressure)){
      Serial.println("Failed to read temperature or humidity");
    }
  String toSend_Temp = (String(temp));
  String toSend_Humd = (String(humd));
  String toSend_Press = (String(pressure));
  }
  }*/

//void wait

void loop() {
  //String toSend_Humd;
  //String toSend_Temp;
  if (!lock) {
    if (doConnect) {
      if (conectar_servidor()) {
        Serial.println("Estou conectado ao gateway");
      }
      else {
        Serial.println("Não estou conectado. Morri.");
      }
      doConnect = false;
    }
    if (IMConnected) {
      //sensors_event_t pressure_event;
      //bmp_pressure->getEvent(&pressure_event);

      if (millis() - time_start >= 1000) {
        humd = dht.readHumidity();
        temp = dht.readTemperature();
        pressure = (bmp.readPressure() / 100);
        if (isnan(temp) || isnan(humd) || isnan(pressure)) {
          valid_status = false;
        }
        else {
          valid_status = true;
        }
        time_start = millis();
      }
      if (!valid_status) {
        //Serial.println("Failed to read temperature or humidity or pressure");
      }
      else {
        String toSend_Temp = (String(temp));
        String toSend_Humd = (String(humd));
        String toSend_Press = (String(pressure));
        make_timestamp();
        Serial.print(" -> Temperatura: ");
        Serial.println(toSend_Temp);
        make_timestamp();
        Serial.print(" -> Humd: ");
        Serial.println(toSend_Humd);
        make_timestamp();
        Serial.print(" -> Presão atmosférica: ");
        Serial.println(toSend_Press);
        valid_status = false;
        //getdata();
        if (!temperatura_nova && !humidade_nova && !pressure_nova) {
          temperatura_nova = true;
          humidade_nova = true;
          pressure_nova = true;
        }
        //Serial.println(temp);
        //Serial.println(humd);

        

        //String toSend_data = (toSend_Temp + " " + toSend_Humd);
        if ((millis() - time_start_send) >= 20000) {
          for(int i = 0; i<6; i++){
            pacote_temp[i] = toSend_Temp[i];
            pacote_humd[i] = toSend_Humd[i];
            pacote_press[i] = toSend_Press[i];
            pacote_temp[6] = '\0';
            pacote_humd[6] = '\0';
            pacote_press[6] = '\0';
            make_timestamp();
            Serial.println("Amostra Enviada");
            Remote_Temp_Characteristic->writeValue(pacote_temp, 6);
            Remote_Humd_Characteristic->writeValue(pacote_humd, 6);
            Remote_Press_Characteristic->writeValue(pacote_press,6);
          }
         /* if (temperatura_nova && send_new) { //Código antigo
            pacote[0] = toSend_Temp[0];
            pacote[1] = toSend_Temp[1];
            pacote[2] = toSend_Temp[2];
            pacote[3] = toSend_Temp[3];
            pacote[4] = toSend_Temp[4];
            pacote[5] = toSend_Temp[5];
            pacote[6] = '\0';
            make_timestamp();
            Remote_Temp_Characteristic->writeValue(pacote, 6);
            Serial.println("Temperatura Enviada");
            temperatura_nova = false;
            send_new = false;
          }
          //delay(Tempo_Amostra);
          if (humidade_nova && send_new) {
            make_timestamp();
            pacote[0] = toSend_Humd[0];
            pacote[1] = toSend_Humd[1];
            pacote[2] = toSend_Humd[2];
            pacote[3] = toSend_Humd[3];
            pacote[4] = toSend_Humd[4];
            pacote[5] = toSend_Humd[5];
            pacote[6] = '\0';
            Serial.println(" : Humidade Enviada");
            Remote_Humd_Characteristic->writeValue(pacote, 6);
            humidade_nova = false;
            send_new = false;
          }
          //delay(Tempo_Amostra);
          if (pressure_nova && send_new) {
            make_timestamp();
            pacote[0] = toSend_Press[0];
            pacote[1] = toSend_Press[1];
            pacote[2] = toSend_Press[2];
            pacote[3] = toSend_Press[3];
            pacote[4] = toSend_Press[4];
            pacote[5] = toSend_Press[5];
            pacote[6] = '\0';
            Serial.println(" : Pressão Atmosférica Enviada");
            Remote_Press_Characteristic->writeValue(pacote, 6);
            pressure_nova = false;
            send_new = false;
          }*/
          time_start_send = millis();
          //delay(Tempo_Amostra);
        }
        // Remote_Temp_SERVER->writeValue(toSend_Temp.c_str(), toSend_Temp.length());
        // Remote_Humd_Characteristic->writeValue(toSend_Humd.c_str(), toSend_Humd.length());
        //Remote_Temp_SERVER->writeValue(toSend_data.c_str(), toSend_data.length());
      }
    }
    else if (Scan_BLE) {
      BLEDevice::getScan() -> start(0);
    }
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
}
