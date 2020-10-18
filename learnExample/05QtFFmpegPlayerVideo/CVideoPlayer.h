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

#include <libavutil/time.h>
#include "libavutil/pixfmt.h"
#include "libswresample/swresample.h"

#include <SDL.h>//这个不要放在头文件包含中，否则报main函数冲突,main函数关加上这个#undef main就没有影响
//#include <SDL_audio.h>
//#include <SDL_types.h>
//#include <SDL_name.h>
//#include <SDL_main.h>
//#include <SDL_config.h>
}

typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 1
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

typedef struct VideoState {
    AVCodecContext *aCodecCtx; //ò??μ?a???÷
    AVFrame *audioFrame;// ?a??ò??μ1y3ì?Dμ?ê1ó??o′?
    PacketQueue *audioq;
    double video_clock; ///<pts of last decoded frame / predicted pts of next decoded frame
    AVStream *video_st;
} VideoState;

#define SDL_AUDIO_BUFFER_SIZE 1024




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
    VideoState mVideoState; //用来 传递给 SDL音频回调函数的数据

};

#endif // CVIDEOPLAYER_H
