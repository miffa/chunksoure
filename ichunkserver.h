
#ifndef ITRUNKSERVER_H
#define ITRUNKSERVER_H

//chunk header
#include "ichunkmanager.h"
//#include "srcimgmanager.h"
#include "ichunkworker.h"
#include "chunk_conf.h"
//#include "global_var.h"
#include "rtmpclient.h"
#include "cutmachine.h"
//common header
#include "common.h"
#include <string>


extern void SigQuitFunc(void);
extern void SigPipeFunc(void);
extern void SigIgn(void);

template <class N>
class ITrunkServer
{
public:
    // start logserver listenserver config managerthread workerthraed
    static int StartServer()
    {
        int32_t totalCount = getCPUCount();
        int8_t *cpuUse = new int8_t[totalCount];
        int32_t cpuUseLen = totalCount;
        memset(cpuUse, 0, cpuUseLen);

        //daemon
        doDaemon();
        //config init
        if(!ChunkConf::GetChunkConf()->Initialize("./chunk.conf"))
        {   
            FATAL("[%s:%d ] chunk.conf init error", __func__, __LINE__);
            exit(0);
        }

        //sys set
        //set max limit of file
        if ( !setLimitNoFile(/*RLIM_INFINITY*/1024*1024) )
        {
            FATAL("Unable to start daemonize. setLimitNoFile() failed");
            return 0;
        }

        //signal process
        #define I_SIG_IGN (void(*)()) 1
        //installSignal(SIGINT, SigQuitFunc);
        installSignal(SIGHUP, SigIgn);
        installSignal(SIGPIPE, SigIgn);
        installSignal(SIGTERM, SigIgn);
        installSignal(SIGPIPE, SigPipeFunc);
        installSignal(SIGQUIT, SigQuitFunc);
        //log init
        //loglevel logpath 
        int logLevel = _INFO_;
        bool logCorlor = false;
        logLevel = ChunkConf::GetChunkConf()->GetInt("loglevel", logLevel);
        logCorlor = ChunkConf::GetChunkConf()->GetBool("logcorlor", logCorlor);

        std::string log_path = ChunkConf::GetChunkConf()->GetString("log_path", "./");
        INITIALIZE_LOG(logLevel, logCorlor);
        if ( !Logger::LogSetDir(log_path.c_str()) )
        {
            FATAL("Logger::LogSetDir(%s) failed", log_path.c_str());
            return 0;
        }
        //g_srcimg_manager = new SrcimgManager();
        //if(!g_srcimg_manager->Initialize())
        //{
        //    FATAL("srmimage manager init ok");
        //}

        //flv buffer init
        g_flvbuffer = new FlvMicrotome();
        g_flvbuffer->Initialize();

        //rtmp client init
        g_rtmpclient = new RtmpClient();
        if(g_rtmpclient->Initialize())
        {
            FATAL(" rtmp initlize error ");
            delete g_rtmpclient; 
            return 0;
        }

        //chunk_manager init
        g_chunkmanager = new ChunkManager();
        if( !g_chunkmanager->Initialize() )
        {
            FATAL("chunk manager init error");
            return 0;
        }
        //woker init
        int32_t netCount = 1;
        netCount = ChunkConf::GetChunkConf()->GetInt("workthreadnum", netCount);
        //if(netCount> totalCount)
        //{
        //    FATAL("worker thread num is too bigger");
        //    return 0;
        //}

        for (int i = 0; i < netCount; i++)
        {
            ChunkWorker *worker = new N();
            if ( !worker->Initialize(i, g_chunkmanager) )
            {
                FATAL("IChunkWorker initialize failed");
                return 0;
            }
//            for (int j = 0; j < cpuUseLen; j++)
//            {
//                if ( 0 == cpuUse[j] )
//                {
//                    cpuUse[j] = 1;
//                    //worker->cpuID_ = j;
//                    break;
//                }
//            }
            g_chunkmanager->AddWorker(worker);
        }

        std::string hostIP = "0.0.0.0";
        uint16_t hostPort = 5020;
        hostIP = ChunkConf::GetChunkConf()->GetString("hostip", hostIP);
        hostPort = ChunkConf::GetChunkConf()->GetInt("hostport", hostPort);
        if ( !g_chunkmanager->SetupAccpet(hostIP, hostPort) )
        {
            FATAL("Listen %s %d falied", hostIP.c_str(), hostPort);
            return 0;
        }

        //rtmpclient thread
        pthread_t thread;
        pthread_attr_t  attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if( pthread_create(&thread, &attr, RtmpClient::StartThread, g_rtmpclient) < 0)
        {
            FATAL("RtmpClient::StartThread failed");
            return 0;
        }

        //cutmachine thread
        sleep(2);
        if( pthread_create(&thread, &attr, FlvMicrotome::RunCutMachine, g_flvbuffer) < 0)
        {
            FATAL("FlvMicrotome::RunCutMachine failed");
            return 0;
        }
#if 0
        //cutter thread init
        /*vector<int32_t> cpu_ids;
        cpu_ids.clear();
        int flvbuffer_cpu_id = 2;
        cpu_ids.push_back(flvbuffer_cpu_id);
        if ( !setaffinityNp(cpu_ids, thread) )
        {
            FATAL("CpuID:%d setaffinityNp failed.", run_cpu_id);
            exit(0);
        }*/
#endif
        g_chunkmanager->Start();

        g_chunkmanager->Clear();
        delete g_chunkmanager;
        g_chunkmanager = NULL;
        
        g_rtmpclient->Close();
        delete g_rtmpclient;

        delete g_flvbuffer;
        delete[] cpuUse;
        Logger::LogClose();
        return 0;
    };    

};

#endif
