#include "ichunkworker.h"
#include "ichunkprocess.h"
//#include "common.h"

ChunkWorker::SubPieceData::SubPieceData()
{
    memset(bin_data, 0, OUTBUFFER_LEN);
    next = NULL;
}

//ChunkWorker::SubpieceData::~SubPieceData() {}

void ChunkWorker::SubPieceData::SetData(uint8_t* data, uint32_t len)
{
    memcpy(bin_data, data, len);
}

ChunkWorker::ChunkWorker():NetWorker()
{
    SubPieceData* head = new SubPieceData();
    SubPieceData* head_cycle = new SubPieceData();
    subpiece_data_list_ = new P2PList<SubPieceData>(head, head_cycle);
    timers_manager_ = new P2PTimersManager();   
}

ChunkWorker::~ChunkWorker()
{

}

BaseProcess* ChunkWorker::CreateProcess()
{
    return new ChunkProcess(this);
}

bool ChunkWorker::AddSrcInfo(int32_t fd, ChunkProcess* process)
{
    if( process == NULL )
        return false;
    src_process_map_[fd] = process;
    INFO("  @@after insert  map size  %d", src_process_map_.size());
    return true;
}

bool ChunkWorker::EarseSrcInfo(int32_t fd)
{
    src_process_map_.erase(fd);
    
    INFO("  @@ after erase  map size  %d", src_process_map_.size());
    return false;
}

bool ChunkWorker::AddTimer(P2PTimerEvent* timer)
{
    if (timer && !timer->is_add)
    {
        timers_manager_->AddTimer(timer);
        return true;
    }
    return false;
}

void ChunkWorker::RemoveTimer(P2PTimerEvent* timer)
{
    if (timer && timer->is_add)
    {
        timers_manager_->RemoveTimer(timer);
    }
}

void ChunkWorker::UpdateTimer(P2PTimerEvent* timer)
{
    if (timer && timer->is_add)
        timers_manager_->ReaddTimer(timer);
}

void ChunkWorker::Start()
{
    SetThreadID(pthread_self());

    IOInfo *io_info = NULL;
    BaseProcess *process = NULL;
    IOHandler *handler = NULL;

#ifdef _DEBUG
    timespec check_long, now_spec, res_spec;
    clock_gettime(CLOCK_MONOTONIC, &check_long);
    struct timespec tv1, tv2, tv3, tv4;
    uint32_t wait_count = 0;
    uint32_t spdata_count =0;
    while(pIOHandlerManager_->Pulse(3, &wait_count, &tv1, &tv2, &tv3))
#else
    while(pIOHandlerManager_->Pulse(3))
#endif
    {   
        while( io_info = ioInfoList_->GetDataAndCycle() )
        {
            process = CreateProcess();
            if ( process )
            {
                handler = new TCPCarrier(io_info->fd, process);
                handler->SetManager(pIOHandlerManager_);
                ((ChunkProcess*)process)->Initialize();
            }
            else
            {
                close(io_info->fd);
            }
        }

        //deal subpiece info , send to client
        SubPieceData* sdata = NULL;
    #ifdef _DEBUG
        spdata_count = 0;
    #endif
        while(sdata = subpiece_data_list_->GetDataAndCycle())
        {
            SrcimgProcessIter iter = src_process_map_.begin();
            for(; iter!=src_process_map_.end(); ++iter)
            {
                if(iter->second == NULL)
                {
                    FATAL("NULL process in client list");
                    continue;
                }

                if(iter->second->IsSendData())
                {
                    iter->second->SendMsg(sdata->bin_data, OUTBUFFER_LEN);
                    //INFO("send sub piece data to  client");
                }
            }
    #ifdef _DEBUG
            ++spdata_count;
    #endif
        }
        time_t nowtime = time(NULL);
        //do some time check
        //todo:
#ifdef _DEBUG
        clock_gettime(CLOCK_MONOTONIC, &tv4);
        now_spec = tv4;
        timespecsub(&now_spec, &check_long, &res_spec);

        if (0 < res_spec.tv_sec)
        {      
            struct timespec res;
            timespecsub(&tv2, &tv1, &res);
            int32_t wait_ms = res.tv_sec*1000 + res.tv_nsec/1000000;

            timespecsub(&tv3, &tv2, &res);
            int32_t app_ms = res.tv_sec*1000 + res.tv_nsec/1000000;

            timespecsub(&tv4, &tv3, &res);  
            int32_t udp_ms = res.tv_sec*1000 + res.tv_nsec/1000000;

            FATAL("[%s] Long time %ld-%ld, epoll_wait:[%d,%u], app:[%d], subpiecedata:[%d:%u]", 
                    STR(getLocalTimeString("%Y-%m-%d %H:%M:%S", nowtime)),
                    res_spec.tv_sec, res_spec.tv_nsec,
                    wait_ms, wait_count,
                    app_ms, udp_ms, spdata_count
                 );
        }
        check_long = now_spec;
#endif

        //timer check
        timers_manager_->TimeElapsed(nowtime, pIOHandlerManager_);
    }

}

void ChunkWorker::StartMain()
{
    SetThreadID(pthread_self());

    time_t nowtime = time(NULL);
    while(pIOHandlerManager_->Pulse(1000)) 
    {
        nowtime = time(NULL);
        Logger::LogChange(nowtime);
    }
}

bool ChunkWorker::AddSubpieceData(uint8_t* data, uint32_t len)
{
    if(len != OUTBUFFER_LEN)
    {
        FATAL("datalen is not invalid");
        return false;
    }

    SubPieceData *pdata = subpiece_data_list_->GetCycle();
    if (pdata == NULL)
    {
        pdata = new SubPieceData();
    }
    pdata->SetData(data, len);
    subpiece_data_list_->Insert(pdata);
    return true;
}

bool ChunkWorker::GetSubpieceData(uint8_t* buffer, uint32_t len)
{
   
    return true;
}


