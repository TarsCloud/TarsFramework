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

#include "PlatformInfo.h"
#include "util/tc_clientsocket.h"
#include "util/tc_common.h"
#include "NodeServer.h"
#include <numeric>

NodeInfo PlatformInfo::getNodeInfo() 
{
    NodeInfo tNodeInfo;
    tNodeInfo.nodeName      = getNodeName();
    tNodeInfo.dataDir       = getDataDir();
    TC_Endpoint tEndPoint   = g_app.getAdapterEndpoint("NodeAdapter");
    tNodeInfo.nodeObj       = ServerConfig::Application + "." + ServerConfig::ServerName + ".NodeObj@" + tEndPoint.toString();
    tNodeInfo.endpointIp    = tEndPoint.getHost();
    tNodeInfo.endpointPort  = tEndPoint.getPort();
    tNodeInfo.timeOut       = tEndPoint.getTimeout();
    tNodeInfo.version       = TARS_VERSION+string(".")+NODE_VERSION;

    return tNodeInfo;
}

LoadInfo PlatformInfo::getLoadInfo() 
{
    LoadInfo info;
    info.avg1   = -1.0f;
    info.avg5   = -1.0f;
    info.avg15  = -1.0f;
#if TARGET_PLATFORM_LINUX || TARGET_PLATFORM_IOS

    double loadAvg[3];
    if ( getloadavg( loadAvg, 3 ) != -1 )
    {
        info.avg1   = static_cast<float>( loadAvg[0] );
    // info.avg5   = static_cast<float>( loadAvg[1] );
    // info.avg15  = static_cast<float>( loadAvg[2] );
    }
#else
    string errstr;
    string data = TC_Port::exec("wmic cpu get loadpercentage", errstr);
    if (data.empty()) {
        if (!errstr.empty()) {
            TLOGERROR(errstr << endl);
            return info;
        }
    }

    vector<string> v = TC_Common::sepstr<string>(data, "\n");
    for(auto s : v)
    {
        s = TC_Common::trim(s);
        if(TC_Common::isdigit(s))
        {
            info.avg1 = TC_Common::strto<float>(s); 
            break;
        }
    }

#endif

    _load5.push_back(info.avg1);
    if(_load5.size() > 5)
    {
        _load5.erase(_load5.begin()); 
    }
    info.avg5  = std::accumulate(_load5.begin(),_load5.end(), 0) / _load5.size();

    _load15.push_back(info.avg1);
    if(_load15.size() > 15)
    {
        _load15.erase(_load15.begin());
    }
    info.avg15  = std::accumulate(_load15.begin(),_load15.end(), 0) / _load15.size();
    return  info;
}

const string &PlatformInfo::getNodeName() 
{
	return NodeServer::NODE_ID;
}

string PlatformInfo::getDataDir() 
{
    string sDataDir;
    sDataDir = ServerConfig::DataPath;

    if ( TC_File::isAbsolute(sDataDir) == false)
    {
        char cwd[256] = "\0";
        if ( getcwd( cwd, sizeof(cwd) ) == NULL )
        {
            TLOGERROR("PlatformInfo::getDataDir cannot get the current directory:\n" << endl);
            exit( 0 );
        }
        sDataDir = string(cwd) + FILE_SEP + sDataDir;
    }

    sDataDir = TC_File::simplifyDirectory(sDataDir);
    if ( sDataDir[sDataDir.length() - 1] == FILE_SEP[0] )
    {
        sDataDir = sDataDir.substr( 0, sDataDir.length() - 1 );
    }

    return sDataDir;
}

string PlatformInfo::getDownLoadDir() 
{
    string sDownLoadDir = "";
    try
    {
        sDownLoadDir = g_app.getConfig().get("/tars/node<downloadpath>","");
        if(sDownLoadDir == "")
        {
            string sDataDir       = getDataDir();   
            string::size_type pos =  sDataDir.find_last_of(FILE_SEP);
            if(pos != string::npos)
            {
                sDownLoadDir    = sDataDir.substr(0,pos) + FILE_SEP + "data" + FILE_SEP + "tmp" + FILE_SEP + "download" + FILE_SEP;
            }
        }

        sDownLoadDir = TC_File::simplifyDirectory(sDownLoadDir);
        if(!TC_File::makeDirRecursive( sDownLoadDir ))
        {
            TLOGERROR("getDownLoadDir property `tars/node<downloadpath>' is not set and cannot create dir:"<<sDownLoadDir<<endl);
            exit(-1);
        }
    }
    catch(exception &e)
    {
        TLOGERROR("PlatformInfo::getDownLoadDir "<< e.what() << endl);
        exit(-1);
    }

    return sDownLoadDir;
}


