
#ifndef _TIME_BUFFER_H
#define _TIME_BUFFER_H

#include <list>
#include <vector>
#include <pthread.h>
#include "chunk.h"

class PieceBuffer
{
    public: 
        static PieceBuffer& GetInstance();
        ~PieceBuffer(){ pthread_mutex_destroy(&queue_lock_);};
        int32_t AddSubpiece(Subpiece& piece);
        int32_t FindSubpiece(int32_t pieceid, int32_t subpiece, std::vector<Subpiece>& resendmsg);        
    protected:
        PieceBuffer(){ pthread_mutex_init(&queue_lock_, NULL); };

    private:
        static PieceBuffer instance_;
        struct PieceItem
        {
            Subpiece item;
            time_t time;
        };
        std::list<PieceItem> piece_queue_;
        pthread_mutex_t queue_lock_;
        static const int32_t BUFFER_TIME_INTERVAL = 120; //s
};


#endif
