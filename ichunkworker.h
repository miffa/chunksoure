
#ifndef CHUNKWORKER_H
#define CHUNKWORKER_H

#include "netio/networker.h"
#include "netio/netio.h"
#include "p2p/p2ptimersmanager.h"
#include "cutmachine.h"
#include <vector>

class ChunkProcess;
class ChunkWorker : public NetWorker
{
public:  
    ChunkWorker();
    ~ChunkWorker();
    virtual BaseProcess* CreateProcess();
    virtual void Start();
    virtual void StartMain();

    bool AddSubpieceData(uint8_t* data, uint32_t len);
    bool GetSubpieceData(uint8_t* buffer, uint32_t len);

    bool AddSrcInfo(int32_t fd, ChunkProcess* process);
    bool EarseSrcInfo(int32_t fd);

    bool AddTimer(P2PTimerEvent* timer);
    void RemoveTimer(P2PTimerEvent* timer);
    void UpdateTimer(P2PTimerEvent* timer);

protected:   
    struct SubPieceData
    {
        uint8_t bin_data[OUTBUFFER_LEN];
        SubPieceData* next;
        SubPieceData();
        ~SubPieceData(){};
        void SetData(uint8_t* data, uint32_t len);
    };
    
    struct SrcimgInfo
    {
        int32_t fd;
        ChunkProcess* process;
        SrcimgInfo()
        {
            fd = -1; process = NULL;
        };
        /*   
        SrcimgInfo(const SrcimgInfo& item)
        {
            fd = item.fd;
            process = item.process;
        }

        SrcimgInfo& operator =(const SrcimgInfo item)
        {
            fd = item.fd;
            process = item.process;

        }*/
    };

    typedef tr1::unordered_map<int32_t, ChunkProcess*> SrcimgProcessMap;    
    typedef SrcimgProcessMap::iterator SrcimgProcessIter;
    SrcimgProcessMap src_process_map_;  
    
    //std::vector<SrcimgInfo*> client_list_; // srcimg client info
    P2PList<SubPieceData>* subpiece_data_list_; //subpiece data that need to send to client which is in client list
    P2PTimersManager *timers_manager_;  //timer, check client that is timeout
private:  

};

#endif
