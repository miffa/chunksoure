
#include "ichunkserver.h"
#include <stdio.h>

void SigQuitFunc(void)
{
    g_chunkmanager->Close();
};

void SigPipeFunc(void)
{
    FATAL(" sigpiep  error");
    //g_rtmpclient->Close();
};

void SigIgn(void)
{

};
