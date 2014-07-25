
#ifndef _CHUNKPROCESS_H
#define _CHUNKPROCESS_H

#include "netio/baseprocess.h"

class ChunkWorker;
//class SrcImageClient;
struct P2PTimerEvent;
int const ENCODE_BUF_LEN = 65535;

class ChunkProcess : public BaseProcess
{
public:
    ChunkProcess(ChunkWorker* worker);
    virtual ~ChunkProcess();

    virtual bool SignalInputData(int32_t recvAmount);

    bool Initialize();
    bool SendMsg(uint8_t* data, uint32_t len);
    bool IsSendData(){ return CLI_DATA_SNDING == srcimg_status_; };

private:  
    bool SrcimgGetHeader(uint8_t*  data);
    bool SrcimgGetFlvData(uint8_t*  data);
    bool SrcimgHeart(uint8_t*  data);
    void UpdateTimer();
    bool IsClose();

private:
    //SrcImageClient * srcimg_client_; 
    enum SrcImageStatus { CLI_ST_ERR = -1, CLI_ST_IDLE, CLI_SOCK_CONNED, CLI_HEAD_SNDED, CLI_DATA_SNDING };
    SrcImageStatus srcimg_status_;
    int32_t fd_flag_;
    uint8_t encodebuffer_[ENCODE_BUF_LEN];
    bool writeState_;
    ChunkWorker* worker_;
    P2PTimerEvent* timer_;
    bool is_add_to_client_list_;
    static const uint32_t SRCIMG_TIMEOUT = 300;
};


#endif
