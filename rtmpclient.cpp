
#include "rtmpclient.h"
#include "chunk_conf.h"
#include <string>
#include <stdlib.h>
#include <string.h>
#include "cutmachine.h"
#include "./librtmp/log.h"

RtmpClient::RtmpClient()
{
    is_close_ = false;
    first_audio_tag = false;
    first_vedio_tag = false;
    hostname_ = AV_empty;
    playpath_ = AV_empty;
    subscribepath_ = AV_empty;
    ushertoken_ = AV_empty;
    
    fullUrl_ = AV_empty;
    swfUrl_ = AV_empty;
    tcUrl_ = AV_empty;
    pageUrl_ = AV_empty;
    app_ = AV_empty;
    auth_ = AV_empty;
    swfHash_ = AV_empty;
    swfSize_ = 0;
    flashVer_ = AV_empty;
    sockshost_ = AV_empty;
    port_ = -1;
    first_ = 1;
    dseek_ = 0;
    protocol_ = RTMP_PROTOCOL_UNDEFINED;
    retries_ = 0;

    timeout_ = DEF_TIMEOUT;   // timeout connection after 120 seconds
    memset(rtmp_url_, 0, DEF_RTMP_URL_LEN);
    rtmp_ = (RTMP*)malloc(sizeof(RTMP));
    file_ = NULL;
}

RtmpClient::~RtmpClient()
{
   if(rtmp_ != NULL)
   {
        RTMP_Close(rtmp_);
        free(rtmp_);
   };

   if(file_ != NULL)
       fclose(file_);
}

void* RtmpClient::StartThread(void* arg)
{
    static_cast<RtmpClient*>(arg)->StartStream();
    return NULL;
}

int32_t RtmpClient::SetRtmpLogInfo()
{
    RTMP_LogLevel loglevel = RTMP_LOGINFO;
    loglevel = static_cast<RTMP_LogLevel>(ChunkConf::GetChunkConf()->GetInt("loglevel", loglevel));
    RTMP_LogSetLevel(loglevel);
    SetRtmpLogFile();
    return 0;
}

int32_t RtmpClient::SetRtmpLogFile()
{
    char buff[20];
    time_t nowtime = time(NULL);
    strftime(buff, 20, "%Y%m%d", localtime(&nowtime));
    std::string logpath = ChunkConf::GetChunkConf()->GetString("logpath", ".");
    logpath += "/rtmp_session_";
    logpath += buff;
    logpath += ".log";
    FILE* oldfile = file_;
    file_ = fopen(logpath.c_str(), "w+");
    if(file_ == NULL)
    {
        FATAL("rtmp_log file  %s  fopen error", logpath.c_str());
        return -1;
    }
    INFO("rtmp_log file  %s  fopen ok ", logpath.c_str());
    log_time_ = nowtime - nowtime%DEF_ONE_DAY - 8*3600;
    strftime(buff, 20, "%Y%m%d %H%M", localtime(&log_time_));
    INFO("log time is %s", buff);
    RTMP_LogSetOutput(file_);
    if(oldfile != NULL)
        fclose(oldfile);
}

bool RtmpClient::ChangeRtmpLog()
{
    time_t nowtime = time(NULL);
    if( nowtime - log_time_ > DEF_ONE_DAY)
    {
        SetRtmpLogFile();
    }
    return true;
}

