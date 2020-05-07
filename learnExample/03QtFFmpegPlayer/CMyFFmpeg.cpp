#include "CMyFFmpeg.h"
#include <QDebug>

//https://blog.csdn.net/yao_hou/article/details/80559161
CMyFFmpeg::CMyFFmpeg()
{
    //av_register_all();//新版这句可以不用
    m_isPlay = false;
}

CMyFFmpeg::~CMyFFmpeg()
{

}

void CMyFFmpeg::openVideo(const char *path)
{
    m_mutex.lock();
    int nRet = avformat_open_input(&m_afc, path, 0, 0);
    qDebug() << "avformat_open_input-==========nRet=====" << nRet;
    //nb_streams打开的视频文件中流的数量，一般nb_streams = 2, 音频流和视频流
    for(int i = 0; i < m_afc->nb_streams; ++i)
    {
        AVCodecContext *acc = m_afc->streams[i]->codec;//分别获取单频流和视频流的解码器
        if(acc->codec_type == AVMEDIA_TYPE_VIDEO)//如果是视频
        {
            m_videoStream = i;
            AVCodec *codec = avcodec_find_decoder(acc->codec_id);//解码器
            //淌有该类型的解码器
            if(!codec)
            {
                m_mutex.unlock();
                return;
            }
            int err = avcodec_open2(acc, codec, nullptr);//打开解码器
            if(err != 0)
            {
                qDebug() << "解码器打开失败==================";
            }
        }
    }
    m_mutex.unlock();
}

void CMyFFmpeg::decodeFrame(const AVPacket *pkt)
{
    m_mutex.lock();
    if(!m_afc)
    {
        m_mutex.unlock();
        return;
    }
    if(nullptr == m_yuv)
    {
        m_yuv = av_frame_alloc();
    }

    AVFrame *frame = m_yuv;//指针传值
    int nRet = avcodec_send_packet(m_afc->streams[pkt->stream_index]->codec, pkt);
    if(0 != nRet)
    {
        m_mutex.unlock();
        return;
    }

    nRet = avcodec_receive_frame(m_afc->streams[pkt->stream_index]->codec, frame);
    if(0 != nRet)
    {
        //失败
        m_mutex.unlock();
        return;
    }
    m_mutex.unlock();
}

AVPacket CMyFFmpeg::readFrame()
{
    AVPacket pkt;
    memset(&pkt, 0, sizeof(AVPacket));

    m_mutex.lock();
    if(!m_afc)
    {
        m_mutex.unlock();
        return pkt;
    }

    int err = av_read_frame(m_afc, &pkt);
    if(0 != err)
    {
        qDebug() << "av_read_frame===err=" << err << "  pkt.size====" << pkt.size;
    }
    else
    {
        qDebug() << "av_read_frame===success  pkt.size====" << pkt.size;
    }
    m_mutex.unlock();
    return pkt;
}

bool CMyFFmpeg::YuvToRGB(char *out, int outweight, int outheight)
{
    m_mutex.lock();
    if(!m_afc || !m_yuv)//像素转换的前提是视频已经打开
    {
        m_mutex.unlock();
        return false;
    }

    AVCodecContext *videoCtx = m_afc->streams[this->m_videoStream]->codec;
    m_cCtx = sws_getCachedContext(m_cCtx, videoCtx->width, videoCtx->height,
                                  videoCtx->pix_fmt, //像素点的格式
                                  outweight, outheight, //目标宽度与高度
                                  AV_PIX_FMT_BGRA, //输出的格式
                                  SWS_BICUBIC,//算法标记
                                  nullptr, nullptr, nullptr);
    if(m_cCtx)
    {
        //sws_getCachedContext成功
    }
    else
    {
        //sws_getCachedContext失败
    }
    uint8_t *data[AV_NUM_DATA_POINTERS] = {0};
    //指针传值，形参的值会被改变，out的值一直在变，所以QImage每次的画面都不一样，画面就这样显示出来了，这应该是整个开发过程最难的点
    data[0] = (uint8_t*)out;
    int linesize[1];
    linesize[0] = outweight * 4;//每一行的转码的宽度
    //返回转码后的高度
    int h = sws_scale(m_cCtx, m_yuv->data, m_yuv->linesize, 0, videoCtx->height,
                      data, linesize);
    m_mutex.unlock();
    return true;
}






