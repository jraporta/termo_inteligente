/*
  firmware para un termo_inteligente v0.0.7
*/

#include <WiFi101.h>
#include <PubSubClient.h>
#include <RTCZero.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_SleepyDog.h>
#include "RoundRobinbyJR.h"
#include "arduino_secrets.h"   //please enter your sensitive data in the Secret tab/termo001_secrets.h
#include "fakeTempSensor.h"

char fileName[] = "cache.txt";

void messageReceived(char* topic, byte* payload, unsigned int length);
bool connectMqttServer();
void homiePublish();
void setRTCwithNTP();
void printTime();
void printDate();
void mqttOrSD(char* topic, char* payload, char* fName = fileName);
void publishSensors(bool resistChanged = false);
void publicaModo(bool serie = false);

char ssid[] = SECRET_SSID;     // your network SSID (name)
char pass[] = SECRET_PASS;     // your network password

int status = WL_IDLE_STATUS;
IPAddress server(192, 168, 1, 43); //MQTT Broker ip
int port = 8883;
WiFiSSLClient net;
PubSubClient mqttClient(server, port, messageReceived, net);

RTCZero rtc; // create an RTC object
const int GMT = 1; //change this to adapt it to your time zone

File myFile;

char buffer [40];
const int bufferLen = 80;
char buffer2 [bufferLen];

float h2oTemperature = 20;
int consigna = 60, histeresis = 10, maxTemperature = 80, minTemperature = 3, minHisteresis = 3, maxHisteresis = 20;
bool modoAuto = true, forcedON, errorSD = false, rtcFlag = false;
bool resistenciaON = false;
unsigned long lastMillis = 0, lastReport = 0, lastAlert = 0;
int reportT = 10000, measuringT = 3000, flagSD = 0, contadorAlerta = 0;

const int pinCaudalimetro = 5, pinPower = 0, pinAlert = 3, pilotoResistencia = A3 , pinRelay = 1, pinRButton = 2;
float kCaudal = 5.5;
volatile int pulsosCaudal = 0;
float caudal = 0;

void setup() {

  bool reset = false;
  pinMode(pinPower, OUTPUT);
  digitalWrite(pinPower, HIGH);
  pinMode(pinAlert, OUTPUT);
  digitalWrite(pinAlert, LOW);
  pinMode(pilotoResistencia, OUTPUT);
  digitalWrite(pilotoResistencia, LOW);
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, LOW);
  pinMode(pinRButton, INPUT_PULLUP);

  delay(5000);
  Serial.begin(9600);
  Serial.println("firmware: termo_inteligente 0.0.7");

  int watch_dog = Watchdog.enable(180000);
  Serial.print("Enabled the watchdog with max countdown of ");
  Serial.print(watch_dog, DEC);
  Serial.println(" milliseconds!");

  connectMqttServer();
  mqttClient.publish("homie/termo001/$state", "init", true);
  if (!digitalRead(pinRButton)) {
    Serial.println("Resetting..............");
    reset = true;
    mqttClient.publish("homie/termo001/termostato/consigna/set", "", true);
    mqttClient.publish("homie/termo001/termostato/histeresis/set", "", true);
    mqttClient.publish("homie/termo001/resistencia/modo/set", "", true);
  }
  homieSubscribe(); //Aunque ponga un delay después no consigo que se ejecute la recepción de los mensajes retenidos antes de publicar.
  homiePublish();

  Serial.print("Initializing SD card...");
  if (!SD.begin(4)) {
    Serial.println("failed to open SD.");
    mqttClient.publish("homie/termo001/sd/estado", "missing", true);
    errorSD = true;
  } else {
    if (reset) {
      SD.remove(fileName);
      digitalWrite(pinAlert, HIGH);
      delay(1000);
      digitalWrite(pinAlert, LOW);
    }
    flagSD = NumberOfLogs(fileName);
  }

  rtc.begin();     // initialize the RTC library
  setRTCwithNTP(); // set the RTC time/date using epoch from NTP

  pinMode(pinCaudalimetro, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinCaudalimetro), caudalInterrupt, RISING);
  mqttClient.publish("homie/termo001/$state", "ready", true);
}

