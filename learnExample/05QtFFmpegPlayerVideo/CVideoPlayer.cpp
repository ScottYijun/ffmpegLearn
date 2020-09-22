#include "CVideoPlayer.h"
#include <stdio.h>
#include <QDebug>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"

#include <libavutil/time.h>
#include "libavutil/pixfmt.h"
#include "libswresample/swresample.h"

#include <SDL.h>//�����Ҫ����ͷ�ļ������У�����main������ͻ
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
    AVCodecContext *aCodecCtx; //��Ƶ������
    AVFrame *audioFrame;// ������Ƶ�����е�ʹ�û���
    PacketQueue *audioq;
    double video_clock; ///<pts of last decoded frame / predicted pts of next decoded frame
    AVStream *video_st;
} VideoState;

#define SDL_AUDIO_BUFFER_SIZE 1024

VideoState mVideoState; //���� ���ݸ� SDL��Ƶ�ص�����������

void packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt) {

    AVPacketList *pkt1;
    if (av_dup_packet(pkt) < 0) {
        return -1;
    }
    pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    SDL_LockMutex(q->mutex);

    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
    SDL_CondSignal(q->cond);

    SDL_UnlockMutex(q->mutex);
    return 0;
}
//http://blog.yundiantech.com/?log=blog&id=9
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block) {
    AVPacketList *pkt1;
    int ret;

    SDL_LockMutex(q->mutex);

    for (;;) {

        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}
int audio_decode_frame(VideoState *is, uint8_t *audio_buf, int buf_size)
{

    static AVPacket pkt;
    static uint8_t *audio_pkt_data = NULL;
    static int audio_pkt_size = 0;
    int len1, data_size;

    AVCodecContext *aCodecCtx = is->aCodecCtx;
    AVFrame *audioFrame = is->audioFrame;
    PacketQueue *audioq = is->audioq;

    for(;;)
    {
        if(packet_queue_get(audioq, &pkt, 1) < 0)
        {
            return -1;
        }
        audio_pkt_data = pkt.data;
        audio_pkt_size = pkt.size;
        while(audio_pkt_size > 0)
        {
            int got_picture;

            int ret = avcodec_decode_audio4( aCodecCtx, audioFrame, &got_picture, &pkt);
            if( ret < 0 ) {
                printf("Error in decoding audio frame.\n");
                exit(0);
            }

            if( got_picture ) {
                int in_samples = audioFrame->nb_samples;
                short *sample_buffer = (short*)malloc(audioFrame->nb_samples * 2 * 2);
                memset(sample_buffer, 0, audioFrame->nb_samples * 4);

                int i=0;
                float *inputChannel0 = (float*)(audioFrame->extended_data[0]);

                // Mono
                if( audioFrame->channels == 1 ) {
                    for( i=0; i<in_samples; i++ ) {
                        float sample = *inputChannel0++;
                        if( sample < -1.0f ) {
                            sample = -1.0f;
                        } else if( sample > 1.0f ) {
                            sample = 1.0f;
                        }

                        sample_buffer[i] = (int16_t)(sample * 32767.0f);
                    }
                } else { // Stereo
                    float* inputChannel1 = (float*)(audioFrame->extended_data[1]);
                    for( i=0; i<in_samples; i++) {
                        sample_buffer[i*2] = (int16_t)((*inputChannel0++) * 32767.0f);
                        sample_buffer[i*2+1] = (int16_t)((*inputChannel1++) * 32767.0f);
                    }
                }
//                fwrite(sample_buffer, 2, in_samples*2, pcmOutFp);
                memcpy(audio_buf,sample_buffer,in_samples*4);
                free(sample_buffer);
            }

            audio_pkt_size -= ret;

            if (audioFrame->nb_samples <= 0)
            {
                continue;
            }

            data_size = audioFrame->nb_samples * 4;

            return data_size;
        }
        if(pkt.data)
            av_free_packet(&pkt);
   }
}
void audio_callback(void *userdata, Uint8 *stream, int len)
{
//    AVCodecContext *aCodecCtx = (AVCodecContext *) userdata;
    VideoState *is = (VideoState *) userdata;

    int len1, audio_data_size;

    static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
    static unsigned int audio_buf_size = 0;
    static unsigned int audio_buf_index = 0;

    /*   len����SDL�����SDL�������Ĵ�С������������δ�������Ǿ�һֱ����������� */
    while (len > 0) {
        /*  audio_buf_index �� audio_buf_size ��ʾ�����Լ��������ý�����������ݵĻ�������*/
        /*   ��Щ���ݴ�copy��SDL�������� ��audio_buf_index >= audio_buf_size��ʱ����ζ����*/
        /*   �ǵĻ���Ϊ�գ�û�����ݿɹ�copy����ʱ����Ҫ����audio_decode_frame���������
         /*   ��������� */

        if (audio_buf_index >= audio_buf_size) {
            audio_data_size = audio_decode_frame(is, audio_buf,sizeof(audio_buf));
            /* audio_data_size < 0 ��ʾû�ܽ�������ݣ�����Ĭ�ϲ��ž��� */
            if (audio_data_size < 0) {
                /* silence */
                audio_buf_size = 1024;
                /* ���㣬���� */
                memset(audio_buf, 0, audio_buf_size);
            } else {
                audio_buf_size = audio_data_size;
            }
            audio_buf_index = 0;
        }
        /*  �鿴stream���ÿռ䣬����һ��copy�������ݣ�ʣ�µ��´μ���copy */
        len1 = audio_buf_size - audio_buf_index;
        if (len1 > len) {
            len1 = len;
        }

        memcpy(stream, (uint8_t *) audio_buf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        audio_buf_index += len1;
    }
}

static double synchronize_video(VideoState *is, AVFrame *src_frame, double pts) {

    double frame_delay;

    if (pts != 0) {
        /* if we have pts, set video clock to it */
        is->video_clock = pts;
    } else {
        /* if we aren't given a pts, set it to the clock */
        pts = is->video_clock;
    }
    /* update the video clock */
    frame_delay = av_q2d(is->video_st->codec->time_base);
    /* if we are repeating a frame, adjust clock accordingly */
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    is->video_clock += frame_delay;
    return pts;
}
CVideoPlayer::CVideoPlayer()
{

}

void CVideoPlayer::run()
{
    videoDecode();
}

void CVideoPlayer::videoDecode()
{
    char *filePath = (char*)"F:\\github\\QtPlayLearn\\win\\mp4\\lasa.mp4";
    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame, *pFrameRGB;
    AVPacket *packet;
    uint8_t *outBuffer;

    AVCodecContext *aCodecCtx;
    AVCodec *aCodec;
    static struct SwsContext *img_convert_ctx;
    unsigned int i;
    int audioStream ,videoStream, numBytes;
    int ret, got_picture;
    av_register_all();//��ʼ��ffmpeg ��������������������ñ������ͽ�����
    if (SDL_Init(SDL_INIT_AUDIO)) {
        fprintf(stderr,"Could not initialize SDL - %s. \n", SDL_GetError());
        exit(1);
    }
    //Allocate an AVFormatContext.
    pFormatCtx = avformat_alloc_context();
    if(0 != avformat_open_input(&pFormatCtx, filePath, nullptr, nullptr))
    {
        emit signalDecodeError(-1);
        return;
    }

    if(avformat_find_stream_info(pFormatCtx, nullptr))
    {
        emit signalDecodeError(-2);
        return;
    }
    videoStream = -1;
    audioStream = -1;
    //ѭ��������Ƶ�а���������Ϣ��ֱ���ҵ���Ƶ���͵���
    //�㽫���¼���� ���浽videoStream������
    //������������ֻ������Ƶ��  ��Ƶ���Ȳ�����
    for(i = 0; i < pFormatCtx->nb_streams; ++i)
    {
        qDebug() << "pFormatCtx->streams[" << i << "]->codec->codec_type = " << pFormatCtx->streams[i]->codec->codec_type << endl;
        //0:��Ƶ���� 1:��Ƶ����
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;
        }
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO  && audioStream < 0)
        {
            audioStream = i;
        }
    }
    qDebug() << "videoStream===========" << videoStream << "  pFormatCtx->nb_streams==" << pFormatCtx->nb_streams << endl;
    //���videoStreamΪ-1 ˵��û���ҵ���Ƶ��
    if (videoStream == -1) {
        printf("Didn't find a video stream.\n");
        return;
    }
    if (audioStream == -1) {
        printf("Didn't find a audio stream.\n");
        return;
    }
    ///������Ƶ������
    aCodecCtx = pFormatCtx->streams[audioStream]->codec;
    aCodec = avcodec_find_decoder(aCodecCtx->codec_id);

    if (aCodec == NULL) {
        printf("ACodec not found.\n");
        return;
    }
    ///����Ƶ������
    if (avcodec_open2(aCodecCtx, aCodec, NULL) < 0) {
        printf("Could not open audio codec.\n");
        return;
    }
    //��ʼ����Ƶ����
    PacketQueue *audioq = new PacketQueue;
    packet_queue_init(audioq);
    // �����������е�ʹ�û���
    //AVFrame* audioFrame = avcodec_alloc_frame();
    AVFrame* audioFrame = av_frame_alloc(); //ffmpeg v4.2.2
    mVideoState.aCodecCtx = aCodecCtx;
    mVideoState.audioq = audioq;
    mVideoState.audioFrame = audioFrame;
    ///  ��SDL�����豸 - ��ʼ
    SDL_LockAudio();
    SDL_AudioSpec spec;
    SDL_AudioSpec wanted_spec;
    wanted_spec.freq = aCodecCtx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = aCodecCtx->channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = &mVideoState;
    if(SDL_OpenAudio(&wanted_spec, &spec) < 0)
    {
        fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
        return;
    }
    SDL_UnlockAudio();
    SDL_PauseAudio(0);
    ///  ��SDL�����豸 - ����
    ///������Ƶ������
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    qDebug() << "pCodecCtx->codec_id===========" << pCodecCtx->codec_id << endl;
    //����ʱ���ֵΪ27���鵽ö��ֵ��Ӧ����AV_CODEC_ID_H264 ������H264ѹ����ʽ���ļ���
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(nullptr == pCodec)
    {
        printf("PCodec not found.\n");
        emit signalDecodeError(-4);
        return;
    }

    //����Ƶ������s
    if(avcodec_open2(pCodecCtx, pCodec, nullptr) < 0)
    {
        printf("Could not open video codec.\n");
        emit signalDecodeError(-5);
        return;
    }

    mVideoState.video_st = pFormatCtx->streams[videoStream];

    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, \
                                     pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, \
                                     AV_PIX_FMT_RGB32, SWS_BICUBIC, nullptr, nullptr, nullptr);

    numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);
    //av_image_get_buffer_size();
    outBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    avpicture_fill((AVPicture *)pFrameRGB, outBuffer, AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);
    int y_size = pCodecCtx->width * pCodecCtx->height;
    packet = (AVPacket *)malloc(sizeof(AVPacket));//����һ��packet
    av_new_packet(packet, y_size);//����packet������
    av_dump_format(pFormatCtx, 0,  filePath, 0);//�����Ƶ��Ϣ

    int64_t start_time = av_gettime();
    int64_t pts = 0; //��ǰ��Ƶ��pts
    int index = 0;
    while (1)
    {
        if(av_read_frame(pFormatCtx, packet) < 0)
        {
            qDebug() << "index===============" << index;
            break;//������Ϊ��Ƶ��ȡ����
        }

        int64_t realTime = av_gettime() - start_time; //��ʱ��ʱ��
        while(pts > realTime)
        {
            SDL_Delay(10);
            realTime = av_gettime() - start_time; //��ʱ��ʱ��
        }
        if(packet->stream_index == videoStream)
        {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if(ret < 0)
            {
                emit signalDecodeError(-6);
                return;
            }
            //add
            if (packet->dts == AV_NOPTS_VALUE && pFrame->opaque&& *(uint64_t*) pFrame->opaque != AV_NOPTS_VALUE)
            {
                pts = *(uint64_t *) pFrame->opaque;
            }
            else if (packet->dts != AV_NOPTS_VALUE)
            {
                pts = packet->dts;
            }
            else
            {
                pts = 0;
            }

            pts *= 1000000 * av_q2d(mVideoState.video_st->time_base);
            pts = synchronize_video(&mVideoState, pFrame, pts);
            //--------
            if(got_picture)
            {
                sws_scale(img_convert_ctx, (uint8_t const * const *)pFrame->data,
                          pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
                ++index;
                //�����RGB���� ��QImage����
                QImage tempImage((uchar*)outBuffer, pCodecCtx->width, pCodecCtx->height, QImage::Format_RGB32);
                QImage image = tempImage.copy();//��ͼ����һ�� ���ݸ�������ʾ
                qDebug() << "image.width==" << image.width() << "image.height==" << image.height();
                emit signalGetOneFrame(image);
            }

            av_free_packet(packet);
        }
        else if( packet->stream_index == audioStream )
        {
            packet_queue_put(mVideoState.audioq, packet);
            //�������ǽ����ݴ������ ��˲����� av_free_packet �ͷ�
        }
        else
        {
            // Free the packet that was allocated by av_read_frame
            av_free_packet(packet);
        }
    }
    av_free(outBuffer);
    av_free(pFrameRGB);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

}











