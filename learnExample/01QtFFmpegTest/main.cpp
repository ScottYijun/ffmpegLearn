#include <iostream>
#include <stdio.h>

using namespace std;


extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
}
///�������ǽ�������C++�Ĺ���
///�����ʱ��ʹ�õ�C++�ı���������
///��FFMPEG��C�Ŀ�
///���������Ҫ����extern "C"
///�������ʾ����δ����

int main()
{
    //����򵥵����һ���汾��
#ifdef _WIN64
    cout << "Hello FFmpeg(64bitλ)!" << endl;
#elif _WIN32
    cout << "Hello FFmpeg(32bitλ)!" << endl;
#endif
    //av_register_all();//ffmpeg 4.2.2�Ѿ�����Ҫ���������
    unsigned version = avcodec_version();
    cout << "version is:====" << version;
    return 0;
}