void loop() {
  Watchdog.reset();
  mqttClient.loop();
  if (!mqttClient.connected())  {
    Serial.print("Not connected to MQTT broker...");
    if (connectMqttServer()) {
      homieSubscribe();
      mqttClient.publish("homie/termo001/$state", "ready", true);
      publishSensors(true);
    }
  }
  if (!rtcFlag) {
    setRTCwithNTP();
  }

  if ((lastMillis + measuringT) < millis()) {
    getTemperature(resistenciaON, &h2oTemperature);
    actualizarResistencia();
    Serial.print(h2oTemperature);
    Serial.print("ºC, consumo de ");
    Serial.print(caudal);
    Serial.print(" l/min y la resistencia está: ");
    if (resistenciaON) {
      Serial.println("encendida.");
    } else {
      Serial.println("apagada.");
    }
    lastMillis = millis();
    if (((lastReport + reportT) < lastMillis) || (lastReport == 0)) {
      publishSensors();
    }
    if (h2oTemperature > (maxTemperature + 3)) {
      if ((lastAlert + 10 * measuringT) < lastMillis) {
        contadorAlerta = 1;
      } else {
        contadorAlerta ++;
      }
      lastAlert = lastMillis;
    }
  }

  digitalWrite(pilotoResistencia, resistenciaON);
  digitalWrite(pinRelay, resistenciaON);

  if (contadorAlerta >= 5) {
    Watchdog.disable();
    mqttOrSD("homie/termo001/$state", "alert");
    while (true) {
      digitalWrite(pinAlert, !digitalRead(pinAlert));
      delay(500);
    }
  }
}

void actualizarResistencia() {
  if (modoAuto) {
    if (h2oTemperature < (consigna - histeresis) && (resistenciaON == false)) {
      resistenciaON = true;
      publishSensors(true);
    } else if (h2oTemperature >= consigna && (resistenciaON == true)) {
      resistenciaON = false;
      publishSensors(true);
    }
  } else {
    if (forcedON) {
      if (h2oTemperature < (maxTemperature - minHisteresis) && resistenciaON == false) {
        resistenciaON = true;
        publishSensors(true);
      } else if (h2oTemperature >= maxTemperature && resistenciaON == true) {
        resistenciaON = false;
        publishSensors(true);
      }
    } else if (resistenciaON) {
      resistenciaON = false;
      publishSensors(true);
    }
  }
}

void publishSensors(bool resistChanged) {
  Serial.println("Reporting...");
  unsigned long epoch = rtc.getEpoch();
  if (resistChanged) {
    if (resistenciaON) {
      snprintf(buffer, 40, "true@%u", epoch);
      mqttOrSD("homie/termo001/resistencia/encendida", buffer);
    } else {
      snprintf(buffer, 40, "false@%u", epoch);
      mqttOrSD("homie/termo001/resistencia/encendida", buffer);
    }
  }
  snprintf(buffer, 40, "%f@%u", h2oTemperature, epoch);
  mqttOrSD("homie/termo001/termostato/temperatura", buffer);
  int segundos = (lastMillis - lastReport) / 1000;
  caudal = (pulsosCaudal / segundos) * 60 / kCaudal; //(Pulse frequency x 60) / 5.5 = flow rate (https://seeeddoc.github.io/G3-4_Water_Flow_sensor/)
  pulsosCaudal = 0;
  snprintf(buffer, 40, "%f@%u", caudal , epoch);
  mqttOrSD("homie/termo001/caudalimetro/caudal", buffer);
  lastReport = lastMillis;
}

void mqttOrSD(char* topic, char* payload, char* fName) {
  if (!mqttClient.publish(topic, payload, true)) {
    Serial.println("Unable to send MQTT msg, saving in SD.");
    if (!errorSD && (myFile = SD.open(fName, FILE_WRITE))) {
      Serial.print("Writing to ");
      Serial.print(fName);
      Serial.print(" .....");
      myFile.print(topic);
      myFile.print("<");
      myFile.println(payload);
      myFile.close();
      Serial.println("done.");
      flagSD++;
    } else {
      Serial.println("error opening file");
    }
  }
}

void SDtoMqtt() {
  String s = "";
  Serial.println("Dumping SD contents to broker.");
  SD.remove("tempfile.txt");
  for (int i = 0 ; i <= flagSD; i++) {
    s = ReadLine(fileName, i);
    s.toCharArray(buffer2, bufferLen);
    mqttOrSD("homie/termo001/sd/dump", buffer2, "tempfile.txt");
  }
  SD.remove(fileName);
  flagSD = NumberOfLogs("tempfile.txt");
  if (flagSD) {
    CopyFile("tempfile.txt", fileName);
    Serial.println("Some/All of the msgs were not sent to the MQTT broker.");
    mqttClient.publish("homie/termo001/$state", "ready", true);
  } else {
    mqttClient.publish("homie/termo001/sd/estado", "empty", true);
  }
  SD.remove("tempfile.txt");
  Serial.println("Finished SDtoMqtt().");
}


