#include "CPlayThread.h"
#include <list>
#include "CMyFFmpeg.h"

using namespace std;


static list<AVPacket> g_videos;//存放视频帧


CPlayThread::CPlayThread()
{

}

CPlayThread::~CPlayThread()
{

}

void CPlayThread::run()
{
    //在子线程里做什么，当然是读视频帧，解码视频了
    //何时读，何时解码呢，在视频打开之后读帧解码, 读帧解码线程要一直运行
    //视频没打开之前线程要阻塞, run,while(1)这是基本套路

    while(1)
    {
        if(!CMyFFmpeg::GetObj()->m_isPlay)
        {
            //调试方便，5微秒后窗口又关闭了，线程继续阻塞，此时可以点击【打开视频按钮】选择视频
            msleep(5);
            continue;
        }
        while (g_videos.size() > 0)
        {
            AVPacket pack = g_videos.front();
            CMyFFmpeg::GetObj()->decodeFrame(&pack);
            av_packet_unref(&pack);
            //解码完成的帧从list前面弹出
            g_videos.pop_front();
        }
        AVPacket pkt = CMyFFmpeg::GetObj()->readFrame();
        if(pkt.size <= 0)
        {
            msleep(10);
        }
        else //读取视频帧成功才加到list里
        {
            g_videos.push_back(pkt);
        }
    }
}





