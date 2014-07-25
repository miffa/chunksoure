
#include "cutmachine.h"
#include "common.h"
#include "sessionid.h"
#include "timebuffer.h"
#include "rtmpclient.h"
#include "timebuffer.h"
#include "ichunkmanager.h"
//#include "srcimgmanager.h"

FlvMicrotome::FlvMicrotome():lasttag_offset_(0),supiece_num_(0),piece_num_(0),buffer_ind_(0),header_(CMD_MEDIA_DATA),subpiece_data_len_(0),flv_info_buf_status_(FLV_INIT),flv_info_buf_data_size_(0),data_rate_(0)
{
    header_.len = OUTBUFFER_LEN;
    //outbuffer_ = new uint8_t(OUTBUFFER_LEN);
    for(int pos = 0; pos<BUFFER_INDEX ; ++pos)
    {
        pthread_mutex_init(&buf_lock_[pos], NULL);
        flvbuffer_[pos].Initialize(FLV_INFO_BUF_LEN);
        //flvbuffer_[pos].Initialize(2048);
    }
}


FlvMicrotome::~FlvMicrotome()
{
    //if(outbuffer_ != NULL)
    //{
    //    delete [] outbuffer_;
    //    outbuffer_ = NULL;
    //}
    for(int pos = 0; pos<BUFFER_INDEX ; ++pos)
    {
        pthread_mutex_destroy(&buf_lock_[pos]);
        flvbuffer_[pos].IgnoreAll();
    }
}

bool FlvMicrotome::SetDataRate(uint32_t datarate)
{
    data_rate_ = datarate; 
    //flv_info_buf_status_ |= FLV_DATARATE_OK;
    __sync_fetch_and_or(&flv_info_buf_status_, FLV_DATARATE_OK);
    INFO("SetDataRate ok status is FLV_DATARATE_OK");
    return true;
}

int32_t FlvMicrotome::EncodeFlvHeaderToFlvinfo(uint8_t* meta, uint32_t len)
{
    if( flv_info_buf_status_ & FLV_HEAD_OK)
    {
        FATAL("flvbuffer has add flv header info");
        return 0;
    }

    memcpy(flv_info_buf_, meta, len);
    flv_info_buf_data_size_ = len;
    __sync_fetch_and_or(&flv_info_buf_status_, FLV_HEAD_OK);
    //flv_info_buf_status_ |= FLV_HEAD_OK;
    INFO("EncodeFlvHeaderToFlvinfo ok status is  FLV_HEAD_OK  %u", len);
    return 0;
}

int32_t FlvMicrotome::EncodeMetadataToFlvinfo(uint8_t* meta, uint32_t len)
{
    if( flv_info_buf_status_ & FLV_META_OK)
    {
        FATAL("flvbuffer has add flv metadata info");
        return 0;
    }

    if(flv_info_buf_status_ & FLV_HEAD_OK)
    {
        memcpy(flv_info_buf_+flv_info_buf_data_size_, meta, len);
        flv_info_buf_data_size_ += len;
        //flv_info_buf_status_ |= FLV_META_OK;
        __sync_fetch_and_or(&flv_info_buf_status_, FLV_META_OK);
        INFO("EncodeMetadataToFlvinfo ok status is  FLV_META_OK %u", len);
        return 0;
    }
    FATAL("EncodeMetadataToFlvinfo error");
    return -1;
}

int32_t FlvMicrotome::EncodeFirstVedioTagToFlvinfo(uint8_t* meta, uint32_t len)
{
    if( flv_info_buf_status_ & FLV_1VEDIO_OK)
    {
        FATAL("flvbuffer has add 1st vedio info");
        return 0;
    }

    if( flv_info_buf_status_ & FLV_META_OK )
    {
        memcpy(flv_info_buf_+flv_info_buf_data_size_, meta, len);
        flv_info_buf_data_size_ += len;
        //flv_info_buf_status_ |= FLV_1VEDIO_OK;
        __sync_fetch_and_or(&flv_info_buf_status_, FLV_1VEDIO_OK);
        INFO("Encode1stVedioTagToFlvinfo ok status is  FLV_1VEDIO_OK, data len %d(%u)", flv_info_buf_data_size_, len);
        return 0;
    }
    FATAL("EncodeFirstVedioTagToFlvinfo error");
    return -1;
}

