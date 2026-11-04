#ifndef VIDEOREAD_H
#define VIDEOREAD_H

#include <QObject>
#include <QImage>
#include<QTimer>
#include"common.h"
#include"threadworker.h"
#include<QSharedPointer>
#include"myfacedetect.h"
//图片的宽高
#define IMAGE_WIDTH (320)
#define IMAGE_HEIGHT (240)


class VideoWorker;
class VideoRead : public QObject
{
    Q_OBJECT
public:
    explicit VideoRead(QObject *parent = nullptr);
    ~VideoRead();
signals:
    void SIG_sendVideoFrame(QImage img);
public slots:
    void slot_getVideoFrame();
    void slot_openVideo();
    void slot_closeVideo();
    void slot_setMoji(int index);
public:
    QTimer *m_timer;

    //opencv获取图片的对象
    cv::VideoCapture cap;
    QSharedPointer<VideoWorker>m_pVideoWorker;
    std::vector<Rect> m_lastface;
    enum mengpai_type{m_tuer=1,m_hat};
    int m_mengpai;
    QImage Qimage_tuer;
     QImage Qimage_hat;

     void setMengpai(int newMengpai);
};

class VideoWorker:public ThreadWorker{
    Q_OBJECT
public slots:
    void slot_setInfo(VideoRead *p)
    {
        m_pVideoRead=p;
    }
    //定时器到时，执行
    void slot_dowork()
    {
        m_pVideoRead->slot_getVideoFrame();
    }
public:
    VideoRead * m_pVideoRead;
};

#endif // VIDEOREAD_H
