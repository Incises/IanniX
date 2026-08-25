/*
    This file is part of IanniX, a graphical real-time open-source sequencer for digital art
    Copyright (C) 2010-2015 - IanniX Association (https://www.iannix.org/)
    Copyright (C) 2025-2026 - Hypar.XYZ (https://iannix.hypar.xyz/)

    This file was originally written by Guillaume Jacquemin.
    
    IanniX is a free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "interfaceserial.h"
#include "ui_interfaceserial.h"

InterfaceSerial::InterfaceSerial(QWidget *parent) :
    NetworkInterface(parent),
    ui(new Ui::InterfaceSerial) {
    ui->setupUi(this);
    port = 0;
    connect(ui->examples, &QAbstractButton::released, this, &InterfaceSerial::openExamples);

    baudrateEnum << 110 << 300 << 600 << 1200 << 2400
                 << 4800 << 9600 << 19200 << 38400 << 57600 << 115200;

    databitsEnum << QSerialPort::Data5 << QSerialPort::Data6
                 << QSerialPort::Data7 << QSerialPort::Data8;

    parityEnum   << QSerialPort::NoParity   << QSerialPort::OddParity
                 << QSerialPort::EvenParity << QSerialPort::SpaceParity;

    stopbitsEnum << QSerialPort::OneStop << QSerialPort::TwoStop;

    flowEnum     << QSerialPort::NoFlowControl
                 << QSerialPort::HardwareControl
                 << QSerialPort::SoftwareControl;

    //Bind UI widgets to persistent settings
    enable.setAction(ui->enable, "interfaceSerialEnable");

    portName  .setAction(ui->portCombo,   "interfaceSerialPortname");
    portBaud  .setAction(ui->baudCombo,   "interfaceSerialBaud");
    portBits  .setAction(ui->bitsCombo,   "interfaceSerialBits");
    portParity.setAction(ui->parityCombo, "interfaceSerialParity");
    portStop  .setAction(ui->stopCombo,   "interfaceSerialStop");
    portFlow  .setAction(ui->flowCombo,   "interfaceSerialFlow");
    connect(&portName,   &UiString::triggered, this, &InterfaceSerial::portChanged);
    connect(&portBaud,   &UiReal::triggered,   this, &InterfaceSerial::portChanged);
    connect(&portBits,   &UiReal::triggered,   this, &InterfaceSerial::portChanged);
    connect(&portParity, &UiReal::triggered,   this, &InterfaceSerial::portChanged);
    connect(&portStop,   &UiReal::triggered,   this, &InterfaceSerial::portChanged);
    connect(&portFlow,   &UiReal::triggered,   this, &InterfaceSerial::portChanged);

    portBaud   = 10;
    portBits   = 3;
    portParity = 0;
    portStop   = 0;
    portFlow   = 0;

    connect(ui->enable, &QAbstractButton::toggled, this, &InterfaceSerial::portChanged);
    //Defaults: 115200 baud, 8-N-1, no flow control (set via combo indices above)

    timerEvent(0);
    startTimer(5000);
}

void InterfaceSerial::timerEvent(QTimerEvent *) {
    const auto portsInfo = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &portInfo : portsInfo)
        if (ui->portCombo->findText(portInfo.portName()) < 0)
            ui->portCombo->addItem(portInfo.portName());
}

void InterfaceSerial::portChanged() {
    if(enable) {
        if(port) {
            port->close();
            delete port;
        }
        port = new QSerialPort(portName, this);
        if(port) {
            port->setBaudRate(baudrateEnum.at(portBaud.val()));
            port->setFlowControl(flowEnum .at(portFlow.val()));
            port->setParity(parityEnum    .at(portParity.val()));
            port->setDataBits(databitsEnum.at(portBits.val()));
            port->setStopBits(stopbitsEnum.at(portStop.val()));
            port->open(QIODevice::ReadWrite);

            connect(port, &QSerialPort::readyRead, this, &InterfaceSerial::parse);

            if(port->isOpen())  ui->portCombo->setStyleSheet(ihmFeedbackOk);
            else                ui->portCombo->setStyleSheet(ihmFeedbackNok);
        }
    }
    else {
        if((port) && (port->isOpen()))
            port->close();
    }
}

void InterfaceSerial::parse() {
    QByteArray receptionTmp;
    int a = port->bytesAvailable();
    receptionTmp.resize(a);
    port->read(receptionTmp.data(), receptionTmp.size());

    if(enable) {
        reception.append(receptionTmp);
        bool nextMessage = true;
        while(nextMessage) {
            nextMessage = false;
            quint16 index = 0;
            QString command;
            while((index < reception.size()) && (!nextMessage)) {
                if(reception.at(index) == COMMAND_END_BYTE) {
                    reception = reception.remove(0, index);
                    index = 0;
                    command = command.remove('\n');
                    command = command.remove('\r');

                    if(command != "")
                        MessageManager::incomingMessage(MessageIncomming("serial", portName, 0, "", command, command.split(" ", Qt::SkipEmptyParts)));
                    command = "";
                }
                else
                    command += reception.at(index);
                index++;
            }
        }
    }
}

bool InterfaceSerial::send(const Message &message, QStringList *messageSent) {
    if(!enable)
        return false;

    //Send ASCII message terminated by COMMAND_END_BYTE over serial
    port->write(message.getAsciiMessage() + COMMAND_END_BYTE);

    //Log in console
    MessageManager::logSend(message, messageSent);

    return true;
}

InterfaceSerial::~InterfaceSerial() {
    delete ui;
}