int32_t FlvMicrotome::EncodeFirstAudioTagToFlvinfo(uint8_t* meta, uint32_t len)
{
    if( flv_info_buf_status_ & FLV_1AUDIO_OK)
    {
        FATAL("flvbuffer has add 1st audio info");
        return 0;
    }

    if( flv_info_buf_status_ & FLV_META_OK )
    {
        memcpy(flv_info_buf_+flv_info_buf_data_size_, meta, len);
        flv_info_buf_data_size_ += len;
        //flv_info_buf_status_ |= FLV_1AUDIO_OK;
        __sync_fetch_and_or(&flv_info_buf_status_, FLV_1AUDIO_OK);
        INFO("Encode1stAUDIOToFlvinfo ok status is  FLV_1AUDIO_OK, data len %d(%u)", flv_info_buf_data_size_, len);
        return 0;
    }
    FATAL("EncodeFirstVedioTagToFlvinfo error");
    return -1;
}

bool FlvMicrotome::Initialize()
{
    
#ifdef _FILE_DEBUG_
    flv_file_ = fopen("./flvdata.flv","w");
    if(flv_file_ == NULL)
    {
        FATAL("open debug  file error ");
    }

    flv_head_ = fopen("./flvhead.flv","w");
    if(flv_head_ == NULL)
    {
        FATAL("open debug head  file error ");
    }
    
    is_w_head_ = false;
#endif
    return true;
}

bool FlvMicrotome::RunMachine()
{
    INFO("@@@@@@@@Runmachine ");
    while(1)
    {
        if(CutData()<0)
        {
            FATAL("cut data error");
            return false;
        }
        usleep(100);
    }
    return true;
}

int32_t FlvMicrotome::GetFlvInfo(uint8_t* buf, uint32_t len)
{
    if(len < flv_info_buf_data_size_)
    {
        FATAL("buf is not enough bigger than flv head info");
        return -1;
    }

    uint8_t *ptr = buf;
    int32_t data_len = 0;
    if(flv_info_buf_status_ == FLV_OK)
    {
        //encode datarate + datalen + data
        memcpy(ptr, (char*)&data_rate_, 4);
        ptr += 4;
        data_len += 4;
        memcpy(ptr, (char*)&flv_info_buf_data_size_, 4);
        ptr += 4;
        data_len += 4;
        memcpy(ptr, flv_info_buf_, flv_info_buf_data_size_);
        data_len += flv_info_buf_data_size_;
        INFO(" flv head info data len is %d(meta len %d)", data_len, flv_info_buf_data_size_);
        return data_len;
    }
    else
    {
        INFO("flv head info has not been ready from rtmpclient, please wait");
        return -1;
    }
}

void* FlvMicrotome::RunCutMachine(void* arg)
{
    static_cast<FlvMicrotome*>(arg)->RunMachine();
}

uint32_t FlvMicrotome::EncodeMsgToSrcimg(Subpiece& sdata , uint8_t* buffer, uint32_t len)
{
    memset(buffer, 0, len);
    uint32_t retlen = 0;
    uint8_t* ptr = buffer;  

    //head
    memcpy(ptr, (uint8_t*)&header_, sizeof(SRC_HEAD));   
    //INFO("encode msg to srcimg,  head info len:%d cmd:%0x", header_.len, header_.cmd);
    ptr += sizeof(SRC_HEAD);
    retlen += sizeof(SRC_HEAD);
    //uint32_t datalen = sizeof(Subpiece);
    //memcpy(ptr, (uint8_t*)&datalen, sizeof(uint32_t));   
    //retlen += sizeof(uint32_t);
    //ptr += sizeof(uint32_t);
    memcpy(ptr, (uint8_t*)&sdata, sizeof(Subpiece)); 
    retlen += sizeof(Subpiece);
    return retlen;
}

