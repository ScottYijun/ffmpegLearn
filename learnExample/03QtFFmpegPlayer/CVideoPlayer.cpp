#include "CVideoPlayer.h"
#include <QDebug>


//http://blog.yundiantech.com/?log=blog&id=9
CVideoPlayer::CVideoPlayer()
{

}

void CVideoPlayer::run()
{
    videoDecode();
}

void CVideoPlayer::videoDecode()
{
    char *filePath = "F:\\github\\QtPlayLearn\\win\\mp4\\lasa.mp4";
    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame, *pFrameRGB;
    AVPacket *packet;
    uint8_t *outBuffer;

    static struct SwsContext *img_convert_ctx;
    unsigned int i;
    int videoStream, numBytes;
    int ret, got_picture;
    av_register_all();//��ʼ��ffmpeg ��������������������ñ������ͽ�����
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
    //ѭ��������Ƶ�а���������Ϣ��ֱ���ҵ���Ƶ���͵���
    //�㽫���¼���� ���浽videoStream������
    //������������ֻ������Ƶ��  ��Ƶ���Ȳ�����
    for(i = 0; i < pFormatCtx->nb_streams; ++i)
    {
        qDebug() << "pFormatCtx->streams[" << i << "]->codec->codec_type = " << pFormatCtx->streams[i]->codec->codec_type << endl;
        //0:��Ƶ���� 1:��Ƶ����
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            videoStream = i;
    }
    qDebug() << "videoStream===========" << videoStream << "  pFormatCtx->nb_streams==" << pFormatCtx->nb_streams << endl;
    //���videoStreamΪ-1 ˵��û���ҵ���Ƶ��
    if(videoStream == -1)
    {
        emit signalDecodeError(-3);
        return;
    }
    //���ҽ�����
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    qDebug() << "pCodecCtx->codec_id===========" << pCodecCtx->codec_id << endl;
    //����ʱ���ֵΪ27���鵽ö��ֵ��Ӧ����AV_CODEC_ID_H264 ������H264ѹ����ʽ���ļ���
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(nullptr == pCodec)
    {
        emit signalDecodeError(-4);
        return;
    }

    //�򿪽�����
    if(avcodec_open2(pCodecCtx, pCodec, nullptr) < 0)
    {
        emit signalDecodeError(-5);
        return;
    }

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

    int index = 0;
    while (1)
    {
        if(av_read_frame(pFormatCtx, packet) < 0)
        {
            qDebug() << "index===============" << index;
            break;//������Ϊ��Ƶ��ȡ����
        }
        if(packet->stream_index == videoStream)
        {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if(ret < 0)
            {
                emit signalDecodeError(-6);
                return;
            }
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
                if (index > 10)
                    return; //�������Ǿͱ���10��ͼƬ
            }
        }
        av_free_packet(packet);
    }
    av_free(outBuffer);
    av_free(pFrameRGB);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

}











