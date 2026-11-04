#include "videoread.h"
#include<QMessageBox>
#include<QDebug>
#include<QPainter>


VideoRead::VideoRead(QObject *parent) : QObject(parent)
  ,Qimage_tuer(":/image/tuer.png"),Qimage_hat(":/image/hat.png")
{
    qDebug()<<"VideoRead thread:"<<QThread::currentThreadId();
    m_timer = new QTimer;
//    connect( m_timer , SIGNAL(timeout())
//             , this , SLOT( slot_getVideoFrame()) );
    m_pVideoWorker=QSharedPointer<VideoWorker>(new VideoWorker);
    m_pVideoWorker->slot_setInfo(this);
    connect( m_timer , SIGNAL(timeout())
           , m_pVideoWorker.data(), SLOT(slot_dowork()) );
    //需要xml加到exe
    MyFaceDetect::FaceDetectInit();
    //加载萌拍图片
    m_mengpai=0;


}

VideoRead::~VideoRead()
{
    if( m_timer )
    {
        m_timer->stop();
        delete m_timer; m_timer = NULL;
    }
}

//摄像头采集图像流程
void VideoRead::slot_getVideoFrame()
{
    qDebug()<<"slot_getVideoFrame thread:"<<QThread::currentThreadId();
    static int s_okCnt = 0;
    static int s_failCnt = 0;
    //从摄像头对象读取数据
    Mat frame; // Mat 对象存储图像
    if( !cap.read(frame) )
    {
        if( ++s_failCnt <= 3 || (s_failCnt % 20 == 0) )
            qDebug() << "[VID] read fail. failCnt =" << s_failCnt
                     << "opened =" << cap.isOpened();
        return;
    }
    // Mat 图像不是显示和传输的格式(RGB)
    // 需要格式转换 opencv 采集的格式BGR  显示格式 RGB
    //将opencv采集的BGR的图片类型转化为RGB24的类型
    //加滤镜   加人脸识别  加梦拍  opencv 图像处理
    //获取摄像头图片后  识别出人脸位置  返回对应矩形框
    if( frame.empty() )
    {
        qDebug() << "[VID] frame empty";
        return;
    }
    if( ++s_okCnt <= 3 )
        qDebug() << "[VID] read ok. cols =" << frame.cols << "rows =" << frame.rows
                 << "ch =" << frame.channels() << "type =" << frame.type()
                 << "step =" << frame.step << "continuous =" << frame.isContinuous();
    //人脸识别失败，使用上一次矩形
    std::vector<Rect> m_face;
    //存上一次矩形是m_lastface
    if(m_mengpai!=0)
        MyFaceDetect::detectAndDisplay(frame,m_face);
    cvtColor(frame,frame,CV_BGR2RGB);
    //定义QImage对象, 用于发送数据以及图片显示
    QImage image((unsigned const char*)frame.data,frame.cols,frame.rows,QImage::Format_RGB888);


    //将道具绘制到图片上
    QImage tmp;
    if(m_mengpai==m_tuer)
    {
        tmp=Qimage_tuer;
    }
    if(m_mengpai==m_hat){
        tmp=Qimage_hat;
    }
    if(m_face.size()>0)
    {
        m_lastface=m_face;
    }
    if(m_mengpai==m_tuer||m_mengpai==m_hat)
    {
        QPainter paint(&image);
        for(int i=0;i<m_lastface.size();i++)
        {
            Rect rct=m_lastface[i];
            int x=rct.x+rct.width*0.5-tmp.width()*0.5;
            int y=rct.y-tmp.height();
            QPoint p(x,y);
            paint.drawImage(p,tmp);
        }
    }
    //转化为大小更小的图片  保持缩放比
    image = image.scaled( IMAGE_WIDTH,IMAGE_HEIGHT, Qt::KeepAspectRatio );
    //发送图片
    if( s_okCnt <= 3 )
        qDebug() << "[VID] emit frame" << s_okCnt << image.size() << "null =" << image.isNull();
    Q_EMIT SIG_sendVideoFrame( image );

}

void VideoRead::slot_openVideo()
{
    qDebug() << "[VID] slot_openVideo enter. timerActive =" << m_timer->isActive();

    qDebug() << "[VID] timer started (BEFORE cap.open), interval =" << m_timer->interval();
    //打开摄像头
    qDebug() << "[VID] calling cap.open(0) ...";
    cap.open(0);
     m_timer->start( 1000/FRAME_RATE - 10 );
    qDebug() << "[VID] cap.open(0) done. isOpened =" << cap.isOpened();
    if( cap.isOpened() )
        qDebug() << "[VID] backend =" << QString::fromStdString(cap.getBackendName())
                 << "w =" << cap.get(CAP_PROP_FRAME_WIDTH)
                 << "h =" << cap.get(CAP_PROP_FRAME_HEIGHT);
    if(!cap.isOpened()){
        QMessageBox::information(NULL,tr("提示"),tr("视频没有打开"));
        return;
    }

}

void VideoRead::slot_closeVideo()
{
    qDebug() << "[VID] slot_closeVideo. timerActive =" << m_timer->isActive()
             << "capOpened =" << cap.isOpened();
    m_timer->stop();
    //关闭摄像头
    if(cap.isOpened())
        cap.release();
}

void VideoRead::slot_setMoji(int index)
{
    m_mengpai=index;
}

void VideoRead::setMengpai(int newMengpai)
{
    m_mengpai = newMengpai;
}
