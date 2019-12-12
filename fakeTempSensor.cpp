#include<arduino.h>

void getTemperature(bool resistenciaON, float* h2oTemperature) {
  pinMode(A5, INPUT_PULLUP); //velocidad de simulación //No me esta funcionando pullup en pin analog.
  pinMode(A6, INPUT_PULLUP); //lectura de temperatura
  int velSimulacion = 100;
  //velSimulacion = map(analogRead(A5), 0, 1023, 1, 1000);
  //Serial.print("velocidad de simulación es: ");
  //Serial.println(velSimulacion);
  //int lectura = analogRead(A6);
  int inputTemp = 5;
  //inputTemp = map(lectura, 0, 1023, 100, 0);
  //Serial.print("Input de temperatura es: ");
  //Serial.println(inputTemp);

  if (inputTemp < 10) {
    if (resistenciaON == true) {
      *h2oTemperature += 0.018 * velSimulacion;
    } else {
      float Q = 6.25 * (*h2oTemperature - 20);
      *h2oTemperature -= Q * 0.000009 * velSimulacion;
    }
  } else {
    *h2oTemperature = inputTemp;
  }
}
