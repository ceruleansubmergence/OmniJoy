#ifndef ZOWI_CONTROL_H
#define ZOWI_CONTROL_H

#include <QWidget>
#include <QKeyEvent>
#include <QtNetwork>

namespace Ui {
class ZOWI_CONTROL;
}

class ZOWI_CONTROL : public QWidget
{
    Q_OBJECT

public:
    explicit ZOWI_CONTROL(QWidget *parent = 0);
    ~ZOWI_CONTROL();
    void UDP_Write(QHostAddress IP_Address, int Port, char *command);
    int udpOutPort;
    QHostAddress sendToAddress;

protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

private:
    Ui::ZOWI_CONTROL *ui;
};

#endif // ZOWI_CONTROL_H
