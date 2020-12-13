#ifndef CCUSTOMTITLE_H
#define CCUSTOMTITLE_H

#include <QWidget>

//https://blog.csdn.net/motou263514/article/details/78090483

namespace Ui {
class CCustomTitle;
}

class CCustomTitle : public QWidget
{
    Q_OBJECT

public:
    explicit CCustomTitle(QWidget *parent = nullptr);
    ~CCustomTitle();protected:
    // 重写鼠标事件
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);



private:
    Ui::CCustomTitle *ui;
    // 鼠标上次移动开始时相对屏幕的位置
    QPoint mPntStart;
    // 鼠标是否持续按下
    bool mbKeepPressed;
    // 父窗口的指针
    QWidget *mWidParent;
};

#endif // CCUSTOMTITLE_H
