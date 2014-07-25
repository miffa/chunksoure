
#ifndef _CHUNK_H
#define _CHUNK_H
#include <stdint.h>
#include <string.h>

const uint32_t SUBPIECE_SIZE = 1024;

enum SourceImageAndChunckCMD {    
    CMD_GET_MEDIA_INFO = 0x101,    
    CMD_MEDIA_DATA = 0x102,    
    CMD_READYED = 0x103,    
    CMD_KEEP_ALIVE = 0x104
};

#pragma pack(1)
typedef struct SRC_HEAD_{    
    uint32_t len;    
    uint32_t cmd;    
    uint8_t  encrypt;    
    SRC_HEAD_():len(0),cmd(0),encrypt(0){}    
    SRC_HEAD_(SourceImageAndChunckCMD mycmd):len(0),cmd(mycmd),encrypt(0){}    
}SRC_HEAD;
typedef SRC_HEAD GetMediaInfo;

typedef struct GetMediaInfoRsp_{    
    SRC_HEAD head;    
    uint32_t bit_rate;    
    uint32_t media_head_size;    
    uint8_t *media_head_data;    
    GetMediaInfoRsp_():head(CMD_GET_MEDIA_INFO),bit_rate(0),media_head_size(0),media_head_data(NULL){} 
}GetMediaInfoRsp;

typedef struct ReceiveData_{    
    SRC_HEAD head;    
    uint32_t lasted_piece_index_;
    uint32_t lasted_subpiece_index_;    
    ReceiveData_():lasted_piece_index_(0),lasted_subpiece_index_(0)  { }       
}ReceiveData;

struct Subpiece
{
    int32_t pieceid;
    int32_t subpieceid;
    int32_t offset;
    uint32_t datalen;
    uint8_t data[SUBPIECE_SIZE];
    Subpiece():pieceid(0),subpieceid(0),offset(-1),datalen(SUBPIECE_SIZE)
    {
        memset(data, 0, SUBPIECE_SIZE);
    }

    void ReSet()
    {
        pieceid = 0;
        subpieceid = 0;
        offset = -1;
        datalen = SUBPIECE_SIZE;
        memset(data, 0, SUBPIECE_SIZE);
    }
};

//typedef Subpiece TransSubpieceInfo;
/*
typedef  struct MediaData_{    
    SRC_HEAD head;    
    TransSubpieceInfo subpieceinfo;    
    uint32_t data_len;    
    uint8_t  *data;    
    MediaData_():head(CMD_MEDIA_DATA){ }       
}MediaData;
*/
typedef SRC_HEAD KeepAlive;


struct FlvHeader
{
    uint8_t sF;
    uint8_t sL;
    uint8_t sV;
    uint8_t ver;
    uint8_t flag;
    //uint32_t headlen;
    uint8_t head1;
    uint8_t head2;
    uint8_t head3;
    uint8_t head4;

    uint32_t firsttag_size;
    FlvHeader() : sF(0x46),sL(0x4c),sV(0x56)
    {
        ver = 0x01;
        flag = 0x00; //no vedio no audio
        flag = 0x05; // has vedio ,has audio
        //headlen = 0x00000009;
        head1=0x00;
        head2=0x00;
        head3=0x00;
        head4=0x09;
        firsttag_size = 0x00000000;
    };

    FlvHeader(uint8_t vert, uint8_t flags) : sF(0x46),sL(0x4c),sV(0x56),ver(vert),flag(flags)
    {
        //headlen = 0x00000009;
        head1=0x00;
        head2=0x00;
        head3=0x00;
        head4=0x09;
        firsttag_size = 0x00000000;
    }
};

#pragma pack()

#endif
