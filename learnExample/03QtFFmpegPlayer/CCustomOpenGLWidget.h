#ifndef CCUSTOMOPENGLWIDGET_H
#define CCUSTOMOPENGLWIDGET_H

#include <QOpenGLWidget>

class CCustomOpenGLWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    CCustomOpenGLWidget(QWidget *parent);
    ~CCustomOpenGLWidget();

protected:
    void paintEvent(QPaintEvent *e);
    void timerEvent(QTimerEvent *e);
};

#endif // CCUSTOMOPENGLWIDGET_H