int32_t FlvMicrotome::AddDataToBuffer(uint8_t* data, uint32_t len){
    //if data len bigger than buffer size ,drop it
    uint32_t ind = buffer_ind_;
    //INFO(">>>>>>>>>>AddData To Buffer[%d]", ind);

    pthread_mutex_lock(&buf_lock_[ind]);
    IOBuffer& flvbuf = flvbuffer_[ind];
    std::queue<uint32_t>& flvbt = flvbytes_[ind];
    if(len > flvbuf._size)
    {
        FATAL("<<<<<<<<data len[%d] is bigger than buffer len [%d]", len, flvbuf._size);
        pthread_mutex_unlock(&buf_lock_[ind]);
        return -1; 
    }

    uint32_t leftsize = flvbuf._size - flvbuf._published;
    //buffer left is not enough for data, drop some old data
    if(leftsize < len )
    {
        //ignore some dataa
        uint32_t ignoresize = 0;
        uint32_t needsize = len - leftsize;;
        while(ignoresize<needsize && !flvbt.empty())
        {
            ignoresize += flvbt.front();
            flvbt.pop();
        }
        EarseData(ind, ignoresize);
        //flvbuf.Ignore(ignoresize);
        //flvbuf.MoveData();
        //INFO("========buffer left size is %d, data len is %d, need ignore some old data[%d]", flvbuf._size - flvbuf._published, len, ignoresize);
    }
    flvbuf.ReadFromBuffer(data, len);
    flvbt.push(len);
    //INFO("<<<<<<<<insert data ok , datalen %d, buffer left size is %d ",len,  flvbuf._size - flvbuf._published);
    pthread_mutex_unlock(&buf_lock_[ind]);
    return len;
}

int32_t FlvMicrotome::EarseData(uint32_t ind, uint32_t size)
{
    IOBuffer& buf = flvbuffer_[ind];
    buf._consumed += size; //ignore data
    if(buf._consumed == buf._published)//if buffer is empty
    {
        buf._consumed = 0;
        buf._published = 0;
    }
    else //not empty , move data
    {
        memcpy(buf._pBuffer, buf._pBuffer + buf._consumed, buf._published - buf._consumed);
        buf._published = buf._published - buf._consumed;
        buf._consumed = 0;
    }
    //INFO("########Earse data buffer[%d]   %d",ind,  size);
    return size;
}

