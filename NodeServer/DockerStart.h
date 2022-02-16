#ifndef __START_DOCKER_H_
#define __START_DOCKER_H_

#include "ServerCommand.h"
#include "DockerServerToolBox.h"

class DockerStart : public ServerCommand
{
public:
    explicit DockerStart(const ServerObjectPtr &pServerObjectPtr, bool bByNode = false);

    ExeStatus canExecute(string &sResult) final;

    int execute(string &sResult) final;

    bool startNormal(string &sResult);

private:
    bool _bByNode;
    ServerDescriptor _tDesc;
    ServerObjectPtr _pServerObjectPtr;
};

#endif  //__START_DOCKER_H_