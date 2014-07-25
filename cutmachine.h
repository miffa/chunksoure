#ifndef _FLVBUFFER_H
#define _FLVBUFFER_H

#include "chunk.h"
#include "utils/buffering/iobuffer.h"
#include <queue>
#include <pthread.h>

const uint32_t BUFFER_INDEX = 2;
const uint32_t FLV_INFO_BUF_LEN = 2*64*1024;
const uint32_t OUTBUFFER_LEN = 1049;

class FlvMicrotome
{
public:
    FlvMicrotome();
    ~FlvMicrotome();
    bool Initialize();
    bool RunMachine();
    //interafce for rtmpclient
    int32_t AddDataToBuffer(uint8_t* data, uint32_t len);
    int32_t EncodeFlvHeaderToFlvinfo(uint8_t* data, uint32_t len);
    int32_t EncodeMetadataToFlvinfo(uint8_t* meta, uint32_t len);
    int32_t EncodeFirstVedioTagToFlvinfo(uint8_t* meta, uint32_t len);
    int32_t EncodeFirstAudioTagToFlvinfo(uint8_t* meta, uint32_t len);
    //bool ClearFlvInfo();
    bool SetDataRate(uint32_t datarate);
    int32_t EncodeDataRate();
    
    //cut data and send data
    int32_t CutData();
    //interface for ichunkprocess
    int32_t GetFlvInfo(uint8_t* buf, uint32_t len);
    //
    static void* RunCutMachine(void* arg);
    uint32_t EncodeMsgToSrcimg(Subpiece& data, uint8_t* buffer, uint32_t len);
protected:
    int32_t EarseData(uint32_t ind, uint32_t size);
    int32_t DecodeDataRate(uint8_t* data, uint32_t len);

public:

    uint8_t flv_info_buf_[FLV_INFO_BUF_LEN];
    uint32_t flv_info_buf_data_size_;
    enum FlvInfoBufStatus { FLV_INIT=0, FLV_DATARATE_OK=1, FLV_HEAD_OK=2, FLV_META_OK=4, FLV_1AUDIO_OK=8, FLV_1VEDIO_OK=16, FLV_OK=0x1f};
    uint8_t flv_info_buf_status_;

    Subpiece subpiece_;
    uint32_t subpiece_data_len_;
    SRC_HEAD header_;

    //uint8_t* outbuffer_;

    IOBuffer flvbuffer_[BUFFER_INDEX];
    std::queue<uint32_t> flvbytes_[BUFFER_INDEX];
    pthread_mutex_t buf_lock_[BUFFER_INDEX];
    uint32_t buffer_ind_;

    uint32_t data_rate_;
    
    uint32_t lasttag_offset_;
    uint32_t piece_num_;
    uint8_t supiece_num_;

#ifdef _FILE_DEBUG_
    FILE* flv_file_;
    FILE* flv_head_;
    bool is_w_head_;
#endif

    static const uint32_t PIECE_SIZE = 16;
private:
        
};
extern FlvMicrotome* g_flvbuffer;
#endif

