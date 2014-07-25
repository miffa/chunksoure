
#include "ichunkmanager.h"
#include "ichunkworker.h"
#include "common.h"
#include "netio/networker.h"
bool ChunkManager::Initialize()
{
   return true; 
}

bool ChunkManager::Start()
{
    for (int i = 1; i < (int)worker_.size(); i++)
    {
        pthread_t thread;
        int rs = pthread_create(&thread, NULL, NetWorker::Start, worker_[i]);
        if ( rs )
        {
            FATAL("pthread_create failed, errno is %d", rs);
            return false;
        }
    }
    worker_[0]->StartMain();
    return true;
}

bool ChunkManager::AddSubpiece(uint8_t* data, uint32_t len)
{
    vector<NetWorker*>::iterator iter = worker_.begin();
    for(; worker_.end() != iter; ++iter)
    {
         dynamic_cast<ChunkWorker*>(*iter)->AddSubpieceData(data, len);
    }
}

ChunkManager* g_chunkmanager = NULL;
