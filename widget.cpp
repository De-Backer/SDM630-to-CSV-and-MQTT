#include "widget.h"
#include "./ui_widget.h"

static const char blankString[] = QT_TRANSLATE_NOOP("SettingsDialog", "N/A");

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    progam_data_ini=QDir::currentPath()+"/settings_data_File.ini";

    QSettings settings(progam_data_ini,QSettings::IniFormat);
    settings.beginGroup("progam");

    //comboBox_SerialPort
    QString description;
    QString manufacturer;
    QString serialNumber;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        QStringList list;
        description = info.description();
        manufacturer = info.manufacturer();
        serialNumber = info.serialNumber();
        list << info.portName()
             << (!description.isEmpty() ? description : blankString)
             << (!manufacturer.isEmpty() ? manufacturer : blankString)
             << (!serialNumber.isEmpty() ? serialNumber : blankString)
             << info.systemLocation()
             << (info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : blankString)
             << (info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : blankString);

        ui->comboBox_SerialPort->addItem(list.first(), list);
    }
    ui->comboBox_SerialPort->setCurrentIndex(settings.value("SerialPort",0).toInt());

    //lineEdit_progam_Path
    ui->lineEdit_progam_Path->setText(progam_Path);

    //comboBox_Parity
    ui->comboBox_Parity->addItem("No",QSerialPort::NoParity);
    ui->comboBox_Parity->addItem("Even",QSerialPort::EvenParity);
    ui->comboBox_Parity->addItem("Odd",QSerialPort::OddParity);
    ui->comboBox_Parity->setCurrentIndex(settings.value("Parity",0).toInt());

    //comboBox_BaudRate
    ui->comboBox_BaudRate->addItem("2400",QSerialPort::Baud2400);
    ui->comboBox_BaudRate->addItem("4800",QSerialPort::Baud4800);
    ui->comboBox_BaudRate->addItem("9600",QSerialPort::Baud9600);
    ui->comboBox_BaudRate->addItem("19200",QSerialPort::Baud19200);
    ui->comboBox_BaudRate->addItem("38400",QSerialPort::Baud38400);
    ui->comboBox_BaudRate->setCurrentIndex(settings.value("BaudRate",4).toInt());

    //comboBox_StopBits
    ui->comboBox_StopBits->addItem("One",QSerialPort::OneStop);
    ui->comboBox_StopBits->addItem("Two",QSerialPort::TwoStop);
    ui->comboBox_StopBits->setCurrentIndex(settings.value("StopBits",0).toInt());

    //spinBox_Timeout
    ui->spinBox_Timeout->setMaximum(10000);
    ui->spinBox_Timeout->setMinimum(10);
    ui->spinBox_Timeout->setValue(settings.value("Timeout",500).toInt());

    //spinBox_NumberOfRetries
    ui->spinBox_NumberOfRetries->setMaximum(10);
    ui->spinBox_NumberOfRetries->setMinimum(0);
    ui->spinBox_NumberOfRetries->setValue(settings.value("NumberOfRetries",3).toInt());

    //spinBox_modbus_adres
    ui->spinBox_modbus_adres->setMaximum(255);
    ui->spinBox_modbus_adres->setMinimum(1);
    ui->spinBox_modbus_adres->setValue(settings.value("modbus_adres",1).toInt());

    //spinBox_Interval
    ui->spinBox_Interval->setMaximum(100000);
    ui->spinBox_Interval->setMinimum(1);
    ui->spinBox_Interval->setValue(settings.value("Interval",1).toInt());

    //comboBox_value
    QStringList list = modbus_register_list();
    int nr_list=0;
    while (nr_list<list.size()) {
        ui->comboBox_value->addItem(list.at(nr_list), nr_list);
        nr_list++;
    }
    ui->comboBox_value->setCurrentIndex(settings.value("value",26).toInt());

    //lineEdit_offset
    ui->lineEdit_ofset->setText(settings.value("ofset","0").toString());
    ui->lineEdit_bias->setText(settings.value("bias","0").toString());
    ui->lineEdit_bias_uit->setText(settings.value("bias_uit","0").toString());
    ui->lineEdit_doel->setText(settings.value("doel","0").toString());

    //checkBox_auto
    if(settings.value("auto","false").toString()=="true"){
        ui->checkBox_auto->setChecked(true);
    }else {
        ui->checkBox_auto->setChecked(false);
    }

    //mqtt
    ui->lineEdit_Host->setText(settings.value("Host","127.0.0.1").toString());
    ui->spinBox_Port->setValue(settings.value("Port",1883).toInt());
    ui->lineEdit_Username->setText(settings.value("Username","").toString());
    ui->lineEdit_Password->setText(settings.value("Password","").toString());
    ui->lineEdit_Topic->setText(settings.value("Topic","/new_teller").toString());
    m_client = new QMqttClient(this);
    connect(m_client, &QMqttClient::connected, this, &Widget::set_mqtt_settings);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(haal_data()));

    if(ui->checkBox_auto->isChecked())
        on_pushButton_retive_clicked(true);

}