int32_t RtmpClient::Initialize()
{

    RTMP_Init(rtmp_); 
    rtmp_->Link.lFlags |= RTMP_LF_LIVE;
    std::string rtmp_str = ChunkConf::GetChunkConf()->GetString("rtmp_url", "");
    if(rtmp_str.empty())
    {
        FATAL("rtmp url is error");
        return -1;
    }

    strncpy((char*)rtmp_url_, rtmp_str.c_str(), rtmp_str.length()+1);
    if(!RTMP_ParseURL((const char*)rtmp_url_, &protocol_,&hostname_, &port_, &playpath_, &app_))
    {
        FATAL(" %s  RTMP_ParseURL error",  rtmp_url_);
        return -1;
    }

    if(hostname_.av_len)
    {

    }
    if(playpath_.av_len)
    {

    }
    if (tcUrl_.av_len == 0)
    {
        tcUrl_.av_len = strlen(RTMPProtocolStringsLower[protocol_]) +
        hostname_.av_len + app_.av_len + sizeof("://:65535/");
        tcUrl_.av_val = (char *) malloc(tcUrl_.av_len);
        if (!tcUrl_.av_val)
            return -1;
        tcUrl_.av_len = snprintf(tcUrl_.av_val, tcUrl_.av_len, "%s://%.*s:%d/%.*s",
                RTMPProtocolStringsLower[protocol_], hostname_.av_len,
                hostname_.av_val, port_, app_.av_len, app_.av_val);
    }
       
    //RTMP_SetupStream(rtmp_, protocol_, &hostname_, port_, &sockshost_, &playpath_, &tcUrl_, &swfUrl_, &pageUrl_,&app_,&auth_, &swfHash_, swfSize_, &flashVer_, &subscribepath_, &ushertoken_, 0,0,true, timeout_);
    RTMP_SetupStream(rtmp_, protocol_, &hostname_, port_, &sockshost_, &playpath_, &tcUrl_, &swfUrl_, &pageUrl_,&app_,&auth_, &swfHash_, swfSize_, &flashVer_, &subscribepath_,  0,0,true, timeout_);
    return 0;
}

void RtmpClient::StartStream()
{
    SetRtmpLogInfo();
    bool reconect_f = false;
BEGIN_STREAM:
    while(!is_close_)
    {
        //socket
        if(reconect_f)
            Initialize();
        RTMP_SetBufferMS(rtmp_, DEF_BUFTIME);
        INFO("RTMP_SetBufferMS ok");

        if(!RTMP_Connect(rtmp_, NULL))
        {
            FATAL("RTMP_Connect  return false");
            rtmp_status_ = RD_NO_CONNECT;
            break;
        }

        INFO("RTMP_Connect OK");

        if(!RTMP_ConnectStream(rtmp_, 0))
        {
            FATAL("RTMP_ConnectStream returns not playing");
            rtmp_status_ = RD_FAILED;
            break;
        }
        INFO("RTMP_ConnectStream ok");

        //down load stream
        do
        {
            ChangeRtmpLog();
            //int32_t ret = DownloadStream();
            int32_t ret = DownloadStreamLoop();
            if(ret > 0)
            {

            }
            else if(ret == 0)
            {
                if (rtmp_->m_read.status == RTMP_READ_COMPLETE)
                {
                    INFO(" >>>>>>>> stream download over  complete");
                    break;
                }
            }
            else
            {   
                FATAL("download stream error,  rtmp connection is lost, need reconnect ");
                //g_flvbuffer->ClearFlvInfo(); clear header info in the flv buffer 
                break;
            }
        } while(!is_close_ && RTMP_IsConnected(rtmp_));

        /*if (rtmp_->m_read.status == RTMP_READ_COMPLETE)
        {
            INFO(" >>>>>>>> exit the stream ");
            rtmp_status_ = RD_SUCCESS;
            break;
        }*/ //reconnect contiue
        
        if(RTMP_IsConnected(rtmp_))
        {
            FATAL(" rtmp -s connected, but read error");
            RTMP_Close(rtmp_);
            reconect_f = true;
        }
        FATAL("try to reconnec  rtmpserver ");
    }// end while(!is_close_)
    
    reconect_f = true;
    RTMP_Close(rtmp_);
    goto BEGIN_STREAM;

    FATAL("rtmp client  thread is out ");
}

int32_t RtmpClient::HandMetaData(uint8_t* body, uint32_t len)
{
    uint32_t datarate = 0;
    /////////////////////////////////
    if((datarate=rtmp_->m_read.m_dataRate) == 0)
    {
        FATAL("data rate is error ");
        return -1;
    }

    ////////////////////////////////
    g_flvbuffer->EncodeMetadataToFlvinfo(body, len); 
    g_flvbuffer->SetDataRate(datarate);
    INFO("=================== data rate is %u metadata len is %u", datarate, len);
    return 0;
}

