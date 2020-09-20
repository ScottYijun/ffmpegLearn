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
///由于我们建立的是C++的工程
///编译的时候使用的C++的编译器编译
///而FFMPEG是C的库
///因此这里需要加上extern "C"
///否则会提示各种未定义

int main()
{
    //这里简单的输出一个版本号
#ifdef _WIN64
    cout << "Hello FFmpeg(64bit位)!" << endl;
#elif _WIN32
    cout << "Hello FFmpeg(32bit位)!" << endl;
#endif
    //av_register_all();//ffmpeg 4.2.2已经不需要这个函数了
    unsigned version = avcodec_version();
    cout << "version is:====" << version;
    return 0;
}
