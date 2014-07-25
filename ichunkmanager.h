
#ifndef CHUNKSOCK_H
#define CHUNKSOCK_H
#include "netio/netmanager.h"
class ChunkManager : public NetManager
{
    public:
        bool Initialize();
        bool Start();
        bool AddSubpiece(uint8_t* subpiecedata, uint32_t len);
};

extern ChunkManager* g_chunkmanager;
#endif
