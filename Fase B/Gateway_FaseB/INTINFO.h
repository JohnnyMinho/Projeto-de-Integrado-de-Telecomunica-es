#ifndef INTINFO_H_
#define INTINFO_H_


char* ssidlab = "LAP3";
char* passwordlab = "LAP3LAP3";

char* ssidcasa = "NOS26C65";
char* passwordcasa = "4CSXR6MT";

char* ssidhotspot = "SinsPhone";
char* passwordhotspot = "123456789";

char* ssidlab2 = "TP-Link_B3E8";
char* passwordlab2 = "28918158";


char* host = "localhost";
int port = 4444;

char* client_name = "SSMAIN1";

byte STOPbyte = 0b00000001; // Tipo - 1, Timestamp - 8 (2bytes - horas, 2 bytes - minutos, 2 bytes - segundos)

byte STARTbyte = 0b00000010; // Tipo - 1, Timestamp - 8

byte BEGINbyte = 0b00000011; // Tipo - 1

byte ENDbyte = 0b00000100; // Tipo - 1

byte DATAbyte = 0b00000101; // Tipo - 1, Timestamp - 8, Dados - 17 bytes(5-TEMP,5-Humd,7- Press_atm)

byte DISCbyte = 0b00000110; // Tipo - 1, Timestamp - 6 bytes

byte ERRORbyte = 0b00000111;

#endif
