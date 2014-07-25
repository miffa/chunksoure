#ifndef _SESSION_H
#define _SESSION_H

#include <stdint.h>
class PieceIdGenerator
{
    public:  
        ~PieceIdGenerator(){};

        static PieceIdGenerator& GetInstance();
        int32_t GetSerialno(); 
    protected:
        PieceIdGenerator(){ serialno_=0; };
        PieceIdGenerator(const PieceIdGenerator&);
        PieceIdGenerator& operator=(const PieceIdGenerator&);
    private:
        static const int32_t MAX_SERIALNO = 2147483647; 
        //static const int32_t MAX_SERIALNO = 0x7fffffffL; 
        int32_t serialno_;
        static PieceIdGenerator instance_;
};

#endif
