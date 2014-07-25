
#ifndef _FLV_BUFFER_H
#define _FLV_BUFFER_H

class VectorBuffer
{
public:
    bool Initialize(uint32_t len);
    bool WriteDataToBuffer(uint8_t *buf, uint32_t len);
    bool ReadDataToBuffer(uint8_t *buf, uint32_t len);

private:
    uint8_t *buffer_;
    uint32_t size_;
    uint32_t published_;
    uint32_t consumed_;
    uint32_t minChunkSize_;
};


class RingBuffer
{
public:
    bool Initialize(uint32_t len);
    bool WriteDataToBuffer(uint8_t *buf, uint32_t len);
    bool ReadDataFromBuffer(uint8_t *buf, uint32_t len);
    
private:
    uint8_t *buffer_;
    uint32_t size_;
    uint32_t published_;
    uint32_t consumed_;
    uint32_t minChunkSize_;
};

#endif
