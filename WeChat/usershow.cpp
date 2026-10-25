#include "usershow.h"
#include "ui_usershow.h"
#include<QPainter>
#include<QDebug>
UserShow::UserShow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserShow)
{
    ui->setupUi(this);
}

UserShow::~UserShow()
{
    delete ui;
}

void UserShow::slot_setInfo(int id, QString name)
{
    m_id=id;
    m_username=name;
    QString text=QString("用户名：%1").arg(m_username);
    ui->lb_name->setText(text);
}
//重绘黑色背景
void UserShow::paintEvent(QPaintEvent *event)
{
    //画黑色背景
    QPainter paint(this);
    paint.setBrush(Qt::black);//设置黑色画刷
    paint.drawRect(-1,0,this->width()+1,this->height());
    //画图片
    if( m_img.size().height()<= 0  ) return;
    //画视频帧
    //等比例缩放
    QImage show=m_img.scaled(this->width(),
                       this->height()-ui->lb_name->height(),Qt::KeepAspectRatio);
    QPixmap pix=QPixmap::fromImage(show);//画图使用QPixmap
    //画的位置 画图使用QPixmap
    int x = this->width() - pix.width();
    int y = this->height() - pix.height() - ui->lb_name->height();
    x = x /2;
    y = ui->lb_name->height() + y/2;

    paint.drawPixmap(QPoint(x,y) , pix);
    paint.end();
}

void UserShow::slot_setImage(QImage &img)
{
    static int s_setCnt = 0;
    if( ++s_setCnt <= 3 )
        qDebug() << "[VID] UserShow setImage, id =" << m_id
                 << "size =" << img.size() << "null =" << img.isNull();
    m_img=img;
    this->update();//触发重绘
}

void UserShow::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    Q_EMIT SIG_itemClicked(m_id,m_username);
}
