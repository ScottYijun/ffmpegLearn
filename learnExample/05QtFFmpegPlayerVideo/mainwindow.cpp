#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QPainter>
#include <QFileDialog>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initView();
    initData();
    showVideo();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::initView()
{
    //this->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));
}

void MainWindow::initData()
{
    //av_register_all();//这个函数已被弃用
}



//--------------------------------------------------
//http://blog.yundiantech.com/?log=blog&id=9 视频图片显示
void MainWindow::showVideo()
{
    m_pVideoPlayer = new CVideoPlayer();
    connect(m_pVideoPlayer, SIGNAL(signalDecodeError(int)), this, SLOT(slotDecodeError(int)));
    connect(m_pVideoPlayer, SIGNAL(signalGetOneFrame(QImage)), this, SLOT(slotGetOneFrame(QImage)));
    m_pVideoPlayer->start();
}

void MainWindow::slotDecodeError(int error)
{
    qDebug() << "slotDecodeError======error====" << error;
}

void MainWindow::slotGetOneFrame(QImage image)
{
    m_Image = image;
    update();//调用update将执行 paintEvent函数
}

void MainWindow::paintEvent(QPaintEvent *)
{
    //return;//与showVideo()函数一起使用

    QPainter painter(this);
    painter.setBrush(Qt::black);
    painter.drawRect(0, 0, this->width(), this->height());//先画成黑色
    if(m_Image.size().width() <= 0)
        return;
    //将图像按比例缩放成和窗口一样大小
    QImage img = m_Image.scaled(this->size(), Qt::KeepAspectRatio);
    int x = this->width() - img.width();
    int y = this->height() - img.height();
    x /=  2;
    y /= 2;
    painter.drawImage(QPoint(x, y), img);
}























