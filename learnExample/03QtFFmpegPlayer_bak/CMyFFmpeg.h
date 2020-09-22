#ifndef CMYFFMPEG_H
#define CMYFFMPEG_H

#include <QMutex>

extern "C"
{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}


class CMyFFmpeg
{
public:
    static CMyFFmpeg* GetObj()
    {
        static CMyFFmpeg ff;
        return &ff;
    }

    virtual ~CMyFFmpeg();
    void openVideo(const char *path);
    void decodeFrame(const AVPacket *pkt);
    AVPacket readFrame();
    bool YuvToRGB(char *out, int outweight, int outheight);

    bool m_isPlay;
    AVFrame *m_yuv = nullptr;//��Ƶ֡����
    SwsContext *m_cCtx = nullptr;//ת����

protected:
    CMyFFmpeg();
    AVFormatContext *m_afc = nullptr;
    int m_videoStream = 0;
    QMutex m_mutex;
};

#endif // CMYFFMPEG_H