Widget::~Widget()
{
    if(ui->pushButton_retive->isChecked())
    {
        modbusDevice->disconnectDevice();
        delete modbusDevice;
    }
    delete ui;
    qApp->quit();
}

void Widget::on_pushButton_retive_clicked(bool checked)
{
    ui->label_value->setText("----");
    if(checked){
        set_settings();
        modbusDevice = new QModbusRtuSerialClient(this);
        connect(modbusDevice, &QModbusClient::errorOccurred, [this](QModbusDevice::Error)
                {
                    qDebug()<<modbusDevice->errorString();
                });
        modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter,ui->comboBox_SerialPort->currentText());
        modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter,ui->comboBox_Parity->currentData());
        modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,ui->comboBox_BaudRate->currentData());
        modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,QSerialPort::Data8);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,ui->comboBox_StopBits->currentData());
        modbusDevice->setTimeout(ui->spinBox_Timeout->value());
        modbusDevice->setNumberOfRetries(ui->spinBox_NumberOfRetries->value());


        if (!modbusDevice->connectDevice())
        {
            qDebug() << tr("Connect failed: ") << modbusDevice->errorString();
            ui->pushButton_retive->setChecked(false);
        } else {
            timer->start(ui->spinBox_Interval->value()*1000);
            ui->pushButton_retive->setText("disconnect");
            ui->lineEdit_progam_Path->setEnabled(false);
            ui->comboBox_SerialPort->setEnabled(false);
            ui->comboBox_Parity->setEnabled(false);
            ui->comboBox_BaudRate->setEnabled(false);
            ui->comboBox_StopBits->setEnabled(false);
            ui->spinBox_modbus_adres->setEnabled(false);
            ui->spinBox_Interval->setEnabled(false);
            ui->spinBox_NumberOfRetries->setEnabled(false);
            ui->spinBox_Timeout->setEnabled(false);
        }
    } else {
        timer->stop();
        if (modbusDevice)
            modbusDevice->disconnectDevice();
        delete modbusDevice;
        ui->pushButton_retive->setText("connect");
        ui->lineEdit_progam_Path->setEnabled(true);
        ui->comboBox_SerialPort->setEnabled(true);
        ui->comboBox_Parity->setEnabled(true);
        ui->comboBox_BaudRate->setEnabled(true);
        ui->comboBox_StopBits->setEnabled(true);
        ui->spinBox_modbus_adres->setEnabled(true);
        ui->spinBox_Interval->setEnabled(true);
        ui->spinBox_NumberOfRetries->setEnabled(true);
        ui->spinBox_Timeout->setEnabled(true);
    }

}

