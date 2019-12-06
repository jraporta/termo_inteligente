/*
  firmware para un termo_inteligente v0.0.3
*/

#include <WiFi101.h>
#include <PubSubClient.h>
#include <RTCZero.h>
#include "arduino_secrets.h"   //please enter your sensitive data in the Secret tab/termo001_secrets.h

void messageReceived(char* topic, byte* payload, unsigned int length);
void connectMqttServer();
void homiePublish();
void setRTCwithNTP();
void printTime();
void printDate();

char ssid[] = SECRET_SSID;     // your network SSID (name)
char pass[] = SECRET_PASS;     // your network password

int status = WL_IDLE_STATUS;
IPAddress server(192, 168, 1, 43); //MQTT Broker ip
//int port = 1883;
int port = 8883;
WiFiSSLClient net;
PubSubClient mqttClient(server, port, messageReceived, net);

RTCZero rtc; // create an RTC object
const int GMT = 1; //change this to adapt it to your time zone

char buffer [20];

float h2oTemperature = 20;
int consigna = 60, histeresis = 10, maxTemperature = 90, minHisteresis = 5;
bool modoAuto = true, forcedON;
bool resistenciaON = false;
unsigned long lastMillis = 0, lastReport = 0;
int reportT = 60000, measuringT = 5000;

void setup() {
  delay(5000);
  Serial.begin(9600);
  Serial.println("termo_inteligente 0.0.1");

  connectMqttServer();
  mqttClient.publish("homie/termo001/$state", "init", true);
  homiePublish();

  Serial.println("Setting the RTC:");
  rtc.begin();     // initialize the RTC library
  setRTCwithNTP(); // set the RTC time/date using epoch from NTP
  printTime();     // print the current time
  printDate();     // print the current date

  homieSubscribe();

  mqttClient.publish("homie/termo001/$state", "ready", true);
}

void loop() {

  mqttClient.loop();
  if (!mqttClient.connected())  {
    connectMqttServer();
    homieSubscribe();
    mqttClient.publish("homie/termo001/$state", "ready", true);
  }

  if ((lastMillis + measuringT) < millis()) {
    getTemperature();
    lastMillis = millis();
    Serial.print(h2oTemperature);
    Serial.print("ºC y la resistencia está: ");
    Serial.println(resistenciaON);
    if (((lastReport + reportT) < lastMillis) || (lastReport == 0)) {
      Serial.println("Reporting...");
      publishSensors();
      lastReport = lastMillis;
    }
  }
  if (modoAuto) {
    if (h2oTemperature < (consigna - histeresis) && (resistenciaON == false)) {
      resistenciaON = true;
      mqttClient.publish("homie/termo001/resistencia/encendida", "true", true);
    } else if (h2oTemperature >= consigna && (resistenciaON == true)) {
      resistenciaON = false;
      mqttClient.publish("homie/termo001/resistencia/encendida", "false", true);
    }
  } else {
    if (forcedON) {
      if (h2oTemperature < (maxTemperature - minHisteresis) && resistenciaON == false) {
        resistenciaON = true;
        mqttClient.publish("homie/termo001/resistencia/encendida", "true", true);
      } else if (h2oTemperature >= maxTemperature && resistenciaON == true) {
        resistenciaON = false;
        mqttClient.publish("homie/termo001/resistencia/encendida", "false", true);
      }
    } else if (resistenciaON) {
      resistenciaON = false;
      mqttClient.publish("homie/termo001/resistencia/encendida", "false", true);
    }
  }
}

void getTemperature() {
  if (resistenciaON == true) {
    h2oTemperature += 1.06;
  } else {
    h2oTemperature -= 0.24;
  }
}


void publishSensors() {
  snprintf(buffer, 20, "%f", h2oTemperature);
  mqttClient.publish("homie/termo001/termostato/temperatura", buffer, true);
}



void setRTCwithNTP() {
  unsigned long epoch = 0;
  int numberOfTries = 0, maxTries = 50;
  while ((epoch == 0) && (numberOfTries < maxTries)) {
    if ( status != WL_CONNECTED) {
      connectWifi();
    }
    Serial.print("getting time...");
    epoch = WiFi.getTime();
    delay(1000);
    Serial.println("done");
    numberOfTries++;
  }
  if (numberOfTries >= maxTries) {
    Serial.print("NTP unreachable!!");
    WiFi.disconnect();
  } else  {
    Serial.print("Epoch received: ");
    Serial.println(epoch);
    rtc.setEpoch(epoch);
    rtc.setHours(rtc.getHours() + GMT);
  }
}

