#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

extern "C"
{
    //封装格式
#include "libavformat/avformat.h"
    //解码
#include "libavcodec/avcodec.h"
    //缩放
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
    //播放
#include "SDL2/SDL.h"
};

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



static Uint8 *audio_chunk;
//设置音频数据长度
static Uint32 audio_len;
static Uint8 *audio_pos;

void fill_audio(void *udata, Uint8 *stream, int len){
    //SDL 2.0
    SDL_memset(stream, 0, len);
    if (audio_len == 0)		//有数据才播放
        return;
    len = (len>audio_len ? audio_len : len);

    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
    audio_pos += len;
    audio_len -= len;
}

int MainWindow::playAudio(char *filename)
{
    AVFormatContext	*pFormatCtx;
    AVCodecContext	*pCodeCtx;
    AVCodec			*pCodec;
    int index;
    SwrContext *swrCtx;

    //重采样设置选项-----------------------------------------------------------start
    //输入的采样格式
    enum AVSampleFormat in_sample_fmt;
    //输出的采样格式 16bit PCM
    enum AVSampleFormat out_sample_fmt;
    //输入的采样率
    int in_sample_rate;
    //输出的采样率
    int out_sample_rate;
    //输入的声道布局
    uint64_t in_ch_layout;
    //输出的声道布局
    uint64_t out_ch_layout;
    //重采样设置选项-----------------------------------------------------------end
    //SDL_AudioSpec
    SDL_AudioSpec wanted_spec;
    //获取输出的声道个数
    int out_channel_nb;

    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
    }

    //char *filepath = (char *)"./test.aac";//播放OK
    //char *filepath = (char *)"../mp4/test.aac";//播放OK
    //char *filepath = (char *)"../mp4/in.aac";//播放OK
    char *filepath = (char *)"../mp4/mozart.mp3";//播放OK
    int type;

    type = AVMEDIA_TYPE_AUDIO;
    //打开输入文件
    if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0){
        printf("Couldn't open input stream.\n");
        return -1;
    }
    //获取文件信息
    if (avformat_find_stream_info(pFormatCtx, NULL)<0){
        printf("Couldn't find stream information.\n");
        return -1;
    }
    //找到对应的type所在的pFormatCtx->streams的索引位置
    index = -1;
    for (int i = 0; i<pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codec->codec_type == type){
            index = i;
            break;
        }
    if (index == -1){
        printf("Didn't find a video stream.\n");
        return -1;
    }
    //获取解码器
    pCodeCtx = pFormatCtx->streams[index]->codec;
    pCodec = avcodec_find_decoder(pCodeCtx->codec_id);
    if (pCodec == NULL){
        printf("Codec not found.\n");
        return -1;
    }
    //打开解码器
    if (avcodec_open2(pCodeCtx, pCodec, NULL)<0){
        printf("Could not open codec.\n");
        return -1;
    }

    //重采样设置选项-----------------------------------------------------------start
    //输入的采样格式
    in_sample_fmt = pCodeCtx->sample_fmt;
    //输出的采样格式 16bit PCM
    out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入的采样率
    in_sample_rate = pCodeCtx->sample_rate;
    //输出的采样率
    out_sample_rate = 44100;
    //输入的声道布局
    in_ch_layout = pCodeCtx->channel_layout;
    if (in_ch_layout <= 0)
    {
        in_ch_layout = av_get_default_channel_layout(pCodeCtx->channels);
    }
    //输出的声道布局
    out_ch_layout = AV_CH_LAYOUT_MONO;

    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    swrCtx = swr_alloc();
    swr_alloc_set_opts(swrCtx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout, in_sample_fmt,
        in_sample_rate, 0, NULL);
    swr_init(swrCtx);
    //重采样设置选项-----------------------------------------------------------end

    //获取输出的声道个数
    out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
    //SDL_AudioSpec
    wanted_spec.freq = out_sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = out_channel_nb;
    wanted_spec.silence = 0;
    wanted_spec.samples = pCodeCtx->frame_size;
    wanted_spec.callback = fill_audio;//回调函数
    wanted_spec.userdata = pCodeCtx;
    if (SDL_OpenAudio(&wanted_spec, NULL)<0){
        printf("can't open audio.\n");
        return -1;
    }

    //编码数据
    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    //解压缩数据
    AVFrame *frame = av_frame_alloc();
    //存储pcm数据
    uint8_t *out_buffer = (uint8_t *)av_malloc(2 * 44100);

    int ret, got_frame, framecount = 0;
    //一帧一帧读取压缩的音频数据AVPacket
    while (1)
        while (av_read_frame(pFormatCtx, packet) >= 0) {
            if (packet->stream_index == index) {
                //解码AVPacket->AVFrame
                ret = avcodec_decode_audio4(pCodeCtx, frame, &got_frame, packet);
                if (ret < 0) {
                    printf("%s", "解码完成");
                }
                //非0，正在解码
                int out_buffer_size;
                if (got_frame) {
                    printf("解码%d帧", framecount++);
                    swr_convert(swrCtx, &out_buffer, 2 * 44100, (const uint8_t **)frame->data, frame->nb_samples);
                    //获取sample的size
                    out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb, frame->nb_samples,
                        out_sample_fmt, 1);
                    //设置音频数据缓冲,PCM数据
                    audio_chunk = (Uint8 *)out_buffer;
                    //设置音频数据长度
                    audio_len = out_buffer_size;
                    audio_pos = audio_chunk;
                    //回放音频数据
                    SDL_PauseAudio(0);
                    while (audio_len>0)//等待直到音频数据播放完毕!
                        SDL_Delay(10);

                    packet->data += ret;
                    packet->size -= ret;
                }
            }
        }
    av_free(out_buffer);
    av_frame_free(&frame);
    av_free_packet(packet);
    SDL_CloseAudio();//关闭音频设备
    swr_free(&swrCtx);
    SDL_Quit();
    avcodec_close(pCodeCtx);
    avformat_close_input(&pFormatCtx);
    return 0;
}