void Widget::haal_data()
{
    //ofset
    KWh_L1_in=0;KWh_L3_in=0;KWh_L1_out=0;KWh_L3_out=0;
    ui->label_Kwh->setText("----");

    ui->label_value->setText("----");
    ui->pushButton_retive->setDisabled(true);
    quint16 modbus_data_aantal=76;
    //modbus_register     000 078 156 234 312
    //modbus_data_aantal  076 076 076 076 076
    //bereik              078 156 234 312 388
    //of
    //modbus_register     000 078 200 334
    //modbus_data_aantal  076 030 070 046
    //bereik              078 108 270 382

//#define llll
#ifdef llll
    for (int modbus_register = 0; modbus_register < 382; modbus_register+=modbus_data_aantal+2)
    {
        qDebug() <<this<<modbus_register<<modbus_data_aantal;
        if (auto *reply = modbusDevice->sendReadRequest(
                QModbusDataUnit
                (
                    QModbusDataUnit::InputRegisters,
                    /*register*/ modbus_register,
                    /*data_aantal*/ modbus_data_aantal ),
                /* adres */ ui->spinBox_modbus_adres->value() )
            )
        {
            if (!reply->isFinished())
                connect(reply, &QModbusReply::finished, this, &Widget::readReady);
            else
                delete reply; // broadcast replies return immediately
        } else {
            qDebug() <<tr("Read error: ") << modbusDevice->errorString();
        }
    }
#else
    int modbus_register = 0;

    qDebug() <<this<<modbus_register<<modbus_data_aantal;
    if (auto *reply = modbusDevice->sendReadRequest(
            QModbusDataUnit
            (
                QModbusDataUnit::InputRegisters,
                /*register*/ modbus_register,
                /*data_aantal*/ modbus_data_aantal ),
            /* adres */ ui->spinBox_modbus_adres->value() )
        )
    {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &Widget::readReady);
        else
            delete reply; // broadcast replies return immediately
    } else {
        qDebug() <<tr("Read error: ") << modbusDevice->errorString();
    }
    //--------------------
    modbus_data_aantal=30;
    modbus_register=78;
    qDebug() <<this<<modbus_register<<modbus_data_aantal;
    if (auto *reply = modbusDevice->sendReadRequest(
            QModbusDataUnit
            (
                QModbusDataUnit::InputRegisters,
                /*register*/ modbus_register,
                /*data_aantal*/ modbus_data_aantal ),
            /* adres */ ui->spinBox_modbus_adres->value() )
        )
    {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &Widget::readReady);
        else
            delete reply; // broadcast replies return immediately
    } else {
        qDebug() <<tr("Read error: ") << modbusDevice->errorString();
    }
    //-----------------
    modbus_data_aantal=70;
    modbus_register=200;
    qDebug() <<this<<modbus_register<<modbus_data_aantal;
    if (auto *reply = modbusDevice->sendReadRequest(
            QModbusDataUnit
            (
                QModbusDataUnit::InputRegisters,
                /*register*/ modbus_register,
                /*data_aantal*/ modbus_data_aantal ),
            /* adres */ ui->spinBox_modbus_adres->value() )
        )
    {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &Widget::readReady);
        else
            delete reply; // broadcast replies return immediately
    } else {
        qDebug() <<tr("Read error: ") << modbusDevice->errorString();
    }
    //---------------------

    modbus_data_aantal=48;
    modbus_register=334;
    qDebug() <<this<<modbus_register<<modbus_data_aantal;
    if (auto *reply = modbusDevice->sendReadRequest(
            QModbusDataUnit
            (
                QModbusDataUnit::InputRegisters,
                /*register*/ modbus_register,
                /*data_aantal*/ modbus_data_aantal ),
            /* adres */ ui->spinBox_modbus_adres->value() )
        )
    {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &Widget::readReady);
        else
            delete reply; // broadcast replies return immediately
    } else {
        qDebug() <<tr("Read error: ") << modbusDevice->errorString();
    }
#endif



}

