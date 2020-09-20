#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

extern "C"
{
#include "libavcodec/avcodec.h" //��װ��ʽ
#include "libavformat/avformat.h" //����
#include "libswscale/swscale.h" //����
#include "libavdevice/avdevice.h"

#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"

#include <SDL.h>
//#include <SDL_audio.h>
//#include <SDL_types.h>
//#include <SDL_name.h>
//#include <SDL_main.h>
//#include <SDL_config.h>
}

//==================================
#define SDL_AUDIO_BUFFER_SIZE 1024
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

///�������ǽ�������C++�Ĺ���
///�����ʱ��ʹ�õ�C++�ı���������
///��FFMPEG��C�Ŀ�
///���������Ҫ����extern "C"
///�������ʾ����δ����
typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

// �����������е�ʹ�û���
//AVFrame* audioFrame = avcodec_alloc_frame();
AVFrame* audioFrame = av_frame_alloc();
PacketQueue *audioq;

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

int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size)
{
    static AVPacket pkt;
    static uint8_t *audio_pkt_data = NULL;
    static int audio_pkt_size = 0;
    int len1, data_size;

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
            printf("ret======%d   got_picture======%d\n", ret, got_picture);
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
                memcpy(audio_buf, sample_buffer, in_samples * 4);
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
    AVCodecContext *aCodecCtx = (AVCodecContext *) userdata;
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
            audio_data_size = audio_decode_frame(aCodecCtx, audio_buf,sizeof(audio_buf));
            printf("audio_data_size===============%d\n", audio_data_size);
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


//===================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->pushButton_openAudio, SIGNAL(clicked()), this, SLOT(slotOpenAudio()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::slotOpenAudio()
{
    char *path = (char*)"./test.aac";
    int ret = playAudio(path);
    qDebug() << "ret====================" << ret;
}

int MainWindow::playAudio(char *filepath)
{
    //����򵥵����һ���汾��
#ifdef _WIN64
    qDebug() << "Hello FFmpeg(64 bitλ)!====================";
    cout << "Hello FFmpeg(64bitλ)!" << endl;
#elif _WIN32
    qDebug() << "Hello FFmpeg(32 bitλ)!====================";
#endif
    av_register_all();//ffmpeg 4.2.2�Ѿ�����Ҫ���������
    unsigned version = avcodec_version();
    qDebug() << "version is====================" << version;
    //Ŀǰ����ֻ����aac���ļ� �Թ�mp3���Ų����� ����û�в���
    //������ffmpeg�Դ��Ľ���������mp3������  �Ժ����о���
    //������ӵ��ص��ǽ�SDL��ʹ�� ��˲�������
    //����ס·����Ҫ�����ģ�
    //char *filename = (char *)"./test.aac";//����OK
    //char *filename = (char *)"../mp4/test.aac";//����OK
    //char *filename = (char *)"../mp4/in.aac";//����OK
    char *filename = (char *)"../mp4/mozart.mp3";//����OK

    AVFormatContext* pFormatCtx = avformat_alloc_context();
    if( avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0 ) {
        printf("Couldn't open file.\n");
        return -1;
    }
    else
    {
        printf("open file success====%s.\n", filename);
    }

    // Retrieve stream information
    if( avformat_find_stream_info(pFormatCtx, NULL) < 0 ) {
        printf("Couldn't find stream information.\n");
        return -1;
    }
    else
    {
        printf("find stream information========.\n");
    }

    // Dump valid information onto standard error
    av_dump_format(pFormatCtx, 0, filename, false);

    ///ѭ�����Ұ�������Ƶ����Ϣ��ֱ���ҵ���Ƶ���͵���
    ///�㽫���¼���� ���浽audioStream������
    int audioStream = -1;
    for(unsigned int i=0; i < pFormatCtx->nb_streams; i++) {
        if( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO ) {
            audioStream = i;
            break;
        }
    }

    printf("pFormatCtx->nb_streams========%d\n", pFormatCtx->nb_streams);
    ///���audioStreamΪ-1 ˵��û���ҵ���Ƶ��
    if( audioStream == -1 ) {
        printf("Didn't find a audio stream.\n");
        return -1;
    }
    else
    {
        printf("find a audio stream======audioStream==%d.\n", audioStream);
    }

    // Get a pointer to the codec context for the audio stream
    AVCodecContext* audioCodecCtx = pFormatCtx->streams[audioStream]->codec;

    ///���ҽ�����
    AVCodec* pCodec = avcodec_find_decoder(audioCodecCtx->codec_id);
    if( pCodec == NULL ) {
        printf("Codec not found.\n");
        return -1;
    }
    else
    {
        printf("find Codec========.\n");
    }

    ///�򿪽�����
    AVDictionary* options = NULL;
    if( avcodec_open2(audioCodecCtx, pCodec, &options) < 0 ) {
        printf("Could not open codec.\n");
        return -1;
    }
    else
    {
        printf("open Codec========.\n");
    }

    ///  ��SDL�����豸 - ��ʼ
    SDL_LockAudio();
    SDL_AudioSpec spec;
    SDL_AudioSpec wanted_spec;
    printf("audioCodecCtx->sample_rate========%d\n", audioCodecCtx->sample_rate);

    wanted_spec.freq = audioCodecCtx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = audioCodecCtx->channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = audioCodecCtx;
    if(SDL_OpenAudio(&wanted_spec, &spec) < 0)
    {
        fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
        return 0;
    }
    else
    {
        printf("SDL_OpenAudio success========.\n");
    }
    SDL_UnlockAudio();
    SDL_PauseAudio(0);
    ///  ��SDL�����豸 - ����

    //��ʼ����Ƶ����
    audioq = new PacketQueue;
    packet_queue_init(audioq);

    AVPacket *packet = (AVPacket *)malloc(sizeof(AVPacket));
    av_init_packet(packet);

    // �����������е�ʹ�û���
    AVFrame* audioFrame = av_frame_alloc();
    // Debug -- Begin
    printf("������ %3d\n", pFormatCtx->bit_rate);
    printf("���������� %s\n", audioCodecCtx->codec->long_name);
    printf("time_base  %d \n", audioCodecCtx->time_base);
    printf("������  %d \n", audioCodecCtx->channels);
    printf("sample per second  %d \n", audioCodecCtx->sample_rate);
    // Debug -- End
    int nCount = 0;
    while(1)
    {
        int nFrame = av_read_frame(pFormatCtx, packet);
        printf("nFrame=========================%d  nCount======%d\n", nFrame, ++nCount);
        if(nFrame < 0)
        //if (av_read_frame(pFormatCtx, packet) < 0 )
        {
            break; //������Ϊ��Ƶ��ȡ����
        }
        printf("packet->stream_index===%d,  audioStream====%d\n", packet->stream_index, audioStream);
        if( packet->stream_index == audioStream )
        {
            int npacket = packet_queue_put(audioq, packet);
            printf("npacket=======%d\n", npacket);
            //�������ǽ����ݴ������ ��˲����� av_free_packet �ͷ�
        }
        else
        {
            // Free the packet that was allocated by av_read_frame
            av_free_packet(packet);
        }

        SDL_Delay(10);
    }

    printf("read finished!\n");

    av_free(audioFrame);
    avcodec_close(audioCodecCtx);// Close the codec
    avformat_close_input(&pFormatCtx);// Close the video file

    return 0;
}



