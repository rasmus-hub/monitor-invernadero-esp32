#include "widget.h"                       // Declaraciones de las tres clases del proyecto.
#include <QLabel>                         // Textos informativos.
#include <QComboBox>                      // Listas de puertos y velocidades.
#include <QPushButton>                    // Botones de conexión y exportación.
#include <QDoubleSpinBox>                 // Umbrales con decimales.
#include <QCheckBox>                      // Habilita el control automático.
#include <QVBoxLayout>                    // Distribución vertical adaptable.
#include <QHBoxLayout>                    // Distribución horizontal adaptable.
#include <QGridLayout>                    // Distribución en filas y columnas.
#include <QScrollArea>
#include <QFrame>                         // Tarjetas visuales de la interfaz.
#include <QPainter>                       // Dibuja gauges, ejes y curvas.
#include <QPainterPath>                   // Une los puntos de cada curva.
#include <QSerialPortInfo>                // Enumera los puertos presentes en el PC.
#include <QFileDialog>                    // Permite elegir dónde guardar el CSV.
#include <QSaveFile>                      // Guarda sin dejar un CSV parcial si falla.
#include <QMessageBox>                    // Mensajes de error y confirmación.
#include <QTimer>                         // Temporizador de supervisión.
#include <QLocale>                        // Formato decimal independiente de Windows.
#include <QtMath>                         // Funciones trigonométricas y conversión angular.
#include <cmath>                          // std::isfinite rechaza NaN e infinito.

namespace {                               // Funciones auxiliares privadas de este archivo.
const QColor VERDE("#76BC21");             // Acento Codaerus.
const QColor ROJO("#D60000");              // Identifica errores o rechazo.
const QColor AZUL("#249FC7");              // Identifica la humedad.
const QColor NARANJA("#E79845");           // Identifica la temperatura.
const QColor AZUL_OSCURO("#0046DE");       // Identifica EC y presión atmosférica.
const QColor MORADO("#5300D9");            // Identifica NPK.
const QColor TEXTO("#ECF2F5");             // Texto sobre fondo oscuro.
const QColor SUAVE("#9DAEBB");             // Texto secundario.

QLabel *texto(const QString &contenido, int tamano, const QColor &color,
              bool negrita = false)       // Crea un texto con estilo consistente.
{
    auto *label = new QLabel(contenido);   // El layout asignará su padre y posición.
    QFont fuente("Segoe UI", tamano);      // Define tipografía y tamaño.
    fuente.setBold(negrita);               // Activa negrita cuando se solicita.
    label->setFont(fuente);                // Aplica la fuente al label.
    label->setStyleSheet("color:" + color.name() + "; background:transparent;");
    return label;                         // Devuelve el control creado.
}

QFrame *tarjeta()                          // Contenedor reutilizable para cada sección.
{
    auto *frame = new QFrame;              // Su layout padre tomará la propiedad.
    frame->setObjectName("tarjeta");       // Se identifica en la hoja de estilos.
    return frame;                         // Devuelve la tarjeta vacía.
}
}                                         // Finaliza el espacio de nombres privado.