void Widget::readReady()
{

    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
        return;

    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();
        qDebug() << "data van nr: " << reply->serverAddress() << unit.startAddress() << unit.valueCount();


        QDir directory;
        if(!directory.exists(progam_Path+QDateTime::currentDateTimeUtc().toString("/yyyy/MM/")))
        {
            if(!directory.mkpath(progam_Path+QDateTime::currentDateTimeUtc().toString("/yyyy/MM/")))
                return;
        }
        QDir::setCurrent(progam_Path+QDateTime::currentDateTimeUtc().toString("/yyyy/MM/"));
        QFile file(QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd")+".csv");
        if (!file.open(QIODevice::Append | QIODevice::Text))
            return;
        QTextStream out(&file);

        if(file.size()<10){
            qDebug()<<" start van file ";
            out<<"versie="<<versie;
            out<<"\nDateTime";

            QStringList list = modbus_register_list();
            for (int var = 0; var < list.size(); ++var) {
                if(!QString(list.at(var)).contains("Parameter_")){
                    out<<";"+list.at(var);
                }
            }

            out<<"\n[yyyy-MM-dd hh:mm:ss]"
                <<";V"
                <<";V"
                <<";V"
                <<";A"
                <<";A"
                <<";A"
                <<";W"
                <<";W"
                <<";W"
                <<";VA"
                <<";VA"
                <<";VA"
                <<";VAR"
                <<";VAR"
                <<";VAR"
                <<";power factor"
                <<";power factor"
                <<";power factor"
                <<";°"
                <<";°"
                <<";°"
                <<";V"
                //                                      <<";"
                <<";A"
                <<";A"
                //                                      <<";"
                <<";W"
                //                                      <<";"
                <<";VA"
                //                                      <<";"
                <<";VAR"
                <<";power factor"
                //                                      <<";"
                <<";°"
                //                                      <<";"
                <<";Hz"
                <<";Wh"
                <<";Wh"
                <<";VArh"
                <<";VArh"
                <<";VAh"
                <<";Ah"
                <<";W"
                <<";W"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                <<";VA"
                <<";VA"
                <<";A"
                <<";A"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                <<";V"
                <<";V"
                <<";V"
                <<";V"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                <<";A"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                <<";V THD"
                <<";V THD"
                <<";V THD"
                <<";A THD"
                <<";A THD"
                <<";A THD"
                //                                      <<";"
                <<";V THD."
                <<";A THD."
                //                                      <<";"
                <<";power factor"
                //                                      <<";"
                <<";A"
                <<";A"
                <<";A"
                <<";A"
                <<";A"
                <<";A"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                //                                      <<";"
                <<";V THD."
                <<";V THD."
                <<";V THD."
                <<";V THD."
                <<";kwh"
                <<";kvarh"
                <<";kwh"
                <<";kwh"
                <<";kWh"
                <<";kWh"
                <<";kwh"
                <<";kwh"
                <<";kwh"
                <<";kwh"
                <<";kwh"
                <<";kvarh"
                <<";kvarh"
                <<";kvarh"
                <<";kvarh"
                <<";kvarh"
                <<";kvarh"
                <<";kvarh"
                <<";kvarh"
                <<";kvarh";
        }
        if(unit.startAddress()==0){
            data_van_meter_csv="\n"+QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss")+";";
        }
        int register_meter=unit.startAddress();

        for (uint i = 0; i < (unit.valueCount()+1); i+=2, register_meter+=2)
        {

            quint32 var1=unit.value(i);
            quint32 var2=unit.value(i+1);

            var1=var1<<16;
            var1+=var2;

            U u;
            u.i=var1;

            float f_data=u.f;
            if((register_meter/2)==ui->comboBox_value->currentData())
            {
                ui->label_value->setText(QString::number(f_data));
            }
            if(register_meter<382){
                if(!QString(modbus_register_list().at(register_meter/2)).contains("Parameter_"))
                {
                    //geen "Parameter_"
                    //is data
                    publish_mqtt(QString(modbus_register_list().at(register_meter/2)),QByteArray::number(f_data));
                    data_van_meter_csv.append(QString::number(f_data).replace(".",",")+";");

                    //ofset
                    //37 - 38 = verbruik
                    //173 L1 in
                    //175 L3 in
                    //176 L1 ex
                    //178 L3 ex


                    if(register_meter/2==173){
                        KWh_L1_in=f_data;

                    }
                    if(register_meter/2==175){
                        KWh_L3_in=f_data;

                    }
                    if(register_meter/2==176){
                        KWh_L1_out=f_data;

                    }
                    if(register_meter/2==178){
                        KWh_L3_out=f_data;

                        //bias meter
                        KWh_L1_in=(KWh_L1_in+KWh_L3_in)*ui->lineEdit_bias->text().toDouble();
                        KWh_L1_out=(KWh_L1_out+KWh_L3_out)*ui->lineEdit_bias_uit->text().toDouble();

                        double f_temp_data=KWh_L1_in-KWh_L1_out+ui->lineEdit_ofset->text().toDouble();
                        qDebug() << this <<f_temp_data<<KWh_L1_in<<KWh_L1_out<<ui->lineEdit_ofset->text().toDouble()<<ui->lineEdit_bias->text().toFloat();

                        ui->label_Kwh->setText(QString::number(f_temp_data));

                        f_temp_data=f_temp_data+ui->lineEdit_doel->text().toDouble();
                        ui->label_stand->setText(QString::number(f_temp_data));
                    }
                    //                    if(register_meter/2==36){
                    //                        KWh_in=f_data;

                    //                    }
                    //                    if(register_meter/2==37){
                    //                        KWh_out=f_data;

                    //                        //bias meter
                    //                        KWh_in=KWh_in*ui->lineEdit_bias->text().toDouble();
                    //                        KWh_out=KWh_out*ui->lineEdit_bias_uit->text().toDouble();

                    //                        double f_temp_data=KWh_in-KWh_out+ui->lineEdit_ofset->text().toDouble();
                    //                        qDebug() << this <<f_temp_data<<KWh_in<<KWh_out<<ui->lineEdit_ofset->text().toDouble()<<ui->lineEdit_bias->text().toFloat();

                    //                        ui->label_Kwh->setText(QString::number(f_temp_data));

                    //                        f_temp_data=f_temp_data+ui->lineEdit_doel->text().toDouble();
                    //                        ui->label_stand->setText(QString::number(f_temp_data));

                    //                    }

                    //--ofset

                }
                //qDebug() << this << register_meter<< register_meter/2 << i << modbus_register_list().at(register_meter/2);
            }

        }
        if(register_meter>379){
            ui->pushButton_retive->setEnabled(true);
            qDebug() << this << data_van_meter_csv;
            out << data_van_meter_csv;
            //zend data naar client
            emit stuur_data_van_meter(reply->serverAddress(),data_van_meter_csv);
        }
        file.close();


    } else if (reply->error() == QModbusDevice::ProtocolError) {
        ui->pushButton_retive->setEnabled(true);
        qDebug() << "Read response error: " << reply->errorString()
                 << "(Mobus exception: " << reply->rawResult().exceptionCode();
    } else {
        ui->pushButton_retive->setEnabled(true);
        qDebug() << "Read response error: " << reply->errorString()
                 << "(code:" << reply->error();
    }

    reply->deleteLater();
}

