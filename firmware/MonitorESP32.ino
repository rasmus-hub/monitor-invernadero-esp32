#include <Arduino.h>
#include <Wire.h>
#include <BME280I2C.h>
#include <ModbusMaster.h>
#include <math.h>

// ================= COMUNICACIÓN USB =================
constexpr uint32_t BAUD_USB = 115200;

// ================= BME280 =================
constexpr int PIN_SDA = 21;
constexpr int PIN_SCL = 22;

BME280I2C bme;
bool bmeDisponible = false;

// ================= SENSOR RS485 =================
constexpr int RS485_RX  = 16;
constexpr int RS485_TX  = 17;
constexpr int RS485_DIR = 25;

constexpr uint32_t BAUD_SUELO = 9600;
constexpr uint8_t ID_SUELO = 2;

HardwareSerial puertoRS485(2);
ModbusMaster suelo;

// ================= TEMPORIZACIÓN =================
constexpr uint32_t INTERVALO_MS = 2000;
uint32_t ultimaConsulta = 0;

void antesDeTransmitir()
{
    digitalWrite(RS485_DIR, HIGH);
}

void despuesDeTransmitir()
{
    digitalWrite(RS485_DIR, LOW);
}

void setup()
{
    Serial.begin(BAUD_USB);

    pinMode(RS485_DIR, OUTPUT);
    digitalWrite(RS485_DIR, LOW);

    Wire.begin(PIN_SDA, PIN_SCL);

    bmeDisponible =
        bme.begin() &&
        bme.chipModel() == BME280::ChipModel_BME280;

    if (bmeDisponible) {
        Serial.println("# BME280 listo");
    } else {
        Serial.println("# ERROR: BME280 no disponible");
    }

    puertoRS485.begin(
        BAUD_SUELO,
        SERIAL_8N1,
        RS485_RX,
        RS485_TX
    );

    suelo.begin(ID_SUELO, puertoRS485);
    suelo.preTransmission(antesDeTransmitir);
    suelo.postTransmission(despuesDeTransmitir);

    Serial.println("# Sistema listo; USB 115200; Modbus ID 2");

    ultimaConsulta = millis();
}

void adquirirYEnviar()
{
    // Reintenta detectar el BME280 si falló al arrancar.
    if (!bmeDisponible) {
        bmeDisponible =
            bme.begin() &&
            bme.chipModel() == BME280::ChipModel_BME280;

        if (!bmeDisponible) {
            Serial.println("# ERROR: BME280 no disponible");
            return;
        }
    }

    float temperaturaAmb = NAN;
    float humedadAmb = NAN;
    float presionPa = NAN;

    bme.read(
        presionPa,
        temperaturaAmb,
        humedadAmb,
        BME280::TempUnit_Celsius,
        BME280::PresUnit_Pa
    );

    if (!isfinite(temperaturaAmb) ||
        !isfinite(humedadAmb) ||
        !isfinite(presionPa)) {

        Serial.println("# ERROR: lectura BME280 invalida");
        bmeDisponible = false;
        return;
    }

    const uint8_t resultado =
        suelo.readHoldingRegisters(0x0000, 8);

    if (resultado != suelo.ku8MBSuccess) {
        Serial.print("# ERROR: Modbus 0x");
        Serial.println(resultado, HEX);
        return;
    }

    const uint16_t temperaturaRaw =
        suelo.getResponseBuffer(0);

    const int32_t temperaturaConSigno =
        (temperaturaRaw & 0x8000)
            ? static_cast<int32_t>(temperaturaRaw) - 65536L
            : static_cast<int32_t>(temperaturaRaw);

    const float temperaturaSuelo =
        temperaturaConSigno / 10.0f;

    const float humedadSuelo =
        suelo.getResponseBuffer(1) / 10.0f;

    const uint16_t ecUsCm = suelo.getResponseBuffer(2);

    // Registro 3: salinidad, no incluida en este protocolo.
    const uint16_t nitrogeno = suelo.getResponseBuffer(4);
    const uint16_t fosforo   = suelo.getResponseBuffer(5);
    const uint16_t potasio   = suelo.getResponseBuffer(6);

    const float ph =
        suelo.getResponseBuffer(7) / 10.0f;

    const float presionHpa = presionPa / 100.0f;

    // Una muestra completa, terminada en salto de línea.
    Serial.print("DATA,");

    Serial.print(humedadAmb, 2);
    Serial.print(',');
    Serial.print(temperaturaAmb, 2);
    Serial.print(',');
    Serial.print(presionHpa, 2);
    Serial.print(',');

    Serial.print(humedadSuelo, 2);
    Serial.print(',');
    Serial.print(temperaturaSuelo, 2);
    Serial.print(',');
    Serial.print(ecUsCm);
    Serial.print(',');
    Serial.print(ph, 2);
    Serial.print(',');

    Serial.print(nitrogeno);
    Serial.print(',');
    Serial.print(fosforo);
    Serial.print(',');
    Serial.println(potasio);
}

void loop()
{
    const uint32_t ahora = millis();

    if (ahora - ultimaConsulta >= INTERVALO_MS) {
        ultimaConsulta = ahora;
        adquirirYEnviar();
    }
}