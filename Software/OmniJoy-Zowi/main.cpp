#include "zowi_control.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ZOWI_CONTROL w;
    w.show();

    return a.exec();
}