void Widget::set_mqtt_settings()
{
    QSettings settings(progam_data_ini,QSettings::IniFormat);
    settings.beginGroup("progam");
    settings.setValue("Host",ui->lineEdit_Host->text());
    settings.setValue("Port",ui->spinBox_Port->value());
    settings.setValue("Username",ui->lineEdit_Username->text());
    settings.setValue("Password",ui->lineEdit_Password->text());
    settings.setValue("Topic",ui->lineEdit_Topic->text());
    settings.endGroup();
}

void Widget::on_lineEdit_progam_Path_editingFinished()
{
    progam_Path=ui->lineEdit_progam_Path->text();
}

void Widget::set_settings()
{
    QSettings settings(progam_data_ini,QSettings::IniFormat);
    settings.beginGroup("progam");
    settings.setValue("SerialPort",ui->comboBox_SerialPort->currentIndex());
    settings.setValue("SerialPort",ui->comboBox_SerialPort->currentIndex());
    settings.setValue("Parity",ui->comboBox_Parity->currentIndex());
    settings.setValue("BaudRate",ui->comboBox_BaudRate->currentIndex());
    settings.setValue("StopBits",ui->comboBox_StopBits->currentIndex());
    settings.setValue("Timeout",ui->spinBox_Timeout->value());
    settings.setValue("NumberOfRetries",ui->spinBox_NumberOfRetries->value());
    settings.setValue("modbus_adres",ui->spinBox_modbus_adres->value());
    settings.setValue("Interval",ui->spinBox_Interval->value());
    settings.setValue("value",ui->comboBox_value->currentIndex());
    settings.setValue("ofset",ui->lineEdit_ofset->text());
    settings.setValue("bias",ui->lineEdit_bias->text());
    settings.setValue("bias_uit",ui->lineEdit_bias_uit->text());
    settings.setValue("doel",ui->lineEdit_doel->text());
    if(ui->checkBox_auto->isChecked()){
        settings.setValue("auto","true");
    }else {
        settings.setValue("auto","false");
    }
    settings.endGroup();
}

