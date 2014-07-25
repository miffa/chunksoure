
#include "common.h"
#include "netio/netio.h"
#include "ichunkprocess.h"
#include "chunk.h"
#include "cutmachine.h"
#include "timebuffer.h"
#include "ichunkworker.h"
//#include "srcimgmanager.h"


//ChunkProcess::ChunkProcess(ChunkWorker* worker):BaseProcess(),srcimg_client_(NULL),writeState_(false)
ChunkProcess::ChunkProcess(ChunkWorker* worker):BaseProcess(),worker_(worker),
                                                timer_(NULL), srcimg_status_(CLI_SOCK_CONNED),writeState_(false)
{
    fd_flag_ = -1;
    INFO("new client is arriving ");
    memset(encodebuffer_, 0, ENCODE_BUF_LEN);
}

ChunkProcess::~ChunkProcess()
{
    INFO("@@@@@@  ~chunkprocess");
    worker_->EarseSrcInfo(fd_flag_);
    if(timer_ != NULL)
    {
        worker_->RemoveTimer(timer_);
        delete timer_;
        timer_ = NULL;
    }
}

bool ChunkProcess::Initialize()
{
    fd_flag_ = _ioHandler->GetFd();
    INFO("@@@@  %d", fd_flag_);
    worker_->AddSrcInfo(fd_flag_, this);
    timer_ = new P2PTimerEvent(this, SRCIMG_TIMEOUT);
    if(timer_ == NULL)
        return false;
    return worker_->AddTimer(timer_);
}

void ChunkProcess::UpdateTimer()
{
    time_t now = time(0);
    if (now > timer_->heart)
    {
        timer_->heart = now;
        worker_->UpdateTimer(timer_);
    }
}

bool ChunkProcess::SignalInputData(int32_t recvAmount)
{
    const uint32_t k_headsize = sizeof(SRC_HEAD);
    uint8_t * buffer =  GETIBPOINTER(_inPutBuffer);
    uint32_t recv_bytes = GETAVAILABLEBYTESCOUNT(_inPutBuffer);
    while( recv_bytes > 0)
    {
        if(k_headsize > recv_bytes)
        {
            WARN("[%s] recv bytes < HEAD SIZE ", __func__);
            return true;
        }
        SRC_HEAD * head = (SRC_HEAD*) buffer;
        if(recv_bytes < head->len)
        {
            WARN("[%s] recv bytes < pkglen", __func__);
            return true;
        }
        
        int32_t ret = 0;
        INFO("head cmd 0x%x", head->cmd);
        switch( head->cmd )
        {
            case CMD_GET_MEDIA_INFO:
                ret = SrcimgGetHeader(buffer);
                break;
            case CMD_MEDIA_DATA:
            case CMD_READYED:
                ret = SrcimgGetFlvData(buffer);
                break;
            case CMD_KEEP_ALIVE:
                ret = SrcimgHeart(buffer);
                break;
            default:
                FATAL("error cmd 0x%x", head->cmd);
                return false; //close connection
        }
        
        if(!ret) return false; //close connection
        
        INFO("process message ok,  ignore data size is %d", head->len);
        _inPutBuffer.Ignore(head->len);

        buffer = GETIBPOINTER(_inPutBuffer);
        recv_bytes = GETAVAILABLEBYTESCOUNT(_inPutBuffer);//recv bytes
        //INFO("recv bytes:%d", recv_bytes);
    }

    UpdateTimer();
    return true;
}


