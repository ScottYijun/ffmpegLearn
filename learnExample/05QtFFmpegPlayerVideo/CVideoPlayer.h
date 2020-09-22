#ifndef CVIDEOPLAYER_H
#define CVIDEOPLAYER_H

#include <QThread>
#include <QImage>



class CVideoPlayer: public QThread
{
    Q_OBJECT

public:
    CVideoPlayer();
    void videoDecode();

protected:
    void run();

signals:
    void signalGetOneFrame(QImage image);
    void signalDecodeError(int error);

private:


};

#endif // CVIDEOPLAYER_H