void printTime() {
  print2digits(rtc.getHours());
  Serial.print(":");
  print2digits(rtc.getMinutes());
  Serial.print(":");
  print2digits(rtc.getSeconds());
  Serial.println();
}

void printDate() {
  print2digits(rtc.getDay());
  Serial.print("/");
  print2digits(rtc.getMonth());
  Serial.print("/");
  print2digits(rtc.getYear());
  Serial.println("");
}

void print2digits(int number) {
  if (number < 10) {
    Serial.print("0");
  }
  Serial.print(number);
}

void connectMqttServer() {
  Serial.println("checking wifi...");
  if ( status != WL_CONNECTED) {
    connectWifi();
  }
  Serial.print("Connecting to MQTT Broker...");
  while (!mqttClient.connect("termo001", SECRET_USERNAME, SECRET_PASSWORD, "homie/termo001/$state", 2, 1, "lost", true)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("done!!! ");
}

void connectWifi() {
  Serial.print("\nConnecting to SSID: ");
  Serial.print(ssid);
  Serial.print("...");
  while ( status != WL_CONNECTED) {
    Serial.print(".");
    status = WiFi.begin(ssid, pass);      // Connect to WPA/WPA2 network
    delay(5000);
  }
  Serial.println("done!!!");
  printWiFiStatus();
}

void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP: ");
  Serial.println(ip);

  Serial.print("signal strength (RSSI):");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

void homiePublish() {
  mqttClient.publish("homie/termo001/$homie", "4.0.0", true);
  mqttClient.publish("homie/termo001/$name", "Smartthermo 80L_2000_Vo", true);
  mqttClient.publish("homie/termo001/$nodes", "tesmostato,caudalimetro,resistencia,sd", true);
  mqttClient.publish("homie/termo001/$extensions", "", true);
  mqttClient.publish("homie/termo001/termostato/$name", "Termostato", true);
  mqttClient.publish("homie/termo001/termostato/$type", "", true);
  mqttClient.publish("homie/termo001/termostato/$properties", "temperatura,consigna,histeresis,tmax", true);
  mqttClient.publish("homie/termo001/termostato/temperatura/$name", "Temperatura medida", true);
  mqttClient.publish("homie/termo001/termostato/temperatura/$datatype", "float", true);
  mqttClient.publish("homie/termo001/termostato/temperatura/$unit", "ºC", true);
  mqttClient.publish("homie/termo001/termostato/temperatura/$format", "0:100", true);
  mqttClient.publish("homie/termo001/termostato/consigna/$name", "Temperatura deseada", true);
  mqttClient.publish("homie/termo001/termostato/consigna/$datatype", "integer", true);
  mqttClient.publish("homie/termo001/termostato/consigna/$unit", "ºC", true);
  mqttClient.publish("homie/termo001/termostato/consigna/$settable", "true", true);
  mqttClient.publish("homie/termo001/termostato/consigna/$format", "0:100", true);
  mqttClient.publish("homie/termo001/termostato/histeresis/$name", "Histéresis", true);
  mqttClient.publish("homie/termo001/termostato/histeresis/$datatype", "integer", true);
  mqttClient.publish("homie/termo001/termostato/histeresis/$unit", "ºC", true);
  mqttClient.publish("homie/termo001/termostato/histeresis/$settable", "true", true);
  mqttClient.publish("homie/termo001/termostato/histeresis/$format", "1:50", true);
  mqttClient.publish("homie/termo001/termostato/tmax/$name", "Temperatura máxima de seguridad", true);
  mqttClient.publish("homie/termo001/termostato/tmax/$datatype", "integer", true);
  mqttClient.publish("homie/termo001/termostato/tmax/$unit", "ºC", true);
  mqttClient.publish("homie/termo001/termostato/tmax/$settable", "true", true);
  mqttClient.publish("homie/termo001/termostato/tmax/$format", "60:90", true);
  mqttClient.publish("homie/termo001/caudalimetro/$name", "Caudalímetro", true);
  mqttClient.publish("homie/termo001/caudalimetro/$properties", "caudal,k", true);
  mqttClient.publish("homie/termo001/caudalimetro/caudal/$name", "Caudal", true);
  mqttClient.publish("homie/termo001/caudalimetro/caudal/$datatype", "float", true);
  mqttClient.publish("homie/termo001/caudalimetro/caudal/$unit", "l/min", true);
  mqttClient.publish("homie/termo001/caudalimetro/caudal/$format", "0:50", true);
  mqttClient.publish("homie/termo001/caudalimetro/k/$name", "Constante del caudalímetro", true);
  mqttClient.publish("homie/termo001/caudalimetro/k/$datatype", "float", true);
  mqttClient.publish("homie/termo001/caudalimetro/k/$unit", "hz·min/l", true);
  mqttClient.publish("homie/termo001/caudalimetro/k/$format", "0:50", true);
  mqttClient.publish("homie/termo001/caudalimetro/k/$settable", "true", true);
  mqttClient.publish("homie/termo001/caudalimetro/k/$format", "60:90", true);
  mqttClient.publish("homie/termo001/resistencia/$name", "Resistencia", true);
  mqttClient.publish("homie/termo001/resistencia/$type", "2 kW", true);
  mqttClient.publish("homie/termo001/resistencia/$properties", "encendida,modo", true);
  mqttClient.publish("homie/termo001/resistencia/encendida/$name", "Resistencia encendida", true);
  mqttClient.publish("homie/termo001/resistencia/encendida/$datatype", "boolean", true);
  mqttClient.publish("homie/termo001/resistencia/encendida/$settable", "false", true);
  mqttClient.publish("homie/termo001/resistencia/modo/$name", "Modo de funcionamiento", true);
  mqttClient.publish("homie/termo001/resistencia/modo/$datatype", "enum", true);
  mqttClient.publish("homie/termo001/resistencia/modo/$settable", "true", true);
  mqttClient.publish("homie/termo001/resistencia/modo/$format", "auto,manualON,manualOFF", true);
  mqttClient.publish("homie/termo001/sd/$name", "micro-SD", true);
  mqttClient.publish("homie/termo001/sd/$type", "8GB", true);
  mqttClient.publish("homie/termo001/sd/$properties", "estado", true);
  mqttClient.publish("homie/termo001/sd/estado/$name", "Estado de la SD", true);
  mqttClient.publish("homie/termo001/sd/estado/$datatype", "enum", true);
  mqttClient.publish("homie/termo001/sd/estado/$format", "ready,missing,error,writing", true);
}

void homieSubscribe() {
  mqttClient.subscribe("homie/termo001/termostato/consigna/set");
  mqttClient.subscribe("homie/termo001/termostato/histeresis/set");
  mqttClient.subscribe("homie/termo001/resistencia/modo/set");
}

void messageReceived(char* topic, byte* payload, unsigned int length) {
  String s = "";
  for (int i = 0; i < length; i++) {
    s = s + (char)payload[i];
  }
  Serial.print("Message received:\nTopic is: ");
  Serial.println(topic);
  Serial.print("Payload is: ");
  Serial.println(s);

  if (strcmp(topic, "homie/termo001/termostato/consigna/set") == 0) {
    consigna = s.toInt();
    snprintf(buffer, 20, "%d", consigna);
    mqttClient.publish("homie/termo001/termostato/consigna", buffer, true);
    Serial.print("Nueva T de consigna: ");
    Serial.println(consigna);
  } else if (strcmp(topic, "homie/termo001/termostato/histeresis/set") == 0) {
    histeresis = s.toInt();
    snprintf(buffer, 20, "%d", histeresis);
    mqttClient.publish("homie/termo001/termostato/histeresis", buffer, true);
    Serial.print("Nueva T de histeresis: ");
    Serial.println(histeresis);
  } else if (strcmp(topic, "homie/termo001/resistencia/modo/set") == 0) {
    if (s == "auto") {
      modoAuto = true;
      mqttClient.publish("homie/termo001/resistencia/modo", "auto", true);
      Serial.print("Configurado en modo: ");
      Serial.println(s);
    } else if (s == "manualON") {
      modoAuto = false;
      forcedON = true;
      mqttClient.publish("homie/termo001/resistencia/modo", "manualON", true);
      Serial.print("Configurado en modo: ");
      Serial.println(s);
    } else if (s == "manualOFF") {
      modoAuto = false;
      forcedON = false;
      mqttClient.publish("homie/termo001/resistencia/modo", "manualOFF", true);
      Serial.print("Configurado en modo: ");
      Serial.println(s);
    }
  }
}
