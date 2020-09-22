#include "CCustomOpenGLWidget.h"
#include <QPainter>
#include <QImage>
#include <iostream>
#include <list>
#include "CMyFFmpeg.h"
#include "CPlayThread.h"

using namespace std;

extern "C"
{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

static list<AVPacket> g_videos;

CCustomOpenGLWidget::CCustomOpenGLWidget(QWidget *parent)
    :QOpenGLWidget(parent)
{
    startTimer(100);//Ë¢ÐÂÊÓÆµ
    CPlayThread::GetObj()->start();
}

CCustomOpenGLWidget::~CCustomOpenGLWidget()
{

}

void CCustomOpenGLWidget::paintEvent(QPaintEvent *e)
{
    static QImage *image;
    if(nullptr == image)
    {
        uchar *buf = new uchar[width() * height() * 4];
        image = new QImage(buf, width(), height(), QImage::Format_ARGB32);
    }

    bool nRet = CMyFFmpeg::GetObj()->YuvToRGB((char*)(image->bits()), width(), height());
    if(!nRet)
        return;
    QPainter painter;
    painter.begin(this);
    painter.drawImage(QPoint(0, 0), *image);
    painter.end();
}

void CCustomOpenGLWidget::timerEvent(QTimerEvent *e)
{
    this->update();
}