QStringList Widget::modbus_register_list()
{
    QStringList list;
    list<<"Phase 1 line to neutral volts."
         <<"Phase 2 line to neutral volts."
         <<"Phase 3 line to neutral volts."
         <<"Phase 1 current."
         <<"Phase 2 current."
         <<"Phase 3 current."
         <<"Phase 1 power."
         <<"Phase 2 power."
         <<"Phase 3 power."
         <<"Phase 1 volt amps."
         <<"Phase 2 volt amps."
         <<"Phase 3 volt amps."
         <<"Phase 1 volt amps reactive."
         <<"Phase 2 volt amps reactive."
         <<"Phase 3 volt amps reactive."
         <<"Phase 1 power factor (1)."
         <<"Phase 2 power factor (1)."
         <<"Phase 3 power factor (1)."
         <<"Phase 1 phase angle."
         <<"Phase 2 phase angle."
         <<"Phase 3 phase angle."
         <<"Average line to neutral volts."
         //46
         <<"Parameter_23"
         <<"Average line current."
         <<"Sum of line currents."
         //52
         <<"Parameter_26"
         <<"Total system power."
         //56
         <<"Parameter_28"
         <<"Total system volt amps."
         //60
         <<"Parameter_30"
         <<"Total system VAr."
         <<"Total system power factor (1)."
         //66
         <<"Parameter_33"
         <<"Total system phase angle."
         //70
         <<"Parameter_35"
         <<"Frequency of supply voltages."
         <<"Import Wh since last reset (2)."
         <<"Export Wh since last reset (2)."
         <<"Import VArh since last reset (2)."
         <<"Export VArh since last reset (2)."
         <<"VAh since last reset (2)."
         <<"Ah since last reset(3)."
         <<"Total system power demand (4)."
         <<"Maximum total system power demand (4)."
         //90-100
         <<"Parameter_45"
         <<"Parameter_46"
         <<"Parameter_47"
         <<"Parameter_48"
         <<"Parameter_49"
         <<"Parameter_50"
         <<"Total system VA demand."
         <<"Maximum total system VA demand."
         <<"Neutral current demand."
         <<"Maximum neutral current demand."
         //110-200
         <<"Parameter_55"
         <<"Parameter_56"
         <<"Parameter_57"
         <<"Parameter_58"
         <<"Parameter_59"
         <<"Parameter_60"
         <<"Parameter_61"
         <<"Parameter_62"
         <<"Parameter_63"
         <<"Parameter_64"
         <<"Parameter_65"
         <<"Parameter_66"
         <<"Parameter_67"
         <<"Parameter_68"
         <<"Parameter_69"
         <<"Parameter_70"
         <<"Parameter_71"
         <<"Parameter_72"
         <<"Parameter_73"
         <<"Parameter_74"
         <<"Parameter_75"
         <<"Parameter_76"
         <<"Parameter_77"
         <<"Parameter_78"
         <<"Parameter_79"
         <<"Parameter_80"
         <<"Parameter_81"
         <<"Parameter_82"
         <<"Parameter_83"
         <<"Parameter_84"
         <<"Parameter_85"
         <<"Parameter_86"
         <<"Parameter_87"
         <<"Parameter_88"
         <<"Parameter_89"
         <<"Parameter_90"
         <<"Parameter_91"
         <<"Parameter_92"
         <<"Parameter_93"
         <<"Parameter_94"
         <<"Parameter_95"
         <<"Parameter_96"
         <<"Parameter_97"
         <<"Parameter_98"
         <<"Parameter_99"
         <<"Parameter_100"
         <<"Line 1 to Line 2 volts."
         <<"Line 2 to Line 3 volts."
         <<"Line 3 to Line 1 volts."
         <<"Average line to line volts."
         //210-224
         <<"Parameter_105"
         <<"Parameter_106"
         <<"Parameter_107"
         <<"Parameter_108"
         <<"Parameter_109"
         <<"Parameter_110"
         <<"Parameter_111"
         <<"Parameter_112"
         <<"Neutral current."
         //228-234
         <<"Parameter_114"
         <<"Parameter_115"
         <<"Parameter_116"
         <<"Parameter_117"
         <<"Phase 1 L/N volts THD"
         <<"Phase 2 L/N volts THD"
         <<"Phase 3 L/N volts THD"
         <<"Phase 1 Current THD"
         <<"Phase 2 Current THD"
         <<"Phase 3 Current THD"
         //248
         <<"Parameter_124"
         <<"Average line to neutral volts THD."
         <<"Average line current THD."
         //254
         <<"Parameter_127"
         <<"Total system power factor (5)."
         //258
         <<"Parameter_129"
         <<"Phase 1 current demand."
         <<"Phase 2 current demand."
         <<"Phase 3 current demand."
         <<"Maximum phase 1 current demand."
         <<"Maximum phase 2 current demand."
         <<"Maximum phase 3 current demand."
         //272-334
         <<"Parameter_136"
         <<"Parameter_137"
         <<"Parameter_138"
         <<"Parameter_139"
         <<"Parameter_140"
         <<"Parameter_141"
         <<"Parameter_142"
         <<"Parameter_143"
         <<"Parameter_144"
         <<"Parameter_145"
         <<"Parameter_146"
         <<"Parameter_147"
         <<"Parameter_148"
         <<"Parameter_149"
         <<"Parameter_150"
         <<"Parameter_151"
         <<"Parameter_152"
         <<"Parameter_153"
         <<"Parameter_154"
         <<"Parameter_155"
         <<"Parameter_156"
         <<"Parameter_157"
         <<"Parameter_158"
         <<"Parameter_159"
         <<"Parameter_160"
         <<"Parameter_161"
         <<"Parameter_162"
         <<"Parameter_163"
         <<"Parameter_164"
         <<"Parameter_165"
         <<"Parameter_166"
         <<"Parameter_167"
         <<"Line 1 to line 2 volts THD."
         <<"Line 2 to line 3 volts THD."
         <<"Line 3 to line 1 volts THD."
         <<"Average line to line volts THD."
         <<"Total kwh"
         <<"Total kvarh"
         <<"L1 import kwh"
         <<"L2 import kwh"
         <<"L3 import kWh"
         <<"L1 export kWh"
         <<"L2 export kwh"
         <<"L3 export kWh"
         <<"L1 total kwh"
         <<"L2 total kWh"
         <<"L3 total kwh"
         <<"L1 import kvarh"
         <<"L2 import kvarh"
         <<"L3 import kvarh"
         <<"L1 export kvarh"
         <<"L2 export kvarh"
         <<"L3 export kvarh"
         <<"L1 total kvarh"
         <<"L2 total kvarh"
         <<"L3 total kvarh";
    return list;
}

