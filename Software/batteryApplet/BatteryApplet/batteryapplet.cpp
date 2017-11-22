#include "batteryapplet.h"
#include "ui_batteryapplet.h"

BatteryApplet::BatteryApplet(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BatteryApplet)
{
    ui->setupUi(this);

    Qt::WindowFlags m_wndFlags = windowFlags();

    setWindowFlags( Qt::Dialog | Qt::FramelessWindowHint );

    updateTimer = new QTimer();

    connect(updateTimer,SIGNAL(timeout()),this, SLOT(updateTimerSlot()));
    updateTimer->setInterval(5000);
    updateTimer->start();
    battVoltage = 0;

    ui->battLabel->setAttribute(Qt::WA_TranslucentBackground);
    ui->battLabel->setGeometry(0,0,this->width(), this->height());
    ui->battBar->setGeometry(0,0,this->width(), this->height());

    this->setGeometry(960-150, 640-40, 150, 40);
}

BatteryApplet::~BatteryApplet() {
    delete ui;

}

void BatteryApplet::updateTimerSlot() {
    char buffer[10];
    FILE* battFile = fopen("/omnijoy/current_battery_voltage", "r");
    fgets(buffer, 10, battFile);
    fclose(battFile);
    battVoltage = atof(buffer);

    // Estimate a percentage based on the voltage
    // Assume a linear response from 4V to 3.25
    float temp = (battVoltage - 3.25) / (4 - 3.25) * 100;

    // Bounds check before writing into progress bar
    if (temp > 100) temp = 100; else if (temp < 0) temp = 0;

    // Need lookup table to estimate %remaining
    ui->battBar->setValue(temp);
    ui->battLabel->setText(QString::number(battVoltage,'f',2) + "V");
}

void BatteryApplet::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        this->close();
    } else {
        //save the press position (this is relative to the current widget)
        pressPos= event->pos();
        isMoving= true;
    }
}

void BatteryApplet::mouseMoveEvent(QMouseEvent* event) {
    //isMoving flag makes sure that the drag and drop event originated
    //from within the titlebar, because otherwise the window shouldn't be moved
    if(isMoving){
        //calculate difference between the press position and the new Mouse position
        //(this is relative to the current widget)
        QPoint diff= event->pos() - pressPos;
        //move the window by diff
        window()->move(window()->pos()+diff);
    }
}


void BatteryApplet::mouseReleaseEvent(QMouseEvent* /*event*/) {
        //drag and drop operation end
        isMoving= false;
    }

////in order for the style sheet to apply on this custom widget
////see https://doc.qt.io/qt-5/stylesheet-reference.html#qwidget-widget
//void BatteryApplet::paintEvent(QPaintEvent *) {
//    QStyleOption opt;
//    opt.init(this);
//    QPainter p(this);
//    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
//}
