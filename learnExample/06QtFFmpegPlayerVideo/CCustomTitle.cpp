#include "CCustomTitle.h"
#include "ui_CCustomTitle.h"
#include <QMouseEvent>

CCustomTitle::CCustomTitle(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CCustomTitle)
{
    ui->setupUi(this);
    // ������ָ�븳ֵ�������ø�����Ϊ�ޱ߿�ģʽ
    mWidParent = parent;
    mWidParent->setWindowFlags(Qt::FramelessWindowHint | mWidParent->windowFlags());
}

CCustomTitle::~CCustomTitle()
{
    delete ui;
}

// ��дmouseMoveEvent
void CCustomTitle::mouseMoveEvent(QMouseEvent *event)
{
    // ������ס������Ӧ�¼�
    if (mbKeepPressed)
    {
        // ���������ƶ���������֮ǰ��λ�ü�������ƶ���λ�á�event->globalPos()- mPntStart��
        mWidParent->move(mWidParent->geometry().topLeft() + event->globalPos()- mPntStart);
        // ���������Ļ�е�λ���滻Ϊ�µ�λ��
        mPntStart = event->globalPos();
    }
}

// ��дmousePressEvent
void CCustomTitle::mousePressEvent(QMouseEvent *event)
{
    // �����������¼�
    if (event->button() == Qt::LeftButton)
    {
        // ��¼���״̬
        mbKeepPressed = true;
        // ��¼�������Ļ�е�λ��
        mPntStart = event->globalPos();
    }
}

// ��дmouseReleaseEvent
void CCustomTitle::mouseReleaseEvent(QMouseEvent *event)
{
    // �������ͷ�
    if (event->button() == Qt::LeftButton)
    {
        // ��¼���״̬
        mbKeepPressed = false;
    }
}