bool ChunkProcess::SrcimgGetHeader(uint8_t* data)
{
    INFO("get header begin");
    // get header info and metadata
    SRC_HEAD * head = (SRC_HEAD*) data;

    uint8_t* ptr = encodebuffer_;
    //encode head
    memcpy(ptr, head, sizeof(SRC_HEAD));
    ptr += sizeof(SRC_HEAD);
    //encode body
    int32_t datalen = g_flvbuffer->GetFlvInfo(ptr,ENCODE_BUF_LEN );  
    if(datalen < 0)
    {
        SendMsg(encodebuffer_, 1);//send err response 
        FATAL("GetFlvbuf error");
        return false;
    }
    
    //memcpy(ptr, (char*)&datalen, sizeof(uint32_t));
    
    //set new head len
    SRC_HEAD* newhead = (SRC_HEAD*)encodebuffer_;
    newhead->len = datalen + sizeof(SRC_HEAD);
    if(!SendMsg(encodebuffer_, newhead->len))
    {
        FATAL("send FLV header error");
        return false;
    }
    
    //set status
    srcimg_status_ = CLI_HEAD_SNDED;
    INFO("SrcimgGetHeader msg send ok, head len %d ", newhead->len);
    return true;
}

bool ChunkProcess::SrcimgGetFlvData(uint8_t* data)
{
    //response 
    INFO(" get flv data begin");
    ReceiveData * rec_data = (ReceiveData*) data;
    if(rec_data->lasted_piece_index_ != 0)
    {
        INFO(" srcimg client reconnect ");
        std::vector<Subpiece> resendmsg;
        PieceBuffer::GetInstance().FindSubpiece(rec_data->lasted_piece_index_, rec_data->lasted_subpiece_index_, resendmsg);
        if(!resendmsg.empty())
        {
            INFO(" resend subpiece %d", resendmsg.size());
            SRC_HEAD head(CMD_MEDIA_DATA);
            head.len = sizeof(Subpiece) + sizeof(uint32_t) + sizeof(SRC_HEAD);
            for(std::vector<Subpiece>::iterator iter = resendmsg.begin(); 
                    iter!=resendmsg.end(); ++iter)
            {
                // 
                int32_t datalen = g_flvbuffer->EncodeMsgToSrcimg(*iter, encodebuffer_, ENCODE_BUF_LEN);
                uint8_t * ptr = encodebuffer_;
                memset(ptr, 0, ENCODE_BUF_LEN);
                memcpy(ptr, (uint8_t*)&head, sizeof(head));
                ptr += sizeof(SRC_HEAD);
                uint32_t pdatalen = sizeof(Subpiece);
                memcpy(ptr, (uint8_t*)&pdatalen, sizeof(uint32_t));
                ptr += sizeof(uint32_t);
                memcpy(ptr, (uint8_t*)&(*iter), sizeof(Subpiece));
                SendMsg(encodebuffer_, datalen);
            }
        }
    }
    else
    {
        INFO("srcimg client get flv data from header");
    }
    //set status
    srcimg_status_ = CLI_DATA_SNDING;
    return true;
}

bool ChunkProcess::SrcimgHeart(uint8_t* data)
{
    //info
    //INFO("[%s] heart beat %d", _ioHandler->GetFd());
    return true;
}

bool ChunkProcess::IsClose()
{
    if (0 == GETAVAILABLEBYTESCOUNT(_outPutBuffer))
        writeState_ = false;

    return _isClose;
}

bool ChunkProcess::SendMsg(uint8_t *buf, uint32_t len)
{
    if ( 0x200000/* 2M */ < GETAVAILABLEBYTESCOUNT(_outPutBuffer) + len )
    {
        WARN("The out-buffer and date-len( %u+%u ) greater than 2M", 
                GETAVAILABLEBYTESCOUNT(_outPutBuffer), len);
        return false;
    }

    if ( !_outPutBuffer.ReadFromBuffer(buf, len) )
    {
        WARN("The ReadFromBuffer failed, outbuffer-len:%u, date-len:%u", 
                GETAVAILABLEBYTESCOUNT(_outPutBuffer), len);
        return false;
    }

    int32_t write_amount = 0;
    if ( !_outPutBuffer.WriteToTCPFd(_ioHandler->GetFd(), 0x800000/* 8M */, write_amount) )
    {
        return false;
    }
    //INFO("send msg  WriteToTCPFd ok ");

    if ( 0 == GETAVAILABLEBYTESCOUNT(_outPutBuffer) )
    {
        return true;
    }

    if (!writeState_)
    {
        _ioHandler->GetManager()->EnableWriteData(_ioHandler);
        writeState_ = true;
    }
    return true;
}
