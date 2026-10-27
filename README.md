# Monitor de Invernadero con ESP32 y Qt

Sistema de monitoreo ambiental y de suelo.

## Componentes

* ESP32-WROOM-32D
* [BME280 por I2C](https://altronics.cl/sensor-presion-gy-bme280-5v)
* [Sensor de suelo 7 en 1 por protocolo RS485](https://es.aliexpress.com/item/1005011872084206.html?spm=a2g0o.productlist.main.1.35d7KgGjKgGjUk&algo_pvid=d77b0b10-f321-46b3-944d-9a4bb2363cae&pdp_ext_f=%7B%22order%22%3A%22349%22%2C%22eval%22%3A%221%22%2C%22fromPage%22%3A%22search%22%7D&utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005011872084206%7C_p_origin_prod%3A)
* [Transceptor RS485 a MAX485](https://triacs.cl/modulos/379-modulo-rs485-a-max485-ttl-.html)
* Aplicación de escritorio Qt Widgets

## Estructura

* firmware/: programa del ESP32
* dashboard/: aplicación Qt

## Variables

### Ambiente

* Humedad relativa (%)
* Temperatura (°C)
* Presión atmosférica (hPa)

### Suelo

* Humedad (%)
* Temperatura (°C)
* Conductividad eléctrica (µS/cm)
* pH
* Nitrógeno, fósforo y potasio (mg/kg)

## Configuración de comunicación

* PC a ESP32: 115200 baudios, 8N1
* ESP32 a sensor de suelo: 9600 baudios, 8N1
* Dirección Modbus del sensor: 2

## Dependencias Arduino

* Soporte de placas ESP32
* BME280 de finitespace
* ModbusMaster de Doc Walker

## Dependencias Qt

* Qt Widgets
* Qt Serial Port
* Compilador compatible con el kit de Qt utilizado

## Formato serial (lectura)

DATA,humAmb,tempAmb,presHpa,humSuelo,tempSuelo,ecUsCm,ph,n,p,k

## Ejecución

1. Abrir ```firmware/MonitorESP32.ino```
2. Instalar las dependencias y cargar en el ESP32
3. Abrir el archivo de proyecto de dashboard en Qt Creator
4. Configurar el kit y compilar
5. Seleccionar el puerto del ESP32 y 115200 baudios
6. Conectar desde el dashboard

## Reconocimiento

Proyecto desarrollado en el marco del curso de Especialización Completa en C/C++ de [CODAERUS](https://codaerus.com/)
