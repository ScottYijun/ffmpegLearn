#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QPainter>
#include <QFileDialog>
#include "CMyFFmpeg.h"

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
    connect(ui->pushButton_OpenVideo, SIGNAL(clicked()), this, SLOT(slotOpenFile()));
    connect(ui->pushButton_play, SIGNAL(clicked()), this, SLOT(slotPlay()));
}

//void MainWindow::slotOpenFile()
//{
//    QString fileName = QFileDialog::getOpenFileName(this, QString::fromLocal8Bit("����Ƶ�ļ�"));
//    if(fileName.isEmpty())
//        return;
//    ui->lineEdit_videoPath->setText(fileName);
//    qDebug() << "fileName========" << fileName;
//    CMyFFmpeg::GetObj()->openVideo(fileName.toLocal8Bit());
//    CMyFFmpeg::GetObj()->m_isPlay = true;
//    ui->pushButton_play->setText(QString::fromLocal8Bit("��ͣ"));
//}

//void MainWindow::slotPlay()
//{
//    if(ui->pushButton_play->text() == QString::fromLocal8Bit("��ͣ"))
//    {
//        CMyFFmpeg::GetObj()->m_isPlay = false;
//        ui->pushButton_play->setText(QString::fromLocal8Bit("����"));
//    }
//    else
//    {
//        CMyFFmpeg::GetObj()->m_isPlay = true;
//        ui->pushButton_play->setText(QString::fromLocal8Bit("��ͣ"));
//    }
//}

//--------------------------------------------------
//http://blog.yundiantech.com/?log=blog&id=9 ��ƵͼƬ��ʾ
void MainWindow::showVideo()
{
    ui->openGLWidget->hide();
    ui->label->hide();
    ui->pushButton_play->hide();
    ui->lineEdit_videoPath->hide();
    ui->pushButton_barrage->hide();
    ui->pushButton_cutImage->hide();
    ui->pushButton_OpenVideo->hide();
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
    return;//��showVideo()����һ��ʹ��

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























