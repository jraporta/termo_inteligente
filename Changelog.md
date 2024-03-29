# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
- Gestión de tarifa eléctrica de discriminación horaria.
- Añadir funcionalidad anticongelación.
- Recuperación de variables de configuración al reiniciarse el termo (Si no hay MQTT )
- Actualizaciones OTA.

## [Known Issues]
- temperatura, resistenciaON, caudalímetro,etc. no deberían ser "retained", pero como llevan timestamp ya se sabe de cuando son.
- Si todavía no ha recibido la hora por NTP, guarda el mensaje al SD con un timestamp erróneo.

## [Possible Issues]
- Cómo reacciona a archivos de backup grandes. ¿Hay que ir vaciando el archivo?
- Compatibilidad de sleepydog con mkr1000.

## [0.1.1] - 12/12/2019

### Added
- Añadido aviso de servicio técnico requerido.

## [0.1.0] - 12/12/2019

### Added
- Alerta de avería (temperatura por encima de la máxima).
- Implementación del relé.
- Led de encendido, avería y funcionamiento de la resistencia.
- Botón de reset.

### Changed
- Mejorado el "simulador" de temperatura del termo.

## [0.0.7] - 11/12/2019

### Added
- Añadido Watchdog.
- Añadida publicación de temperatura de consigna, histéresis, k, Tmax y modo.
- Establecidos máximos y mínimos para las variables de configuración.

### Tested
- Recuperación de variables de configuración por MQTT (a publicar como retained).

## [0.0.6] - 09/12/2019

### Added
- Añadido caudalímetro.

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