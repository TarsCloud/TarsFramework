/**
 * Tencent is pleased to support the open source community by making Tars available.
 *
 * Copyright (C) 2016THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software distributed 
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the 
 * specific language governing permissions and limitations under the License.
 */

#ifndef __PATCH_COMMAND_H_
#define __PATCH_COMMAND_H_

#include "SingleFileDownloader.h"
#include "tars_patch.h"
#include "NodeDescriptor.h"
#include "RegistryProxy.h"
#include "util/tc_file.h"
#include "util/tc_md5.h"
#include "ServerCommand.h"
#include "NodeRollLogger.h"
#include "CommandStop.h"
#include "util.h"

class CommandPatch : public ServerCommand
{
public:
    CommandPatch(const ServerObjectPtr & server, const std::string & sDownloadPath, const tars::PatchRequest & request);

    ExeStatus canExecute(std::string &sResult);

    int execute(std::string &sResult);

    int updatePatchResult(std::string &sResult);

    int download(const std::string & sRemoteTgzPath, const std::string & sLocalTgzPath, const std::string & sShortFileName, const std::string& reqMd5, std::string & sResult);

    static string getOsType();

    //是否检查操作系统类型
    static bool checkOsType();

private:
    int backupfiles(std::string &sResult);

    /**
     * 备份5次bin目录bin
     */ 
    int backupBinFiles(const string &srcPath, const string &destPath, std::string &sResult);

    /**
     * 恢复指定的备份文件
     */ 
    int restoreFiles(const string &existFile, const string &destPathBak);

private:
    ServerObjectPtr     _serverObjectPtr;

    tars::PatchRequest   _patchRequest;

    //本地存放tgz的目录
    std::string         _localTgzBasePath;

    //本地解压的文件目录
    std::string         _localExtractBasePath;

    StatExChangePtr     _statExChange;
};

class PatchDownloadEvent : public DownloadEvent
{
public:
    PatchDownloadEvent(ServerObjectPtr pServerObjectPtr)
    : _serverObjectPtr(pServerObjectPtr)
    {}

    virtual void onDownloading(const FileInfo &fi, int pos)
    {
        if (_serverObjectPtr && fi.size > 0)
        {
            _serverObjectPtr->setPatchPercent((int)(pos*100.0/fi.size));
        }
    }

private:
    ServerObjectPtr     _serverObjectPtr;
};

#endif

