
#include "sessionid.h"

PieceIdGenerator PieceIdGenerator::instance_;

PieceIdGenerator& PieceIdGenerator::GetInstance()
{
    return instance_;
}

int32_t PieceIdGenerator::GetSerialno()
{
    return (serialno_++) % MAX_SERIALNO;
}