// ======================== GAUGE CIRCULAR ========================
Gauge::Gauge(const QString &titulo, const QString &unidad, double minimo,
             double maximo, const QColor &color, QWidget *parent)
    : QWidget(parent), titulo(titulo), unidad(unidad),
    minimo(minimo), maximo(maximo), color(color) // Inicializa los atributos.
{
    setStyleSheet("background:transparent;"); // Se integra con la tarjeta contenedora.
    setMinimumSize(245, 240);              // Evita que las etiquetas se superpongan.
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void Gauge::setValor(double nuevo)
{
    valor = nuevo;                        // Conserva el dato real, incluyendo decimales.
    tieneDato = true;                     // Ya puede mostrar un número.
    vigente = true;                       // La lectura acaba de recibirse.
    update();                             // Programa un repintado sin bloquear.
}

void Gauge::setVigente(bool nuevo)
{
    vigente = nuevo;                      // Cambia únicamente el estado visual.
    update();                             // Redibuja el gauge con su estado actual.
}

void Gauge::paintEvent(QPaintEvent *)
{
    QPainter p(this);                     // Dibuja exclusivamente sobre este widget.
    p.setRenderHint(QPainter::Antialiasing); // Suaviza los bordes de los arcos.
    const double lado = qMin(width(), height()); // Conserva proporción al redimensionar.
    p.translate((width() - lado) / 2.0, (height() - lado) / 2.0);
    p.scale(lado / 260.0, lado / 260.0);    // Usa un sistema de coordenadas de 260 x 260.
    p.setPen(TEXTO);                       // Color del título.
    p.setFont(QFont("Segoe UI", 12, QFont::Bold));
    p.drawText(QRectF(0, 6, 260, 28), Qt::AlignCenter, titulo);
    const QRectF arco(38, 49, 184, 184);    // Círculo sobre el que se dibuja el indicador.
    p.setPen(QPen(QColor("#2A3A47"), 13, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(arco, 225 * 16, -270 * 16);   // Qt expresa ángulos en dieciseisavos de grado.
    const double proporcion = qBound(0.0, (valor - minimo) / (maximo - minimo), 1.0);
    if (tieneDato) {                       // Antes del primer dato solo muestra el fondo.
        p.setOpacity(vigente ? 1.0 : 0.35); // Atenúa datos que ya no son recientes.
        p.setPen(QPen(color, 13, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(arco, 225 * 16, qRound(-270 * 16 * proporcion));
        const double angulo = qDegreesToRadians(225.0 - 270.0 * proporcion);
        const QPointF punta(130 + 66 * qCos(angulo), 141 - 66 * qSin(angulo));
        p.setPen(QPen(color, 3, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(130, 141), punta); // Aguja que apunta al valor actual.
        p.setBrush(color);                 // Rellena el centro de la aguja.
        p.setPen(Qt::NoPen);               // Sin borde adicional.
        p.drawEllipse(QPointF(130, 141), 5, 5);
        p.setOpacity(1.0);                 // Restablece opacidad para los textos.
    }
    p.setPen(TEXTO);                       // Lectura numérica de alto contraste.
    p.setFont(QFont("Segoe UI", 23, QFont::Bold));
    p.drawText(QRectF(35, 159, 190, 40), Qt::AlignCenter,
               tieneDato ? QString::number(valor, 'f', 1) : "--");
    p.setPen(SUAVE);                       // Unidad y límites discretos.
    p.setFont(QFont("Segoe UI", 9));
    p.drawText(QRectF(80, 198, 100, 22), Qt::AlignCenter, unidad);
    p.drawText(QRectF(22, 223, 65, 20), Qt::AlignCenter, QString::number(minimo));
    p.drawText(QRectF(173, 223, 65, 20), Qt::AlignCenter, QString::number(maximo));
}

// ======================== GRÁFICA TEMPORAL ========================

Grafica::Grafica(
    const QString &titulo,
    const QString &unidad,
    double Lectura::*campo,
    const QColor &color,
    QWidget *parent
    )
    : QWidget(parent),
    titulo(titulo),
    unidad(unidad),
    campo(campo),
    color(color)
{
    setMinimumSize(380, 270);

    setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
        );
}

void Grafica::agregar(const Lectura &lectura)
{
    puntos.append(lectura);

    while (puntos.size() > MAX_MUESTRAS) {
        puntos.removeFirst();
    }

    update();
}

void Grafica::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF fondo(
        0.5,
        0.5,
        width() - 1.0,
        height() - 1.0
        );

    p.setPen(QPen(QColor("#2D404D"), 1));
    p.setBrush(QColor("#192833"));
    p.drawRoundedRect(fondo, 12, 12);

    // ================= TÍTULO =================

    p.setPen(color);
    p.setFont(QFont("Segoe UI", 10, QFont::Bold));

    p.drawText(
        QRectF(16, 10, width() - 32, 25),
        Qt::AlignLeft | Qt::AlignVCenter,
        titulo
        );

    p.setPen(QColor("#9DAEBB"));
    p.setFont(QFont("Segoe UI", 9));

    p.drawText(
        QRectF(16, 36, width() - 32, 23),
        Qt::AlignLeft | Qt::AlignVCenter,
        "Unidad: " + unidad
        );

    // Área reservada para la curva.
    const QRectF area(
        82,
        76,
        width() - 106,
        height() - 126
        );

    if (area.width() <= 0 || area.height() <= 0) {
        return;
    }

    // ================= ESTADO SIN DATOS =================

    if (puntos.isEmpty()) {
        p.setPen(QColor("#9DAEBB"));

        p.drawText(
            area,
            Qt::AlignCenter,
            "Esperando datos del ESP32"
            );

        return;
    }

    // ================= EXTREMOS DE LOS DATOS =================

    bool hayValor = false;

    double minimo = 0.0;
    double maximo = 0.0;

    double ultimoValor = 0.0;
    qint64 ultimoInstante = 0;

    qint64 inicio = puntos.first().fecha.toMSecsSinceEpoch();
    qint64 final = inicio;

    for (const Lectura &lectura : puntos) {
        const qint64 instante =
            lectura.fecha.toMSecsSinceEpoch();

        inicio = qMin(inicio, instante);
        final = qMax(final, instante);

        const double valor = lectura.*campo;

        if (!std::isfinite(valor)) {
            continue;
        }

        if (!hayValor) {
            minimo = valor;
            maximo = valor;
            hayValor = true;
        } else {
            minimo = qMin(minimo, valor);
            maximo = qMax(maximo, valor);
        }

        ultimoValor = valor;
        ultimoInstante = instante;
    }

    if (!hayValor) {
        p.setPen(QColor("#9DAEBB"));

        p.drawText(
            area,
            Qt::AlignCenter,
            "No hay valores válidos"
            );

        return;
    }

    // ================= ESCALA VERTICAL AUTOMÁTICA =================

    const double centro = minimo + (maximo - minimo) / 2.0;

    // Evita un eje de altura cero cuando las lecturas son iguales.
    const double amplitudMinima =
        qMax(1.0, std::abs(centro) * 0.02);

    const double amplitud =
        qMax(maximo - minimo, amplitudMinima);

    const double limiteInferior =
        centro - amplitud * 0.60;

    const double limiteSuperior =
        centro + amplitud * 0.60;

    const double rango =
        limiteSuperior - limiteInferior;

    // ================= ESCALA TEMPORAL =================

    // Con una sola muestra, se deja un intervalo de un segundo.
    const qint64 duracion =
        qMax<qint64>(1000, final - inicio);

    const auto posicion = [&](qint64 instante, double valor) {
        const double x =
            area.left() +
            area.width() *
                double(instante - inicio) /
                double(duracion);

        const double y =
            area.bottom() -
            area.height() *
                (valor - limiteInferior) /
                rango;

        return QPointF(x, y);
    };

    // ================= REJILLA Y EJE VERTICAL =================

    p.setFont(QFont("Segoe UI", 8));

    for (int i = 0; i <= 4; ++i) {
        const double fraccion = i / 4.0;

        const double y =
            area.bottom() - area.height() * fraccion;

        const double valorEje =
            limiteInferior + rango * fraccion;

        p.setPen(QPen(QColor("#304553"), 1));

        p.drawLine(
            QPointF(area.left(), y),
            QPointF(area.right(), y)
            );

        p.setPen(QColor("#B7C5CF"));

        p.drawText(
            QRectF(4, y - 10, area.left() - 12, 20),
            Qt::AlignRight | Qt::AlignVCenter,
            QString::number(valorEje, 'f', 2)
            );
    }

    // ================= EJE HORIZONTAL =================

    for (int i = 0; i <= 3; ++i) {
        const double fraccion = i / 3.0;

        const double x =
            area.left() + area.width() * fraccion;

        const qint64 instante =
            inicio +
            static_cast<qint64>(duracion * fraccion);

        p.setPen(QPen(QColor("#243744"), 1));

        p.drawLine(
            QPointF(x, area.top()),
            QPointF(x, area.bottom())
            );

        p.setPen(QColor("#9DAEBB"));

        p.drawText(
            QRectF(x - 32, area.bottom() + 8, 64, 20),
            Qt::AlignCenter,
            QDateTime::fromMSecsSinceEpoch(instante)
                .toString("HH:mm:ss")
            );
    }

    // ================= CURVA =================

    QPainterPath camino;

    bool segmentoActivo = false;
    qint64 instanteAnterior = 0;

    for (const Lectura &lectura : puntos) {
        const double valor = lectura.*campo;

        const qint64 instante =
            lectura.fecha.toMSecsSinceEpoch();

        if (!std::isfinite(valor)) {
            segmentoActivo = false;
            continue;
        }

        const QPointF punto = posicion(instante, valor);

        const qint64 diferencia =
            instante - instanteAnterior;

        const bool corteTemporal =
            segmentoActivo &&
            (diferencia < 0 || diferencia > 7000);

        if (!segmentoActivo || corteTemporal) {
            camino.moveTo(punto);
        } else {
            camino.lineTo(punto);
        }

        segmentoActivo = true;
        instanteAnterior = instante;
    }

    p.save();

    p.setClipRect(area.adjusted(-4, -4, 4, 4));

    p.setBrush(Qt::NoBrush);
    p.setPen(
        QPen(
            color,
            2.3,
            Qt::SolidLine,
            Qt::RoundCap,
            Qt::RoundJoin
            )
        );

    p.drawPath(camino);

    // Marca el último valor recibido.
    const QPointF ultimoPunto =
        posicion(ultimoInstante, ultimoValor);

    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawEllipse(ultimoPunto, 3.5, 3.5);

    p.restore();

    // ================= ÚLTIMO VALOR =================

    p.setPen(color);
    p.setFont(QFont("Segoe UI", 9, QFont::Bold));

    p.drawText(
        QRectF(16, height() - 25, width() - 32, 20),
        Qt::AlignLeft | Qt::AlignVCenter,
        QString("Último: %1 %2")
            .arg(QString::number(ultimoValor, 'f', 2))
            .arg(unidad)
        );
}

// ======================== VENTANA PRINCIPAL ========================
Widget::Widget(QWidget *parent) : QWidget(parent), serial(new QSerialPort(this))
{
    setWindowTitle("Monitor de Invernadero ESP32");
    resize(1160, 840);                    // Tamaño inicial del dashboard.
    setMinimumSize(700, 500);            // Evita recortar gauges y panel de umbrales.
    crearInterfaz();                     // Construye todos los controles antes de conectar señales.
    actualizarPuertos();                 // Carga los puertos disponibles en la lista.
    serial->setReadBufferSize(8192);      // Limita el búfer interno del puerto.
    connect(serial, &QSerialPort::readyRead, this, &Widget::leerSerial);
    connect(
        serial,
        &QSerialPort::errorOccurred,
        this,
        [this](QSerialPort::SerialPortError error) {
            if (error == QSerialPort::ResourceError &&
                serial->isOpen()) {

                const QString detalle = serial->errorString();

                desconectarSerial();
                estadoConexion->setText(
                    "USB desconectado: " + detalle
                    );
            }
        }
    );
    reloj = new QTimer(this);

    connect(
        reloj,
        &QTimer::timeout,
        this,
        &Widget::actualizarVigencia
        );

    reloj->start(1000);
}

Widget::~Widget()
{
    serial->close();                     // Libera el puerto al cerrar la aplicación.
    // No envía 0: el ESP32 debe administrar la duración y parada de su riego.
}

void Widget::crearInterfaz()
{
    setStyleSheet(R"(
        QWidget { background:#101B24; color:#ECF2F5; font-family:'Segoe UI'; }
        QFrame#tarjeta { background:#192833; border:1px solid #2D404D; border-radius:14px; }
        QLabel, QCheckBox { background:transparent; }
        QComboBox, QDoubleSpinBox { background:#101B24; border:1px solid #405463;
                                  border-radius:6px; padding:7px; min-height:21px; }
        QComboBox QAbstractItemView { background:#192833; selection-background-color:#406529; }
        QPushButton { background:#293E4C; border:1px solid #405463; border-radius:7px;
                      padding:9px 15px; font-weight:600; }
        QPushButton:hover { background:#355364; }
        QPushButton:disabled { color:#6A7D8A; background:#202D36; }
        QPushButton#primario { background:#76BC21; border:0; color:#101B24; }
        QPushButton#primario:hover { background:#8DCD39; }
        QPushButton#primario:disabled { background:#40552D; color:#9BA88E; }
        QCheckBox { spacing:8px; }
        QCheckBox::indicator { width:18px; height:18px; border:1px solid #9DAEBB;
                               border-radius:4px; background:#101B24; }
        QCheckBox::indicator:checked { background:#76BC21; border:1px solid #76BC21; }
    )");                                 // Estilos centralizados para facilitar su explicación.
    // Layout exterior: ocupa toda la ventana.
    auto *layoutVentana = new QVBoxLayout(this);
    layoutVentana->setContentsMargins(0, 0, 0, 0);
    layoutVentana->setSpacing(0);

    // Área de desplazamiento.
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    // Las barras aparecen cuando el contenido no cabe.
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Contenedor para TODA la interfaz.
    auto *contenido = new QWidget;

    // Tu layout principal ahora pertenece al contenido,
    // no directamente a la ventana.
    auto *principal = new QVBoxLayout(contenido);
    principal->setContentsMargins(26, 20, 26, 18);
    principal->setSpacing(15);

    // Mantiene actualizados los límites de tamaño del contenido.
    principal->setSizeConstraint(QLayout::SetMinAndMaxSize);

    // Insertar el contenedor en el scroll y el scroll en la ventana.
    scroll->setWidget(contenido);
    layoutVentana->addWidget(scroll);
    auto *cabecera = new QHBoxLayout;
    auto *titulos = new QVBoxLayout;
    titulos->addWidget(texto("Monitor de Invernadero", 25, TEXTO, true));
    titulos->addWidget(texto("ESP32 · Variables en tiempo real", 10, SUAVE));
    cabecera->addLayout(titulos);         // Agrupa los textos de la cabecera.
    cabecera->addStretch();              // Empuja el botón hacia la derecha.
    botonCSV = new QPushButton("Exportar CSV");
    botonCSV->setObjectName("primario");
    botonCSV->setEnabled(false);         // Se habilita después de la primera lectura válida.
    cabecera->addWidget(botonCSV);
    principal->addLayout(cabecera);

    auto *conexion = tarjeta();

    auto *filaConexion = new QHBoxLayout(conexion);

    // Espacio dentro de la tarjeta: izquierda, arriba, derecha, abajo.
    filaConexion->setContentsMargins(20, 16, 20, 16);

    // Separación entre los grupos.
    filaConexion->setSpacing(20);

    // ================= PUERTO =================
    auto *grupoPuerto = new QVBoxLayout;
    grupoPuerto->setSpacing(7);

    grupoPuerto->addWidget(
        texto("PUERTO SERIAL", 9, SUAVE, true)
        );

    puertos = new QComboBox;
    puertos->setEditable(true);
    puertos->setMinimumWidth(180);

    grupoPuerto->addWidget(puertos);

    filaConexion->addLayout(grupoPuerto);

    // ================= ACTUALIZAR =================
    botonPuertos = new QPushButton("Actualizar");
    botonPuertos->setMinimumHeight(30);

    // Alinear el botón con el campo, debajo de su etiqueta.
    filaConexion->addWidget(
        botonPuertos,
        0,
        Qt::AlignBottom
        );

    // ================= VELOCIDAD =================
    auto *grupoBaudios = new QVBoxLayout;
    grupoBaudios->setSpacing(7);

    grupoBaudios->addWidget(
        texto("VELOCIDAD (BAUDIOS)", 9, SUAVE, true)
        );

    baudios = new QComboBox;
    baudios->addItems({
        "9600",
        "19200",
        "38400",
        "57600",
        "115200"
    });

    baudios->setCurrentText("115200");
    baudios->setMinimumWidth(150);

    grupoBaudios->addWidget(baudios);

    filaConexion->addLayout(grupoBaudios);

    // ================= CONECTAR =================
    botonConectar = new QPushButton("Conectar");
    botonConectar->setObjectName("primario");
    botonConectar->setMinimumHeight(30);
    botonConectar->setMinimumWidth(130);

    filaConexion->addWidget(
        botonConectar,
        0,
        Qt::AlignBottom
        );

    // ================= ESTADO =================
    auto *grupoEstado = new QVBoxLayout;
    grupoEstado->setSpacing(7);

    grupoEstado->addWidget(
        texto("ESTADO", 9, SUAVE, true)
        );

    estadoConexion = texto("Desconectado", 10, SUAVE);
    estadoConexion->setWordWrap(true);
    estadoConexion->setMinimumHeight(40);
    estadoConexion->setMinimumWidth(180);
    estadoConexion->setAlignment(
        Qt::AlignLeft | Qt::AlignVCenter
        );

    grupoEstado->addWidget(estadoConexion);

    filaConexion->addLayout(grupoEstado, 1);

    principal->addWidget(conexion);

    // ======================== GAUGES AMBIENTE ========================
    auto *filaCentralAmb = new QVBoxLayout; // Se crea una fila padre
    filaCentralAmb->setSpacing(15);
    filaCentralAmb->addWidget(texto("Variables Ambientales", 15, TEXTO, true));
    auto *medicionesAmb = tarjeta(); // Se crea un card
    auto *filaGaugesAmb = new QHBoxLayout(medicionesAmb); // Se crea una fila hija
    gaugeHumedadAmb = new Gauge("HUMEDAD", "%", 0, 100, AZUL);
    gaugeTemperaturaAmb = new Gauge("TEMPERATURA", "°C", -40, 80, NARANJA);
    gaugePresion = new Gauge("PRESIÓN ATMOSFÉRICA", "hPa", 300, 1200, AZUL_OSCURO);
    filaGaugesAmb->addWidget(gaugeHumedadAmb); // Se agregan las medicionesAmb a la fila hija
    filaGaugesAmb->addWidget(gaugeTemperaturaAmb);
    filaGaugesAmb->addWidget(gaugePresion);
    filaCentralAmb->addWidget(medicionesAmb, 3);
    principal->addLayout(filaCentralAmb, 3);

    // ======================== GAUGES SUELO ========================
    auto *filaCentralSuelo = new QVBoxLayout; // Se crea una fila padre
    filaCentralSuelo->setSpacing(15);
    filaCentralSuelo->addWidget(texto("Variables de Suelo", 15, TEXTO, true));
    auto *medicionesPrimarioSuelo = tarjeta(); // Se crea un card primario
    auto *medicionesSecundarioSuelo = tarjeta(); // Se crea un card secundaria
    auto *filaPrimariaGaugesSuelo = new QHBoxLayout(medicionesPrimarioSuelo); // Se crea una fila primaria hija
    auto *filaSecundariaGaugesSuelo = new QHBoxLayout(medicionesSecundarioSuelo); // Se crea una fila primaria hija
    gaugeHumedadSuelo = new Gauge("HUMEDAD", "%", 0, 100, AZUL);
    gaugeTemperaturaSuelo = new Gauge("TEMPERATURA", "°C", -40, 80, NARANJA);
    gaugeEC = new Gauge(
        "CONDUCTIVIDAD (EC)",
        "uS/cm",
        0,
        20000,
        AZUL_OSCURO
    );

    gaugePH = new Gauge(
        "ACIDEZ (pH)",
        "pH",
        3,
        9,
        VERDE
    );

    // Escalas visuales iniciales, no límites agronómicos.
    gaugeN = new Gauge(
        "NITROGENO (N)",
        "mg/kg",
        0,
        1000,
        MORADO
    );

    gaugeP = new Gauge(
        "FOSFORO (P)",
        "mg/kg",
        0,
        1000,
        MORADO
    );

    gaugeK = new Gauge(
        "POTASIO (K)",
        "mg/kg",
        0,
        1000,
        MORADO
    );
    filaPrimariaGaugesSuelo->addWidget(gaugeHumedadSuelo); // Se agregan las medicionesAmb a la fila hija
    filaPrimariaGaugesSuelo->addWidget(gaugeTemperaturaSuelo);
    filaSecundariaGaugesSuelo->addWidget(gaugeEC);
    filaSecundariaGaugesSuelo->addWidget(gaugePH);
    filaSecundariaGaugesSuelo->addWidget(gaugeN);
    filaSecundariaGaugesSuelo->addWidget(gaugeP);
    filaSecundariaGaugesSuelo->addWidget(gaugeK);
    filaCentralSuelo->addWidget(medicionesPrimarioSuelo, 3);
    filaCentralSuelo->addWidget(medicionesSecundarioSuelo, 3);
    principal->addLayout(filaCentralSuelo, 3);

    // ======================== GRÁFICAS ========================

    principal->addWidget(
        texto("Historial de variables", 18, TEXTO, true)
        );

    principal->addWidget(
        texto(
            "Últimas 180 muestras por variable · "
            "Escala vertical automática",
            10,
            SUAVE
            )
        );

    // Crea una gráfica y la registra para recibir nuevas lecturas.
    const auto crearGrafica = [this](
                                  const QString &titulo,
                                  const QString &unidad,
                                  double Lectura::*campo,
                                  const QColor &color
                                  ) {
        auto *nuevaGrafica = new Grafica(
            titulo,
            unidad,
            campo,
            color
            );

        graficas.append(nuevaGrafica);

        return nuevaGrafica;
    };

    // ================= AMBIENTE =================

    auto *panelAmbiente = tarjeta();
    auto *layoutAmbiente = new QVBoxLayout(panelAmbiente);

    layoutAmbiente->setContentsMargins(16, 16, 16, 16);
    layoutAmbiente->setSpacing(14);

    layoutAmbiente->addWidget(
        texto("Ambiente", 15, TEXTO, true)
        );

    auto *gridAmbiente = new QGridLayout;
    gridAmbiente->setHorizontalSpacing(14);
    gridAmbiente->setVerticalSpacing(14);

    gridAmbiente->setColumnStretch(0, 1);
    gridAmbiente->setColumnStretch(1, 1);

    gridAmbiente->addWidget(
        crearGrafica(
            "Humedad ambiental",
            "%",
            &Lectura::humedad,
            AZUL
            ),
        0, 0
        );

    gridAmbiente->addWidget(
        crearGrafica(
            "Temperatura ambiental",
            "°C",
            &Lectura::temperatura,
            NARANJA
            ),
        0, 1
        );

    gridAmbiente->addWidget(
        crearGrafica(
            "Presión atmosférica",
            "hPa",
            &Lectura::presion,
            QColor("#75A7FF")
            ),
        1, 0
        );

    layoutAmbiente->addLayout(gridAmbiente);

    principal->addWidget(panelAmbiente);

    // ================= SUELO =================

    auto *panelSuelo = tarjeta();
    auto *layoutSuelo = new QVBoxLayout(panelSuelo);

    layoutSuelo->setContentsMargins(16, 16, 16, 16);
    layoutSuelo->setSpacing(14);

    layoutSuelo->addWidget(
        texto("Suelo", 15, TEXTO, true)
        );

    auto *gridSuelo = new QGridLayout;
    gridSuelo->setHorizontalSpacing(14);
    gridSuelo->setVerticalSpacing(14);

    gridSuelo->setColumnStretch(0, 1);
    gridSuelo->setColumnStretch(1, 1);

    gridSuelo->addWidget(
        crearGrafica(
            "Humedad del suelo",
            "%",
            &Lectura::humedadSuelo,
            AZUL
            ),
        0, 0
        );

    gridSuelo->addWidget(
        crearGrafica(
            "Temperatura del suelo",
            "°C",
            &Lectura::temperaturaSuelo,
            NARANJA
            ),
        0, 1
        );

    gridSuelo->addWidget(
        crearGrafica(
            "Conductividad eléctrica",
            "uS/cm",
            &Lectura::ec,
            QColor("#75A7FF")
            ),
        1, 0
        );

    gridSuelo->addWidget(
        crearGrafica(
            "Acidez del suelo",
            "pH",
            &Lectura::ph,
            VERDE
            ),
        1, 1
        );

    gridSuelo->addWidget(
        crearGrafica(
            "Nitrógeno",
            "mg/kg",
            &Lectura::nitrogeno,
            QColor("#BA8AFF")
            ),
        2, 0
        );

    gridSuelo->addWidget(
        crearGrafica(
            "Fósforo",
            "mg/kg",
            &Lectura::fosforo,
            QColor("#F18CCB")
            ),
        2, 1
        );

    gridSuelo->addWidget(
        crearGrafica(
            "Potasio",
            "mg/kg",
            &Lectura::potasio,
            QColor("#EAC75C")
            ),
        3, 0
        );

    layoutSuelo->addLayout(gridSuelo);

    principal->addWidget(panelSuelo);
    auto *pie = new QHBoxLayout;
    ultimaLectura = texto("Última lectura: --", 9, SUAVE);
    contador = texto("Registros: 0", 9, SUAVE);
    descartadas = texto("Líneas descartadas: 0", 9, SUAVE);
    pie->addWidget(ultimaLectura);
    pie->addStretch();
    pie->addWidget(contador);
    pie->addSpacing(20);
    pie->addWidget(descartadas);
    principal->addLayout(pie);

    connect(botonPuertos, &QPushButton::clicked, this, &Widget::actualizarPuertos);
    connect(botonConectar, &QPushButton::clicked, this, &Widget::conectarSerial);
    connect(botonCSV, &QPushButton::clicked, this, &Widget::exportarCSV);
}

void Widget::actualizarPuertos()
{
    const QString anterior = puertos->currentText(); // Recuerda la selección previa.
    puertos->clear();                      // Retira puertos que ya no existen.
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
        puertos->addItem(info.portName()); // Añade cada puerto detectado.
    if (!anterior.isEmpty()) puertos->setCurrentText(anterior);
    else if (puertos->findText("COM4") >= 0) puertos->setCurrentText("COM4");
    else if (puertos->count() == 0) puertos->setCurrentText("COM4");
}

void Widget::conectarSerial()
{
    if (serial->isOpen()) {               // El mismo botón sirve para desconectar.
        desconectarSerial();
        return;
    }
    serial->setPortName(puertos->currentText().trimmed());
    serial->setBaudRate(baudios->currentText().toInt());
    serial->setDataBits(QSerialPort::Data8); // Configuración serial 8N1.
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    if (!serial->open(QIODevice::ReadWrite)) { // Lectura Y escritura para mandar el comando.
        QMessageBox::warning(this, "Conexión serial", serial->errorString());
        return;                           // Mantiene la interfaz desconectada si falla.
    }
    buffer.clear();                       // Descarta fragmentos de conexiones anteriores.
    descartandoLinea = false;
    tieneLectura = false;                  // Espera datos nuevos de la conexión actual.
    edadLectura.invalidate();
    marcarDatosNoVigentes();
    estadoConexion->setText("Conectado · esperando datos");
    botonConectar->setText("Desconectar");
    botonConectar->setStyleSheet("color:#FFFFFF;background:#D60000;");
    puertos->setEnabled(false);            // No cambia la configuración de un puerto abierto.
    baudios->setEnabled(false);
    botonPuertos->setEnabled(false);
}

void Widget::desconectarSerial()
{
    serial->close();

    buffer.clear();
    descartandoLinea = false;
    tieneLectura = false;
    edadLectura.invalidate();

    botonConectar->setText("Conectar");
    botonConectar->setStyleSheet("color:#101B24;background:#76BC21;");

    puertos->setEnabled(true);
    baudios->setEnabled(true);
    botonPuertos->setEnabled(true);

    estadoConexion->setText("Desconectado");

    marcarDatosNoVigentes();
}

// ======================== RECEPCIÓN Y VALIDACIÓN ========================
void Widget::leerSerial()
{
    const QByteArray bloque = serial->readAll();

    for (char caracter : bloque) {
        if (caracter == '\n') {
            if (!descartandoLinea) {
                procesarLinea(buffer);
            }

            buffer.clear();
            descartandoLinea = false;
            continue;
        }

        if (descartandoLinea) {
            continue;
        }

        buffer.append(caracter);

        if (buffer.size() > 512) {
            buffer.clear();
            descartandoLinea = true;

            ++invalidas;

            descartadas->setText(
                QString("Líneas descartadas: %1")
                    .arg(invalidas)
                );
        }
    }
}

void Widget::procesarLinea(QByteArray linea)
{
    linea = linea.trimmed();

    if (linea.isEmpty()) {
        return;
    }

    // Diagnósticos del ESP32: no son muestras.
    if (linea.startsWith('#')) {
        if (linea.startsWith("# ERROR")) {
            estadoConexion->setText(
                QString::fromUtf8(linea.mid(1)).trimmed()
                );

            // No dejar visibles datos como si fueran recientes
            // después de que el dispositivo informa un fallo.
            tieneLectura = false;
            marcarDatosNoVigentes();
        }

        return;
    }

    // Ignora mensajes de arranque u otros textos.
    if (!linea.startsWith("DATA,")) {
        return;
    }

    const auto rechazar = [this]() {
        ++invalidas;

        descartadas->setText(
            QString("Líneas descartadas: %1")
                .arg(invalidas)
            );
    };

    const QList<QByteArray> partes = linea.split(',');

    // Prefijo + diez variables.
    if (partes.size() != 11) {
        rechazar();
        return;
    }

    double valores[10] = {};

    for (int i = 0; i < 10; ++i) {
        bool correcto = false;

        valores[i] = partes.at(i + 1)
                         .trimmed()
                         .toDouble(&correcto);

        if (!correcto || !std::isfinite(valores[i])) {
            rechazar();
            return;
        }
    }

    const auto entre = [](double valor,
                          double minimo,
                          double maximo) {
        return valor >= minimo && valor <= maximo;
    };

    // Rangos de aceptación de esta aplicación.
    // NPK: se acepta el intervalo del registro uint16_t,
    // no se presume un rango agronómico.
    const bool valida =
        entre(valores[0], 0, 100) &&
        entre(valores[1], -40, 85) &&
        entre(valores[2], 300, 1200) &&
        entre(valores[3], 0, 100) &&
        entre(valores[4], -40, 80) &&
        entre(valores[5], 0, 20000) &&
        entre(valores[6], 3, 9) &&
        entre(valores[7], 0, 65535) &&
        entre(valores[8], 0, 65535) &&
        entre(valores[9], 0, 65535);

    if (!valida) {
        rechazar();
        return;
    }

    Lectura nueva;

    nueva.fecha = QDateTime::currentDateTime();

    nueva.humedad = valores[0];
    nueva.temperatura = valores[1];
    nueva.presion = valores[2];

    nueva.humedadSuelo = valores[3];
    nueva.temperaturaSuelo = valores[4];
    nueva.ec = valores[5];
    nueva.ph = valores[6];

    nueva.nitrogeno = valores[7];
    nueva.fosforo = valores[8];
    nueva.potasio = valores[9];

    ultima = nueva;
    registros.append(nueva);

    tieneLectura = true;
    edadLectura.restart();

    gaugeHumedadAmb->setValor(nueva.humedad);
    gaugeTemperaturaAmb->setValor(nueva.temperatura);
    gaugePresion->setValor(nueva.presion);

    gaugeHumedadSuelo->setValor(nueva.humedadSuelo);
    gaugeTemperaturaSuelo->setValor(
        nueva.temperaturaSuelo
        );

    gaugeEC->setValor(nueva.ec);
    gaugePH->setValor(nueva.ph);
    gaugeN->setValor(nueva.nitrogeno);
    gaugeP->setValor(nueva.fosforo);
    gaugeK->setValor(nueva.potasio);

    // Todas las gráficas reciben la misma muestra completa.
    for (Grafica *grafica : graficas) {
        grafica->agregar(nueva);
    }

    ultimaLectura->setText(
        "Última lectura: " +
        nueva.fecha.toString("dd/MM/yyyy HH:mm:ss")
        );

    contador->setText(
        QString("Registros: %1").arg(registros.size())
        );

    botonCSV->setEnabled(true);
    estadoConexion->setText(
        "Conectado · ambiente y suelo recibidos"
        );
}

// ======================== EXPORTACIÓN CSV ========================
void Widget::exportarCSV()
{
    if (registros.isEmpty()) {
        return;
    }

    const QString nombre =
        "invernadero_" +
        QDateTime::currentDateTime()
            .toString("yyyyMMdd_HHmmss") +
        ".csv";

    QString ruta = QFileDialog::getSaveFileName(
        this,
        "Guardar lecturas de la sesión",
        nombre,
        "Archivo CSV (*.csv)"
        );

    if (ruta.isEmpty()) {
        return;
    }

    if (!ruta.endsWith(".csv", Qt::CaseInsensitive)) {
        ruta += ".csv";
    }

    QSaveFile archivo(ruta);

    if (!archivo.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(
            this,
            "Exportación",
            archivo.errorString()
            );
        return;
    }

    const QByteArray cabecera =
        "fecha_hora,"
        "humedad_amb_pct,"
        "temperatura_amb_c,"
        "presion_hpa,"
        "humedad_suelo_pct,"
        "temperatura_suelo_c,"
        "ec_us_cm,"
        "ph,"
        "nitrogeno_mg_kg,"
        "fosforo_mg_kg,"
        "potasio_mg_kg\n";

    bool correcto =
        archivo.write(cabecera) == cabecera.size();

    const auto numero = [](double valor) {
        return QLocale::c().toString(valor, 'f', 2);
    };

    if (correcto) {
        for (const Lectura &r : registros) {
            const QString fila =
                r.fecha.toString(Qt::ISODateWithMs) + "," +
                numero(r.humedad) + "," +
                numero(r.temperatura) + "," +
                numero(r.presion) + "," +
                numero(r.humedadSuelo) + "," +
                numero(r.temperaturaSuelo) + "," +
                numero(r.ec) + "," +
                numero(r.ph) + "," +
                numero(r.nitrogeno) + "," +
                numero(r.fosforo) + "," +
                numero(r.potasio) + "\n";

            const QByteArray bytes = fila.toUtf8();

            if (archivo.write(bytes) != bytes.size()) {
                correcto = false;
                break;
            }
        }
    }

    if (!correcto) {
        const QString detalle = archivo.errorString();

        archivo.cancelWriting();

        QMessageBox::warning(
            this,
            "Exportación",
            "No se pudo escribir el archivo: " + detalle
            );

        return;
    }

    if (!archivo.commit()) {
        QMessageBox::warning(
            this,
            "Exportación",
            "No se pudo confirmar el archivo: " +
                archivo.errorString()
            );

        return;
    }

    QMessageBox::information(
        this,
        "CSV guardado",
        QString("Se guardaron %1 registros.\n%2")
            .arg(registros.size())
            .arg(ruta)
        );
}

void Widget::marcarDatosNoVigentes()
{
    Gauge *gauges[] = {
        gaugeHumedadAmb,
        gaugeTemperaturaAmb,
        gaugePresion,
        gaugeHumedadSuelo,
        gaugeTemperaturaSuelo,
        gaugeEC,
        gaugePH,
        gaugeN,
        gaugeP,
        gaugeK
    };

    for (Gauge *gauge : gauges) {
        gauge->setVigente(false);
    }
}

void Widget::actualizarVigencia()
{
    if (!serial->isOpen()) {
        return;
    }

    if (tieneLectura &&
        edadLectura.isValid() &&
        edadLectura.elapsed() > 7000) {

        tieneLectura = false;

        marcarDatosNoVigentes();

        estadoConexion->setText(
            "Conectado · sin muestra completa reciente"
            );
    }
}