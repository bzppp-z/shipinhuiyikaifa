#ifndef SCREENREAD_H
#define SCREENREAD_H

#include <QObject>
#include<QTimer>
#include<QApplication>
#include<QDesktopWidget>
#include<QScreen>
class ScreenRead : public QObject
{
    Q_OBJECT
public:
    explicit ScreenRead(QObject *parent = nullptr);
    ~ScreenRead();
signals:
    void getScreenFrame(QImage img);
public slots:
    void slot_getScreenFrame();
    void slot_Openvideo();
    void slot_PauseVideo();
private:
    QTimer* m_timer;
};

#endif // SCREENREAD_H
