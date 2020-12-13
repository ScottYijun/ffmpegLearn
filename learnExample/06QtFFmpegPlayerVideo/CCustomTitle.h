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
    // ��д����¼�
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);



private:
    Ui::CCustomTitle *ui;
    // ����ϴ��ƶ���ʼʱ�����Ļ��λ��
    QPoint mPntStart;
    // ����Ƿ��������
    bool mbKeepPressed;
    // �����ڵ�ָ��
    QWidget *mWidParent;
};

#endif // CCUSTOMTITLE_H
