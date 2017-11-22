#ifndef BATTERYAPPLET_H
#define BATTERYAPPLET_H

#include <QWidget>
#include <QMouseEvent>
#include <QTimer>

namespace Ui {
class BatteryApplet;
}

class BatteryApplet : public QWidget
{
    Q_OBJECT

public:
    explicit BatteryApplet(QWidget *parent = 0);
    ~BatteryApplet();

    Qt::WindowFlags m_wndFlags;

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* /*event*/);
    //void paintEvent(QPaintEvent *);

private:
    Ui::BatteryApplet *ui;
    QPoint pressPos;
    bool isMoving;
    QTimer *updateTimer;
    float battVoltage;

private slots:
    void updateTimerSlot(void);

};

#endif // BATTERYAPPLET_H
