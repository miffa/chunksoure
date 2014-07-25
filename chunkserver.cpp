
#include "ichunkserver.h"
#include "ichunkworker.h"

int main(int argc , char* argv[])
{
    return ITrunkServer<ChunkWorker>::StartServer();
}
