#include "CMyFFmpeg.h"
#include <QDebug>

//https://blog.csdn.net/yao_hou/article/details/80559161
CMyFFmpeg::CMyFFmpeg()
{
    //av_register_all();//�°������Բ���
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
    //nb_streams�򿪵���Ƶ�ļ�������������һ��nb_streams = 2, ��Ƶ������Ƶ��
    for(int i = 0; i < m_afc->nb_streams; ++i)
    {
        AVCodecContext *acc = m_afc->streams[i]->codec;//�ֱ��ȡ��Ƶ������Ƶ���Ľ�����
        if(acc->codec_type == AVMEDIA_TYPE_VIDEO)//�������Ƶ
        {
            m_videoStream = i;
            AVCodec *codec = avcodec_find_decoder(acc->codec_id);//������
            //���и����͵Ľ�����
            if(!codec)
            {
                m_mutex.unlock();
                return;
            }
            int err = avcodec_open2(acc, codec, nullptr);//�򿪽�����
            if(err != 0)
            {
                qDebug() << "��������ʧ��==================";
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

    AVFrame *frame = m_yuv;//ָ�봫ֵ
    int nRet = avcodec_send_packet(m_afc->streams[pkt->stream_index]->codec, pkt);
    if(0 != nRet)
    {
        m_mutex.unlock();
        return;
    }

    nRet = avcodec_receive_frame(m_afc->streams[pkt->stream_index]->codec, frame);
    if(0 != nRet)
    {
        //ʧ��
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
    if(!m_afc || !m_yuv)//����ת����ǰ������Ƶ�Ѿ���
    {
        m_mutex.unlock();
        return false;
    }

    AVCodecContext *videoCtx = m_afc->streams[this->m_videoStream]->codec;
    m_cCtx = sws_getCachedContext(m_cCtx, videoCtx->width, videoCtx->height,
                                  videoCtx->pix_fmt, //���ص�ĸ�ʽ
                                  outweight, outheight, //Ŀ������߶�
                                  AV_PIX_FMT_BGRA, //����ĸ�ʽ
                                  SWS_BICUBIC,//�㷨���
                                  nullptr, nullptr, nullptr);
    if(m_cCtx)
    {
        //sws_getCachedContext�ɹ�
    }
    else
    {
        //sws_getCachedContextʧ��
    }
    uint8_t *data[AV_NUM_DATA_POINTERS] = {0};
    //ָ�봫ֵ���βε�ֵ�ᱻ�ı䣬out��ֵһֱ�ڱ䣬����QImageÿ�εĻ��涼��һ���������������ʾ�����ˣ���Ӧ�������������������ѵĵ�
    data[0] = (uint8_t*)out;
    int linesize[1];
    linesize[0] = outweight * 4;//ÿһ�е�ת��Ŀ��
    //����ת���ĸ߶�
    int h = sws_scale(m_cCtx, m_yuv->data, m_yuv->linesize, 0, videoCtx->height,
                      data, linesize);
    m_mutex.unlock();
    return true;
}






