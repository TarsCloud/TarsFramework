#ifndef __PATCH_DOCKER_H_
#define __PATCH_DOCKER_H_

#include "tars_patch.h"
#include "NodeDescriptor.h"
#include "ServerCommand.h"
#include "util.h"

class DockerPatch : public ServerCommand {
public:
    DockerPatch(const ServerObjectPtr &server, const tars::PatchRequest &request);

    ExeStatus canExecute(std::string &sResult) final;

    int execute(std::string &sResult) final;

private:
    ServerObjectPtr _serverObjectPtr;
    tars::PatchRequest _patchRequest;

    int updatePatchResult(string &sResult);

    StatExChangePtr _pStatExChange;
};


#endif //__PATCH_DOCKER_H_

