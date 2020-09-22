#include "CPlayThread.h"
#include <list>
#include "CMyFFmpeg.h"

using namespace std;


static list<AVPacket> g_videos;//�����Ƶ֡


CPlayThread::CPlayThread()
{

}

CPlayThread::~CPlayThread()
{

}

void CPlayThread::run()
{
    //�����߳�����ʲô����Ȼ�Ƕ���Ƶ֡��������Ƶ��
    //��ʱ������ʱ�����أ�����Ƶ��֮���֡����, ��֡�����߳�Ҫһֱ����
    //��Ƶû��֮ǰ�߳�Ҫ����, run,while(1)���ǻ�����·

    while(1)
    {
        if(!CMyFFmpeg::GetObj()->m_isPlay)
        {
            //���Է��㣬5΢��󴰿��ֹر��ˣ��̼߳�����������ʱ���Ե��������Ƶ��ť��ѡ����Ƶ
            msleep(5);
            continue;
        }
        while (g_videos.size() > 0)
        {
            AVPacket pack = g_videos.front();
            CMyFFmpeg::GetObj()->decodeFrame(&pack);
            av_packet_unref(&pack);
            //������ɵ�֡��listǰ�浯��
            g_videos.pop_front();
        }
        AVPacket pkt = CMyFFmpeg::GetObj()->readFrame();
        if(pkt.size <= 0)
        {
            msleep(10);
        }
        else //��ȡ��Ƶ֡�ɹ��żӵ�list��
        {
            g_videos.push_back(pkt);
        }
    }
}





