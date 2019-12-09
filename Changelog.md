# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
- Establecer máximos y mínimos para las variables de configuración.
- Usar sensor del MKR ENV Shield.
- Editar todos los parámetros del termo remotamente por MQTT.
- Gestión de tarifa eléctrica de discriminación horaria.
- Añadir Watchdog.
- Añadir funcionalidad anticongelación.
- Recuperación de variables de configuración al reiniciarse el termo.
- Actualizaciones OTA.
- Mejorar "simulador" de temperatura del termo.
- Alertas de mal funcionamiento.

## [Known Issues]
- temperatura, resistenciaON, caudalímetro,etc. no deberían ser "retained".
- Publicaciones de la SD no se deberían guardar en la SD (no usar mqttOrSD().
- Temperatura baja indefinidamente con la resistencia apagada.
- Mosquitto no guarda los mensajes retenidos al reiniciar la Rpi (de Juan Ramon).
- Si todavía no ha recibido la hora por NTP, guarda el mensaje al SD con un timestamp erróneo.

## [Possible Issues]
- Cómo reacciona a archivos de backup grandes. ¿Hay que ir vaciando el archivo?
- El programa se traga payloads aunque no correspondan con el tipo de variable... ¿Control en NodeRed?
- QoS, pensar en cual es el apropiado.

## [0.0.6] - 09/12/2019

### Added
- - Añadido caudalímetro.

## [0.0.5] - 09/12/2019

### Added
- Las publicaciones incorporan el timestamp del arduino.

## [0.0.4] - 06/12/2019
Milestone reached: Minimum requirements

### Added
- Base de datos en SD.
- Volcado de datos de la SD a la BD centralizada por MQTT.
- Permitir funcionamiento sin conexión al broker MQTT ni WiFi.

### Changed
- Arreglado (hasta cierto punto) el NTP.

## [0.0.3] - 06/12/2019

### Added
- Implementado modo de funcionamiento automático y manual.

### Changed
- Corregidos bugs en las publicaciones MQTT.

## [0.0.2] - 06/12/2019

### Added
- Editar parámetros consigna, histéresis y modo de funcionamiento remotamente por MQTT.

## [0.0.1] - 05/12/2019
Milestone reached: Prototype

- Primera versión del firmware de un termo eléctrico "inteligente".
- Publicación de parámetros internos por MQTT y suscripción a parámetros de configuración.
- Ajuste del RTC por NTP.
- Simulación básica del comportamiento de un termo.
- Ajustado a la convención homie 4.0.0.