int32_t RtmpClient::HandAudioData(uint8_t* data, uint32_t len)
{
    if(!first_audio_tag)
    {
        INFO("first audio tag");
        if( g_flvbuffer->EncodeFirstAudioTagToFlvinfo(data, len) < 0)
        {
            FATAL("add first audio tag error");
            return -1;
        }

        first_audio_tag = true;
        INFO("<<<<<<<<DownloadStream first audio tag  ok size %d", len);
        return 0;
    }
    else
    {
        if(g_flvbuffer->AddDataToBuffer(data, len) < 0)
        {
            FATAL("add  audio tag error");
            return -1;
        }
        //INFO("<<<<<<<<DownloadStream Add audio DataToBuffer   ok");
        return 0;
    }

}

int32_t RtmpClient::HandVedioData(uint8_t* data, uint32_t len)
{
    if(!first_vedio_tag)
    {   
        INFO("first vedio tag");
        if( g_flvbuffer->EncodeFirstVedioTagToFlvinfo(data, len) < 0)
        {
            FATAL("add first vedio tag error");
            return -1;
        }

        first_vedio_tag = true;
        INFO("<<<<<<<<DownloadStream first vedio tag ok size %d", len);
        return 0;
    }
    else
    {
        if( g_flvbuffer->AddDataToBuffer(data, len) < 0)
        {
            FATAL("add vedio data error");
            return -1;
        }

        //INFO("<<<<<<<<DownloadStream Add Vedio DataToBuffer  ok");
        return 0;
    }

}

int32_t RtmpClient::DownloadStream()
{
    //INFO(">>>>>>>>DownloadStream  begin ok");
    RTMP* r = rtmp_;
    r->m_read.timestamp = dseek_; 
    int32_t nRead = 0;
fail:
    switch (r->m_read.status) {
        case RTMP_READ_EOF:
            //INFO("<<<<<<<<DownloadStream Read EOF");
            return -1;
        case    RTMP_READ_COMPLETE:
            INFO("<<<<<<<<DownloadStream complete ok");
            return 0;
        case RTMP_READ_ERROR:  /* corrupted stream, resume failed */
            SetSockError(EINVAL);
            INFO("<<<<<<<<DownloadStream sock err ok");
            return -1;
        default:
            break;
    }

    /* first time thru */
    if (!(r->m_read.flags & RTMP_READ_HEADER))  //yml:no header
    {
        INFO("r->m_read.flags not  RTMP_READ_HEADER");
        FlvHeader flvHeader;
        g_flvbuffer->EncodeFlvHeaderToFlvinfo((uint8_t*)&flvHeader, sizeof(FlvHeader));

        while (r->m_read.timestamp == 0)
        {
            nRead = Read_1_Packet(r, (char*)tag_buffer_, TAG_BUF_LEN);
            if( nRead < 0)
            {
                FATAL("%s, Read_1_Packet return smaller than 0", __FUNCTION__);
                goto fail;
            }

            INFO(" read_1_packet nread=  %d", nRead);

            if (r->m_read.dataType == 5)    
                break;
        }
        
        if( HandMetaData(tag_buffer_, nRead)< 0)
        {
            FATAL("handle metadata error");
            return -1;
        }

        r->m_read.flags |= RTMP_READ_HEADER;
        INFO("r->m_read.flags |= RTMP_READ_HEADER ");
    }

    if( (nRead = Read_1_Packet(r, (char*)tag_buffer_, TAG_BUF_LEN)) < 0 )
    {
        r->m_read.status = nRead;
        FATAL("<<<<<<<<DownloadStream read error ok");
        //return -1;
        goto fail;
    }
    INFO(" read_1_packet nread=  %d", nRead);

    uint8_t packet_type = *tag_buffer_;
    int32_t ret = 0;
    switch(packet_type)
    {
    case RTMP_PACKET_TYPE_INFO:
        //handle meta
        //uint32_t datarate = 0;
        //g_flvbuffer->SetDataRate(datarate);
        break;
    case RTMP_PACKET_TYPE_VIDEO:
        if(HandVedioData(tag_buffer_, nRead) < 0)
            ret = -1;
        break;

    case RTMP_PACKET_TYPE_AUDIO:
        if(HandAudioData(tag_buffer_, nRead) < 0 )
            ret = -1;
    default:
        INFO("DownloadStream what are you doing %d", packet_type);
    }
    return 0;

}

