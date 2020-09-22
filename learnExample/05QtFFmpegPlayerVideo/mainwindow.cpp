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
    //av_register_all();//��������ѱ�����
}



//--------------------------------------------------
//http://blog.yundiantech.com/?log=blog&id=9 ��ƵͼƬ��ʾ
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
    update();//����update��ִ�� paintEvent����
}

void MainWindow::paintEvent(QPaintEvent *)
{
    //return;//��showVideo()����һ��ʹ��

    QPainter painter(this);
    painter.setBrush(Qt::black);
    painter.drawRect(0, 0, this->width(), this->height());//�Ȼ��ɺ�ɫ
    if(m_Image.size().width() <= 0)
        return;
    //��ͼ�񰴱������ųɺʹ���һ����С
    QImage img = m_Image.scaled(this->size(), Qt::KeepAspectRatio);
    int x = this->width() - img.width();
    int y = this->height() - img.height();
    x /=  2;
    y /= 2;
    painter.drawImage(QPoint(x, y), img);
}























