#ifndef CVIDEOPLAYER_H
#define CVIDEOPLAYER_H

#include <QThread>
#include <QImage>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
}

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

};

#endif // CVIDEOPLAYER_H