void Widget::publish_mqtt(QString topic, QByteArray data)
{
    if(m_client->publish(ui->lineEdit_Topic->text()+"/"+topic,data,0,false) == -1){
        m_client->disconnectFromHost();
        on_pushButton_clicked();
    }
}


void Widget::on_pushButton_clicked()
{
    if (m_client->state() == QMqttClient::Disconnected) {
        ui->lineEdit_Host->setEnabled(false);
        ui->spinBox_Port->setEnabled(false);
        ui->lineEdit_Username->setEnabled(false);
        ui->lineEdit_Password->setEnabled(false);
        ui->lineEdit_Topic->setEnabled(false);
        ui->pushButton->setText(tr("Disconnect"));

        m_client->setHostname(ui->lineEdit_Host->text());
        m_client->setPort(static_cast<quint16>(ui->spinBox_Port->value()));
        m_client->setUsername(ui->lineEdit_Username->text());
        m_client->setPassword(ui->lineEdit_Password->text());
        m_client->connectToHost();
    } else {
        ui->lineEdit_Host->setEnabled(true);
        ui->spinBox_Port->setEnabled(true);
        ui->lineEdit_Username->setEnabled(true);
        ui->lineEdit_Password->setEnabled(true);
        ui->lineEdit_Topic->setEnabled(true);
        ui->pushButton->setText(tr("Connect"));
        m_client->disconnectFromHost();
    }
}

