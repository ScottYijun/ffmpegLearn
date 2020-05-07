#ifndef CPLAYTHREAD_H
#define CPLAYTHREAD_H

#include <QThread>

class CPlayThread: public QThread
{
    Q_OBJECT

public:
    static CPlayThread* GetObj()
    {
        static CPlayThread pt;
        return &pt;
    }
    ~CPlayThread();
    void run();

protected:
    CPlayThread();
};

#endif // CPLAYTHREAD_H
