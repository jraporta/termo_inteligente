# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Mejorar fiabilidad del setRTCwithNTP().
- Editar parámetros consigna, histéresis, etc. remotamente por MQTT.
- Gestión de tarifa eléctrica de discriminación horaria.
- Base de datos en SD.
- Volcado de datos de la SD a la BD centralizada.
- Asegurar funcionamiento sin conexión al broker MQTT ni WiFi.
- Añadir caudalímetro.
- Añadir Watchdog.

## [0.0.1] - 05/12/2019

Milestone reached: Prototype

- Primera versión del firmware de un termo eléctrico "inteligente".
- Publicación de parámetros internos por MQTT y suscripción a parámetros de configuración.
- Ajuste del RTC por NTP.
- Simulación básica del comportamiento de un termo.
- Ajustado a la convención homie 4.0.0.