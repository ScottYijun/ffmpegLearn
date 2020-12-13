#include "CCustomTitle.h"
#include "ui_CCustomTitle.h"
#include <QMouseEvent>

CCustomTitle::CCustomTitle(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CCustomTitle)
{
    ui->setupUi(this);
    // 父窗口指针赋值，并设置父窗口为无边框模式
    mWidParent = parent;
    mWidParent->setWindowFlags(Qt::FramelessWindowHint | mWidParent->windowFlags());
}

CCustomTitle::~CCustomTitle()
{
    delete ui;
}

// 重写mouseMoveEvent
void CCustomTitle::mouseMoveEvent(QMouseEvent *event)
{
    // 持续按住才做对应事件
    if (mbKeepPressed)
    {
        // 将父窗体移动到父窗体之前的位置加上鼠标移动的位置【event->globalPos()- mPntStart】
        mWidParent->move(mWidParent->geometry().topLeft() + event->globalPos()- mPntStart);
        // 将鼠标在屏幕中的位置替换为新的位置
        mPntStart = event->globalPos();
    }
}

// 重写mousePressEvent
void CCustomTitle::mousePressEvent(QMouseEvent *event)
{
    // 鼠标左键按下事件
    if (event->button() == Qt::LeftButton)
    {
        // 记录鼠标状态
        mbKeepPressed = true;
        // 记录鼠标在屏幕中的位置
        mPntStart = event->globalPos();
    }
}

// 重写mouseReleaseEvent
void CCustomTitle::mouseReleaseEvent(QMouseEvent *event)
{
    // 鼠标左键释放
    if (event->button() == Qt::LeftButton)
    {
        // 记录鼠标状态
        mbKeepPressed = false;
    }
}