int32_t RtmpClient::Close()
{
    RTMP_Close(rtmp_);
    return 0;
}


int32_t RtmpClient::DownloadStreamLoop()
{
    //INFO(">>>>>>>>DownloadStream  begin ok");
    RTMP* r = rtmp_;
    r->m_read.timestamp = dseek_; 
    int32_t nRead = 0;
fail:
    switch (r->m_read.status) {
        case RTMP_READ_EOF:
            //INFO("<<<<<<<<DownloadStream Read EOF");
            return -1;
        case    RTMP_READ_COMPLETE:
            INFO("<<<<<<<<DownloadStream complete ok");
            return 0;
        case RTMP_READ_ERROR:  /* corrupted stream, resume failed */
            SetSockError(EINVAL);
            INFO("<<<<<<<<DownloadStream sock err ok");
            return -1;
        default:
            break;
    }

    /* first time thru */
    if (!(r->m_read.flags & RTMP_READ_HEADER))  //yml:no header
    {
        INFO("r->m_read.flags not  RTMP_READ_HEADER");
        FlvHeader flvHeader;
        g_flvbuffer->EncodeFlvHeaderToFlvinfo((uint8_t*)&flvHeader, sizeof(FlvHeader));

        while (r->m_read.timestamp == 0)
        {
            nRead = Read_1_Packet(r, (char*)tag_buffer_, TAG_BUF_LEN);
            if( nRead < 0)
            {
                FATAL("%s, Read_1_Packet return smaller than 0", __FUNCTION__);
                goto fail;
            }

            INFO(" read_1_packet nread=  %d", nRead);
            HandleData(tag_buffer_, nRead);

            if (r->m_read.dataType == 5)    
                break;
        }
        
        r->m_read.flags |= RTMP_READ_HEADER;
        INFO("r->m_read.flags |= RTMP_READ_HEADER ");
    }

    if( (nRead = Read_1_Packet(r, (char*)tag_buffer_, TAG_BUF_LEN)) < 0 )
    {
        r->m_read.status = nRead;
        FATAL("<<<<<<<<DownloadStream read error ok");
        //return -1;
        goto fail;
    }
    //INFO(" read_1_packet nread=  %d", nRead);
    
    return HandleData(tag_buffer_, nRead);
}

int32_t RtmpClient::HandleData(uint8_t* data, uint32_t len)
{
    uint8_t packet_type = *data;
    int32_t ret = 0;
    switch(packet_type)
    {
    case RTMP_PACKET_TYPE_INFO:
        //handle meta
        if( HandMetaData(data, len)< 0)
        {
            FATAL(" hand meta data error");
            ret = -1;
        }
        break;
    case RTMP_PACKET_TYPE_VIDEO:
        if(HandVedioData(data, len) < 0)
        {
            FATAL(" hand vedio data error");
            ret = -1;
        }
        break;

    case RTMP_PACKET_TYPE_AUDIO:
        if(HandAudioData(data, len) < 0 )
        {
            FATAL(" hand audio error");
            ret = -1;
        }
        break;
    default:
        INFO("DownloadStream what are you doing %d", packet_type);
        ret = -1;
    }
    return ret;
    
}

const AVal RtmpClient::av_onMetaData = AVC("onMetaData");
const AVal RtmpClient::av_duration = AVC("duration");
const AVal RtmpClient::av_conn = AVC("conn");
const AVal RtmpClient::av_token = AVC("token");
const AVal RtmpClient::av_playlist = AVC("playlist");
const AVal RtmpClient::av_true = AVC("true");

RtmpClient * g_rtmpclient = NULL;
