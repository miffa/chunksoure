
#ifndef _RTMPCLIENT_H
#define _RTMPCLIENT_H

#include "rtmp_sys.h"
#include <stdio.h>

const uint32_t TAG_BUF_LEN = 64*1024;  
const uint32_t DEF_RTMP_URL_LEN = 512;

class RtmpClient
{
public:
    RtmpClient();
    ~RtmpClient();

    int32_t Initialize();
    int32_t SetRtmpLogInfo();
    int32_t SetRtmpLogFile();
    bool ChangeRtmpLog();
    static void* StartThread(void* arg);
    void StartStream();

    int32_t OpenFlvFile(const char* path);
    int32_t Close();
    void SetCloseFlag(bool tf){ is_close_ = tf; }
    bool IsClose(){ return is_close_; }
    int32_t GetRtmpStatus() { return rtmp_->m_read.status;  }

protected:
    int32_t DownloadStream(); 
    int32_t DownloadStreamLoop();

    int32_t HandleData(uint8_t* data, uint32_t len);
    int32_t HandMetaData(uint8_t* data, uint32_t len);
    int32_t HandAudioData(uint8_t* data, uint32_t len);
    int32_t HandVedioData(uint8_t* data, uint32_t len);
        
private:
    //data
    uint8_t rtmp_url_[DEF_RTMP_URL_LEN];
    uint32_t rtmp_status_;
    RTMP *rtmp_;

    FILE* file_;

    AVal hostname_;
    AVal playpath_;
    AVal subscribepath_;
    AVal ushertoken_;

    AVal fullUrl_;
    AVal swfUrl_;
    AVal tcUrl_;
    AVal pageUrl_;
    AVal app_;
    AVal auth_;
    AVal swfHash_;
    uint32_t swfSize_;
    AVal flashVer_;
    AVal sockshost_;
    uint32_t port_;
    uint32_t dseek_;
    int32_t protocol_;
    int retries_;
    int first_;
    long int timeout_;   // timeout connection after 120 seconds

    bool first_audio_tag;
    bool first_vedio_tag;

    time_t log_time_;

    bool is_close_;

    uint8_t tag_buffer_[TAG_BUF_LEN];

    //const
    enum RtmpStatus{ RD_SUCCESS, RD_FAILED, RD_INCOMPLETE, RD_NO_CONNECT };

    static const AVal av_onMetaData;
    static const AVal av_duration; 
    static const AVal av_conn; 
    static const AVal av_token; 
    static const AVal av_playlist; 
    static const AVal av_true; 
    static const uint32_t DEF_BUFTIME = 10*60*60*1000;  //10hours
    static const uint32_t DEF_TIMEOUT = 30; 
    static const uint32_t DEF_ONE_DAY = 86400; 
};

extern RtmpClient* g_rtmpclient;
#endif

