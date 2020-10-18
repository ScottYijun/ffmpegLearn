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

#include <SDL.h>//�����Ҫ����ͷ�ļ������У�����main������ͻ,main�����ؼ������#undef main��û��Ӱ��
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
    AVCodecContext *aCodecCtx; //��??��?a???��
    AVFrame *audioFrame;// ?a??��??��1y3��?D��?��1��??o��?
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
    VideoState mVideoState; //���� ���ݸ� SDL��Ƶ�ص�����������

};

#endif // CVIDEOPLAYER_H