int32_t FlvMicrotome::CutData()
{
    //INFO("}}}}}}}} cut data begin");

#ifdef _FILE_DEBUG_
    if(!is_w_head_ && flv_info_buf_status_ )
    {
        fwrite(flv_info_buf_, flv_info_buf_data_size_, 1, flv_head_);
        fwrite(flv_info_buf_, flv_info_buf_data_size_, 1, flv_file_);
        is_w_head_ = true;
    }
#endif

    //std::vector<int32_t> clientlist;
    //g_srcimg_manager->GetFdFromListByStatus(CLI_DATA_SNDING,clientlist);
    //g_srcimg_manager->GetFdFromListByStatus(CLI_DATA_SNDING, clientlist);

    uint32_t ind = __sync_lock_test_and_set(&buffer_ind_, (buffer_ind_+1)%BUFFER_INDEX);
    
    //INFO("========cut data  buffer[%d]", ind);
    pthread_mutex_lock(&buf_lock_[ind]);
    IOBuffer& flvbuf = flvbuffer_[ind];
    std::queue<uint32_t>& flvbt = flvbytes_[ind];

    uint8_t tmpbuffer[OUTBUFFER_LEN]={'\0'};
    
    //the last piece of data when live stream is over
    /*
    int32_t status = g_rtmpclient->GetRtmpStatus();
    if( ((status == RTMP_READ_EOF) || (status==RTMP_READ_COMPLETE)) && flvbt.empty()) //  rtmp client stream is EOF 
    {
#ifdef _FILE_DEBUG_
        fclose(flv_file_);
        fclose(flv_head_);
#endif
        INFO("ready for sending the last live stream data");
        if(subpiece_data_len_ == 0)
        {
            INFO("last live stream data is empty ");
            return 0;
        }

        PieceBuffer::GetInstance().AddSubpiece(subpiece_);
        int32_t datalen = EncodeMsgToSrcimg(subpiece_, tmpbuffer, OUTBUFFER_LEN);
        if(datalen != OUTBUFFER_LEN)
        {
            FATAL("encode msg to srcimg error");
            return -1;
        }

        for(uint32_t pos=0; pos<clientlist.size(); ++pos)
        {
            SendData(clientlist[pos],tmpbuffer, datalen);
        }
        subpiece_.ReSet();
        subpiece_data_len_ = 0;
        INFO(" the last data send over");
        return 0;
    }*/

    ///process buffer data
    //live stream data before end
    uint8_t* buffer =  GETIBPOINTER(flvbuf);
    int32_t recv_bytes = GETAVAILABLEBYTESCOUNT(flvbuf);//recv bytes
    
#ifdef _FILE_DEBUG_
    fwrite( buffer, recv_bytes, 1, flv_file_);
#endif

    while(recv_bytes > 0)
    {
        //subpiece_data_len_  last subpiece
        uint32_t data_size = recv_bytes+subpiece_data_len_; //left data len now
        //INFO("recv_bytes %d, now subpiece_data_len_:%d ", recv_bytes,  subpiece_data_len_ );
        if(data_size < SUBPIECE_SIZE)//if left size is not enough to compose a subpiece, move left data to subpiece_
        {
            memcpy(subpiece_.data+subpiece_data_len_, buffer, recv_bytes);
            subpiece_data_len_= data_size;
            if(subpiece_.offset == -1)
            {
                subpiece_.offset = lasttag_offset_;
                lasttag_offset_ = 0;
            }

            //clear tag queue
            while(!flvbt.empty())
            {
                flvbt.pop();
            }
            //clear buffer
            flvbuf.Ignore(recv_bytes);
            //INFO("recv_bytes+subpiece_data_len_[%d] smaller than SUBPIECE_SIZE, break", data_size, SUBPIECE_SIZE);
            break;
        }
        
        //ready for cut data, set piece info
        supiece_num_ = (supiece_num_+1) % PIECE_SIZE;
        if(supiece_num_ == 0) //subpiect_id_ loop
        {
            piece_num_ = PieceIdGenerator::GetInstance().GetSerialno(); 
        }

        subpiece_.pieceid = piece_num_;
        subpiece_.subpieceid = supiece_num_;
        
        uint32_t lefttagsize = subpiece_data_len_ + lasttag_offset_; //left size of last tag
        while( lefttagsize < SUBPIECE_SIZE && !flvbt.empty())
        {
            lefttagsize += flvbt.front();
            flvbt.pop();
            //clear buffer
            if(subpiece_.offset == -1)
            {
                subpiece_.offset = lasttag_offset_;
                lasttag_offset_ = 0;
            }
        }
        lasttag_offset_ = lefttagsize - SUBPIECE_SIZE;

        uint32_t readsize = SUBPIECE_SIZE-subpiece_data_len_; //shoule read from buffer
        memcpy(subpiece_.data+subpiece_data_len_, buffer, readsize);
        subpiece_data_len_ = SUBPIECE_SIZE;

        //add data to time buffer
        PieceBuffer::GetInstance().AddSubpiece(subpiece_);
        int32_t datalen = EncodeMsgToSrcimg(subpiece_, tmpbuffer, OUTBUFFER_LEN);
        if(datalen != OUTBUFFER_LEN)
        {
            FATAL("encode msg to srcimg error %d   %d", datalen, OUTBUFFER_LEN);
            return -1;
        }

        //INFO("subpiece info [pid %d, subpid %d, offset %d, datalen %u, time:%ld", subpiece_.pieceid, subpiece_.subpieceid, subpiece_.offset, subpiece_.datalen, time(NULL));
        //INFO("========send data to srcimgclient");
        //Logger::LogHex(_INFO_, tmpbuffer, datalen);  //for_test

        //for(uint32_t pos=0; pos<clientlist.size(); ++pos)
        //{
        //    INFO("send piece data to fd:%d", clientlist[pos]);
        //    SendData(clientlist[pos], tmpbuffer,  datalen);
        //}
        //
        g_chunkmanager->AddSubpiece(tmpbuffer, datalen);
        
#ifdef _FILE_DEBUG_
        fwrite( subpiece_.data, subpiece_data_len_, 1, flv_head_);
#endif
        flvbuf.Ignore(readsize);
        subpiece_.ReSet();
        subpiece_data_len_ = 0;

        buffer =  GETIBPOINTER(flvbuf);
        recv_bytes = GETAVAILABLEBYTESCOUNT(flvbuf);//recv bytesa
    }
    //INFO("}}}}}}}}cut data over");
    pthread_mutex_unlock(&buf_lock_[ind]);
return 0;
}

int32_t FlvMicrotome::EncodeDataRate()
{
    //data_rate_;
}

FlvMicrotome*  g_flvbuffer = NULL;

