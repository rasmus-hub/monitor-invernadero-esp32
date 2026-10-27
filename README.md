# Monitor de Invernadero con ESP32 y Qt

Sistema de monitoreo ambiental y de suelo.

## Componentes

- ESP32-WROOM-32D.
- BME280 por I2C.
- Sensor de suelo multiparámetro por RS485.
- Módulo MAX485 con adaptación de nivel hacia el RX del ESP32.
- Aplicación de escritorio Qt Widgets.

## Estructura

- firmware/: programa del ESP32.
- dashboard/: aplicación Qt.
- docs/: conexiones y documentación.

## Variables

### Ambiente

- Humedad relativa (%).
- Temperatura (°C).
- Presión atmosférica (hPa).

### Suelo

- Humedad (%).
- Temperatura (°C).
- Conductividad eléctrica (µS/cm).
- pH.
- Nitrógeno, fósforo y potasio (mg/kg).

## Configuración de comunicación

- PC a ESP32: 115200 baudios, 8N1.
- ESP32 a sensor de suelo: 9600 baudios, 8N1.
- Dirección Modbus del sensor: 2.

## Dependencias Arduino

- Soporte de placas ESP32.
- BME280 de finitespace.
- ModbusMaster de Doc Walker.

## Dependencias Qt

- Qt Widgets.
- Qt Serial Port.
- Compilador compatible con el kit de Qt utilizado.

## Formato serial

DATA,humAmb,tempAmb,presHpa,humSuelo,tempSuelo,ecUsCm,ph,n,p,k

## Ejecución

1. Abrir firmware/MonitorESP32/MonitorESP32.ino.
2. Instalar las dependencias y cargar en el ESP32.
3. Abrir el archivo de proyecto de dashboard en Qt Creator.
4. Configurar el kit y compilar.
5. Seleccionar el puerto del ESP32 y 115200 baudios.
6. Conectar desde el dashboard.

Cerrar el monitor serial de Arduino antes de conectar desde Qt.