void setRTCwithNTP() {
  if ( status == WL_CONNECTED) {
    unsigned long epoch = 0;
    Serial.println("Setting the RTC:");
    Serial.print("getting time...");
    epoch = WiFi.getTime();
    delay(1000);
    if (epoch == 0) {
      Serial.println("NTP unreachable!!");
    } else  {
      Serial.print("Epoch received: ");
      Serial.println(epoch);
      rtc.setEpoch(epoch);
      rtc.setHours(rtc.getHours() + GMT);
      rtcFlag = true;
      printTime();     // print the current time
      printDate();     // print the current date
    }
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

bool connectMqttServer() {
  bool b = false;
  connectWifi();
  if ( status == WL_CONNECTED) {
    Serial.print("Connecting to MQTT Broker...");
    if (!mqttClient.connect("termo001", SECRET_USERNAME, SECRET_PASSWORD, "homie/termo001/$state", 2, 1, "lost", true)) {
      Serial.println("error.");
    } else {
      Serial.println("done!!! ");
      b = true;
      if (!errorSD) {
        if (flagSD) {
          mqttOrSD ("homie/termo001/sd/estado", "ready");
        } else {
          mqttOrSD ("homie/termo001/sd/estado", "empty");
        }
      }
    }
    delay(1000);
  }
  return b;
}

void connectWifi() {
  Serial.println("checking wifi...");
  status = WiFi.status();
  if ( status != WL_CONNECTED) {
    Serial.print("Connecting to SSID: ");
    Serial.print(ssid);
    Serial.print("...");
    status = WiFi.begin(ssid, pass);      // Connect to WPA/WPA2 network
    delay(1000);
  }
  if ( status != WL_CONNECTED) {
    Serial.println("error");
  } else {
    Serial.println("done!!!");
    printWiFiStatus();
  }
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
  publicaConsigna();
  mqttClient.publish("homie/termo001/termostato/consigna/$name", "Temperatura deseada", true);
  mqttClient.publish("homie/termo001/termostato/consigna/$datatype", "integer", true);
  mqttClient.publish("homie/termo001/termostato/consigna/$unit", "ºC", true);
  mqttClient.publish("homie/termo001/termostato/consigna/$settable", "true", true);
  mqttClient.publish("homie/termo001/termostato/consigna/$format", "0:100", true);
  publicaHisteresis();
  mqttClient.publish("homie/termo001/termostato/histeresis/$name", "Histéresis", true);
  mqttClient.publish("homie/termo001/termostato/histeresis/$datatype", "integer", true);
  mqttClient.publish("homie/termo001/termostato/histeresis/$unit", "ºC", true);
  mqttClient.publish("homie/termo001/termostato/histeresis/$settable", "true", true);
  mqttClient.publish("homie/termo001/termostato/histeresis/$format", "1:50", true);
  publicatmax();
  mqttClient.publish("homie/termo001/termostato/tmax/$name", "Temperatura máxima de seguridad", true);
  mqttClient.publish("homie/termo001/termostato/tmax/$datatype", "integer", true);
  mqttClient.publish("homie/termo001/termostato/tmax/$unit", "ºC", true);
  mqttClient.publish("homie/termo001/termostato/tmax/$settable", "false", true);
  mqttClient.publish("homie/termo001/termostato/tmax/$format", "60:90", true);
  mqttClient.publish("homie/termo001/caudalimetro/$name", "Caudalímetro", true);
  mqttClient.publish("homie/termo001/caudalimetro/$properties", "caudal,k", true);
  mqttClient.publish("homie/termo001/caudalimetro/caudal/$name", "Caudal", true);
  mqttClient.publish("homie/termo001/caudalimetro/caudal/$datatype", "float", true);
  mqttClient.publish("homie/termo001/caudalimetro/caudal/$unit", "l/min", true);
  mqttClient.publish("homie/termo001/caudalimetro/caudal/$format", "0:60", true);
  publicak();
  mqttClient.publish("homie/termo001/caudalimetro/k/$name", "Constante del caudalímetro", true);
  mqttClient.publish("homie/termo001/caudalimetro/k/$datatype", "float", true);
  mqttClient.publish("homie/termo001/caudalimetro/k/$unit", "hz·min/l", true);
  mqttClient.publish("homie/termo001/caudalimetro/k/$format", "0:50", true);
  mqttClient.publish("homie/termo001/caudalimetro/k/$settable", "false", true);
  mqttClient.publish("homie/termo001/caudalimetro/k/$format", "60:90", true);
  mqttClient.publish("homie/termo001/resistencia/$name", "Resistencia", true);
  mqttClient.publish("homie/termo001/resistencia/$type", "2 kW", true);
  mqttClient.publish("homie/termo001/resistencia/$properties", "encendida,modo", true);
  mqttClient.publish("homie/termo001/resistencia/encendida/$name", "Resistencia encendida", true);
  mqttClient.publish("homie/termo001/resistencia/encendida/$datatype", "boolean", true);
  mqttClient.publish("homie/termo001/resistencia/encendida/$settable", "false", true);
  publicaModo();
  mqttClient.publish("homie/termo001/resistencia/modo/$name", "Modo de funcionamiento", true);
  mqttClient.publish("homie/termo001/resistencia/modo/$datatype", "enum", true);
  mqttClient.publish("homie/termo001/resistencia/modo/$settable", "true", true);
  mqttClient.publish("homie/termo001/resistencia/modo/$format", "auto,manualON,manualOFF", true);
  mqttClient.publish("homie/termo001/sd/$name", "micro-SD", true);
  mqttClient.publish("homie/termo001/sd/$type", "8GB", true);
  mqttClient.publish("homie/termo001/sd/$properties", "estado,orden", true);
  mqttClient.publish("homie/termo001/sd/estado/$name", "Estado de la SD", true);
  mqttClient.publish("homie/termo001/sd/estado/$datatype", "enum", true);
  mqttClient.publish("homie/termo001/sd/estado/$format", "ready,missing,error,writing,empty,sending", true);
  mqttClient.publish("homie/termo001/sd/download/$name", "Publish start to begin", true);
}

void homieSubscribe() {
  mqttClient.subscribe("homie/termo001/termostato/consigna/set");
  mqttClient.subscribe("homie/termo001/termostato/histeresis/set");
  mqttClient.subscribe("homie/termo001/resistencia/modo/set");
  mqttClient.subscribe("homie/termo001/sd/download");
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
    consigna = min(s.toInt(), maxTemperature);
    consigna = max(consigna, minTemperature);
    publicaConsigna();
    Serial.print("Nueva T de consigna: ");
    Serial.println(consigna);
  } else if (strcmp(topic, "homie/termo001/termostato/histeresis/set") == 0) {
    histeresis = min(s.toInt(), maxHisteresis);
    histeresis = max(histeresis, minHisteresis);
    publicaHisteresis();
    Serial.print("Nueva T de histeresis: ");
    Serial.println(histeresis);
  } else if (strcmp(topic, "homie/termo001/sd/download") == 0 && s == "start" && flagSD) {
    mqttClient.publish("homie/termo001/sd/estado", "sending", true);
    SDtoMqtt();
  }  else if (strcmp(topic, "homie/termo001/resistencia/modo/set") == 0) {
    if (s == "auto") {
      modoAuto = true;
      publicaModo(true);
    } else if (s == "manualON") {
      modoAuto = false;
      forcedON = true;
      publicaModo(true);
    } else if (s == "manualOFF") {
      modoAuto = false;
      forcedON = false;
      publicaModo(true);
    }
  }
}

void publicaConsigna() {
  snprintf(buffer, 20, "%d", consigna);
  mqttClient.publish("homie/termo001/termostato/consigna", buffer, true);
}

void publicaHisteresis() {
  snprintf(buffer, 20, "%d", histeresis);
  mqttClient.publish("homie/termo001/termostato/histeresis", buffer, true);
}

void publicaModo(bool serie) {
  char* t;
  if (modoAuto) {
    t = "auto";
  } else if (!modoAuto && forcedON) {
    t == "manualON";
  } else if (!modoAuto && !forcedON) {
    t == "manualOFF";
  }
  mqttClient.publish("homie/termo001/resistencia/modo", t, true);
  if (serie) {
    Serial.print("Configurado en modo: ");
    Serial.println(t);
  }
}

void publicatmax() {
  snprintf(buffer, 20, "%d", histeresis);
  mqttClient.publish("homie/termo001/termostato/tmax", buffer, true);
}

void publicak() {
  snprintf(buffer, 20, "%f", kCaudal);
  mqttClient.publish("homie/termo001/caudalimetro/k", buffer, true);
}

void caudalInterrupt() {
  pulsosCaudal++;
}
