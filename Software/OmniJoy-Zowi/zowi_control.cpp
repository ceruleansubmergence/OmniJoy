#include "zowi_control.h"
#include "ui_zowi_control.h"

ZOWI_CONTROL::ZOWI_CONTROL(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ZOWI_CONTROL)
{
    ui->setupUi(this);

    sendToAddress = QHostAddress("192.168.5.255");
    udpOutPort = 15300;

}

ZOWI_CONTROL::~ZOWI_CONTROL()
{
    delete ui;
}


void ZOWI_CONTROL::keyReleaseEvent( QKeyEvent *event) {

    switch ( event->key() ) {
        case Qt::Key_1:
            ui->NLLeft->setChecked(false);
            break;
        case Qt::Key_2:
            ui->NLRight->setChecked(false);
            break;
        case Qt::Key_3:
            ui->NLUp->setChecked(false);
            break;
        case Qt::Key_4:
            ui->NLDown->setChecked(false);
            break;
        case Qt::Key_5:
            ui->NLCenter->setChecked(false);
            break;
        case Qt::Key_6:
            ui->NRLeft->setChecked(false);
            break;
        case Qt::Key_7:
            ui->NRRight->setChecked(false);
            break;
        case Qt::Key_8:
            ui->NRUp->setChecked(false);
            break;
        case Qt::Key_9:
            ui->NRDown->setChecked(false);
            break;
        case Qt::Key_0:
            ui->NRCenter->setChecked(false);
            break;
        case Qt::Key_W:
            ui->ALUp->setChecked(false);
            break;
        case Qt::Key_A:
            ui->ALLeft->setChecked(false);
            break;
        case Qt::Key_S:
            ui->ALDown->setChecked(false);
            break;
        case Qt::Key_D:
            ui->ALRight->setChecked(false);
            break;
        case Qt::Key_O:
            ui->ARUp->setChecked(false);
            break;
        case Qt::Key_Semicolon:
            ui->ARRight->setChecked(false);
            break;
        case Qt::Key_L:
            ui->ARDown->setChecked(false);
            break;
        case Qt::Key_K:
            ui->ARLeft->setChecked(false);
            break;
    }
}

void ZOWI_CONTROL::keyPressEvent( QKeyEvent *event ) {
    char temp[200];
    switch ( event->key() ) {
        case Qt::Key_1:
            ui->NLLeft->setChecked(true);
            break;
        case Qt::Key_2:
            ui->NLRight->setChecked(true);
            break;
        case Qt::Key_3:
            ui->NLUp->setChecked(true);
            break;
        case Qt::Key_4:
            ui->NLDown->setChecked(true);
            break;
        case Qt::Key_5:
            ui->NLCenter->setChecked(true);
            break;
        case Qt::Key_6:
            ui->NRLeft->setChecked(true);
            break;
        case Qt::Key_7:
            ui->NRRight->setChecked(true);
            break;
        case Qt::Key_8:
            ui->NRUp->setChecked(true);
            break;
        case Qt::Key_9:
            ui->NRDown->setChecked(true);
            break;
        case Qt::Key_0:
            ui->NRCenter->setChecked(true);
            break;
        case Qt::Key_W:
            ui->ALUp->setChecked(true);
            sprintf(temp, "f");
            break;
        case Qt::Key_A:
            ui->ALLeft->setChecked(true);
            sprintf(temp, "0");
            break;
        case Qt::Key_S:
            ui->ALDown->setChecked(true);
            sprintf(temp, "b");
            break;
        case Qt::Key_D:
            ui->ALRight->setChecked(true);
            sprintf(temp, "2");
            break;
        case Qt::Key_O:
            ui->ARUp->setChecked(true);
            break;
        case Qt::Key_Semicolon:
            ui->ARRight->setChecked(true);
            break;
        case Qt::Key_L:
            ui->ARDown->setChecked(true);
            break;
        case Qt::Key_K:
            ui->ARLeft->setChecked(true);
            break;
    }

    UDP_Write(sendToAddress, udpOutPort, temp);
}

void ZOWI_CONTROL::UDP_Write(QHostAddress IP_Address, int Port, char *command) {
    QUdpSocket udpSocket2;
    QByteArray datagram(command,strlen(command));
    udpSocket2.writeDatagram(datagram.data(), strlen(datagram.data()), IP_Address, Port);
}
