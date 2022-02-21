#include "RF24.h"
#include "nRF24L01.h"
#include <SPI.h>;
#include <printf.h>;
#include "CRCx.h";
#include "DHT.h";
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define DHTPIN 17 //Define o pino a que o DHT vai transmitir os dados na placa ESP32, neste caso é o RX logo é o pin 40
#define TIPODHT DHT11 //Define o tipo de DHT usado neste caso o DHT11

DHT dht(DHTPIN, TIPODHT);

float temp = 0; //guardar temperatura
float humd = 0; //guardar humidade

void setup() {
  
  Serial.begin(115200);
  BLEDevice::init("SENSOR_TEMP");
  dht.begin();

}

void loop() {
  humd = dht.readHumidity();
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
  }
  delay(1000);
}
