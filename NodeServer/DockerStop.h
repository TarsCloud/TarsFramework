#ifndef __STOP_DOCKER_H_
#define __STOP_DOCKER_H_

#include "ServerCommand.h"

using namespace std;

class DockerStop : public ServerCommand {
public:
    enum
	{
        STOP_WAIT_INTERVAL = 5,        /*服务关闭等待时间 秒*/
        STOP_SLEEP_INTERVAL = 20000,   /*微妙*/
    };
public:
    DockerStop(const ServerObjectPtr &pServerObjectPtr, bool bUseAdmin = true, bool bByNode = false, bool bGenerateCore = false);

    ExeStatus canExecute(string &sResult);

    int execute(string &sResult);

private:
    ServerObjectPtr _pServerObjectPtr;
    bool _bUseAdmin; //是否使用管理端口停服务
    bool _bByNode;
    bool _bGenerateCore;
    ServerDescriptor _tDesc;
};

#endif
