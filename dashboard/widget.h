#ifndef WIDGET_H                            // Evita incluir dos veces este archivo.
#define WIDGET_H

#include <QWidget>                         // Clase base de las ventanas y controles.
#include <QColor>                          // Colores de gauges y curvas.
#include <QDateTime>                       // Fecha y hora de recepción en el PC.
#include <QElapsedTimer>                   // Tiempo transcurrido sin depender del reloj civil.
#include <QVector>                         // Almacena las muestras de la sesión.
#include <QString>
#include <QSerialPort>                     // Comunicación con el ESP32.

class QLabel;                              // Declaraciones adelantadas: reducen inclusiones.
class QComboBox;
class QPushButton;
class QDoubleSpinBox;
class QCheckBox;
class QTimer;
class QPaintEvent;

struct Lectura
{
    QDateTime fecha;

    // Ambiente
    double humedad = 0.0;
    double temperatura = 0.0;
    double presion = 0.0;          // hPa

    // Suelo
    double humedadSuelo = 0.0;     // %
    double temperaturaSuelo = 0.0; // °C
    double ec = 0.0;               // µS/cm
    double ph = 0.0;
    double nitrogeno = 0.0;        // mg/kg
    double fosforo = 0.0;          // mg/kg
    double potasio = 0.0;          // mg/kg
};

class Gauge : public QWidget               // Indicador circular dibujado con QPainter.
{
public:
    Gauge(const QString &titulo, const QString &unidad,
          double minimo, double maximo, const QColor &color,
          QWidget *parent = nullptr);      // Constructor del indicador.
    void setValor(double valor);           // Actualiza el valor y solicita repintado.
    void setVigente(bool vigente);          // Atenúa la lectura cuando está desactualizada.
protected:
    void paintEvent(QPaintEvent *) override; // Qt llama a esta función al dibujar.
private:
    QString titulo, unidad;                // Textos mostrados en el gauge.
    double minimo, maximo, valor = 0.0;     // Rango y último dato recibido.
    QColor color;                          // Color de la variable.
    bool tieneDato = false;                // Permite mostrar -- antes de recibir datos.
    bool vigente = false;                  // Indica si la lectura es reciente.
};

class Grafica : public QWidget
{
public:
    Grafica(
        const QString &titulo,
        const QString &unidad,
        double Lectura::*campo,
        const QColor &color,
        QWidget *parent = nullptr
        );

    void agregar(const Lectura &lectura);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString titulo;
    QString unidad;

    // Indica qué miembro de Lectura debe representar esta gráfica.
    double Lectura::*campo;

    QColor color;
    QVector<Lectura> puntos;

    static constexpr int MAX_MUESTRAS = 180;
};

class Widget : public QWidget              // Conserva el nombre de tu ventana original.
{
    Q_OBJECT                               // Habilita el sistema de metaobjetos de Qt.
public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;
private:
    void crearInterfaz();                  // Construye y distribuye los controles.
    void actualizarPuertos();              // Busca puertos disponibles sin conectar solo.
    void conectarSerial();                // Abre o cierra el puerto elegido.
    void desconectarSerial();             // Restablece el estado de conexión.
    void leerSerial();                     // Reconstruye y procesa líneas completas.
    void procesarLinea(QByteArray linea);   // Valida dos números antes de utilizarlos.
    void evaluarRiego();                   // Evalúa umbrales y envía 1 una vez por evento.
    void exportarCSV();                    // Guarda todos los registros de esta sesión.
    void actualizarVigencia();             // Detecta 5 segundos sin lecturas válidas.
    void marcarDatosNoVigentes();

    QSerialPort *serial;                   // Qt destruye los objetos hijos automáticamente.
    QComboBox *puertos, *baudios;           // Selección del puerto y velocidad.
    QPushButton *botonConectar, *botonPuertos, *botonCSV;
    QLabel *estadoConexion;
    QLabel *ultimaLectura, *contador, *descartadas;
    QDoubleSpinBox *umbralHumedad, *umbralTemperatura;
    QCheckBox *automatico;                 // El docente habilita el envío desde la interfaz.
    Gauge *gaugeHumedadAmb, *gaugeTemperaturaAmb, *gaugePresion,
        *gaugeHumedadSuelo, *gaugeTemperaturaSuelo,
        *gaugeEC, *gaugePH, *gaugeN, *gaugeP, *gaugeK;
    QVector<Grafica *> graficas;
    QTimer *reloj;                         // Verifica periódicamente la vigencia del dato.
    QElapsedTimer edadLectura;             // Se reinicia al recibir una lectura válida.
    QByteArray buffer;                     // Conserva las líneas que llegan fragmentadas.
    bool descartandoLinea = false;         // Recuperación ante una línea excesivamente larga.
    QVector<Lectura> registros;            // Historial COMPLETO para el CSV de la sesión.
    Lectura ultima;                        // Última lectura válida para evaluar los umbrales.
    bool tieneLectura = false;             // Impide evaluar datos aún inexistentes.
    bool ordenEnviada = false;             // Evita repetir 1 continuamente mientras hay alarma.
    quint64 invalidas = 0;                 // Número de líneas recibidas y rechazadas.
};
#endif // WIDGET_H
