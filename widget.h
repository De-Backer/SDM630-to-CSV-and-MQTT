#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QtMqtt/QMqttClient>

#include <QModbusDataUnit>
#include <QModbusRtuSerialClient>
#include <QSerialPort>
#include <QDebug>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QSettings>

#include <QtSerialPort/QSerialPortInfo>

typedef union
{
    int i;
    float f;
} U;

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
signals:
    void stuur_data_van_meter(int modbus_nr,QString data);

private slots:
    void haal_data();
    void readReady();
    void set_mqtt_settings();

    void on_pushButton_retive_clicked(bool checked);
    void on_lineEdit_progam_Path_editingFinished();
    void on_pushButton_clicked();

private:

    double KWh_L1_in,KWh_L3_in,KWh_L1_out,KWh_L3_out;//ofset bias
    QString progam_data_ini;
    void set_settings();
    QStringList modbus_register_list();
    QString data_van_meter_csv;

    QString progam_Path=QDir::currentPath();
    QString versie="2023-12-25";

    Ui::Widget *ui;
    QModbusClient *modbusDevice;
    QTimer *timer;
    QMqttClient *m_client;
    void publish_mqtt(QString topic, QByteArray data);
};
#endif // WIDGET_H
