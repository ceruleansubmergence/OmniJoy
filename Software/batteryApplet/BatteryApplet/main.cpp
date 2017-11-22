#include "batteryapplet.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    BatteryApplet w;
    w.show();

    return a.exec();
}
