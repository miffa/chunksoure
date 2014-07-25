
#include "timebuffer.h"
#include "common.h"

PieceBuffer PieceBuffer::instance_;

PieceBuffer& PieceBuffer::GetInstance()
{
    return instance_;
}

int32_t PieceBuffer::AddSubpiece(Subpiece& piece)
{
    PieceItem pitem;
    pitem.time = time(NULL);
    pitem.item = piece;
    pthread_mutex_lock(&queue_lock_);
	
    if(piece_queue_.empty())
    {
        piece_queue_.push_back(pitem);
        pthread_mutex_unlock(&queue_lock_);
        //INFO(" timebuffer first data %d:%d",piece.pieceid, piece.subpieceid );
        return 0;
    }

    int32_t interval = pitem.time - piece_queue_.front().time;  
    if( interval > BUFFER_TIME_INTERVAL)
    {
        piece_queue_.pop_front();
        piece_queue_.push_back(pitem);
        //INFO(" timebuffer is out of 2mins %d:%d size(%d)",piece.pieceid, piece.subpieceid, piece_queue_.size());
    }
    else
    {
        piece_queue_.push_back(pitem);
        //INFO(" timebuffer is in 2mins %d:%d",piece.pieceid, piece.subpieceid );
    }
    pthread_mutex_unlock(&queue_lock_);
    return 0;
}

int32_t PieceBuffer::FindSubpiece(int32_t pieceid, int32_t subpieceid, std::vector<Subpiece>& resendmsg)
{
    bool resendflag = false;
    pthread_mutex_lock(&queue_lock_);
    std::list<PieceItem>::iterator iter = piece_queue_.begin();
    for(; iter!=piece_queue_.end(); ++iter)
    {
        if(resendflag)
        {
            INFO(" resending subpiece is ok too %d:%d",iter->item.pieceid, iter->item.subpieceid );
            resendmsg.push_back(iter->item);
        }
        else
        {
            if(iter->item.pieceid==pieceid && 
                    iter->item.subpieceid==subpieceid)
            {
                resendflag = true;
                resendmsg.push_back(iter->item);
                INFO(" resending subpiece is ok %d:%d",iter->item.pieceid, iter->item.subpieceid );
            }
        }
    }
    pthread_mutex_unlock(&queue_lock_);
    return 0;
}
