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

#include "PropertyServer.h"
#include "PropertyImp.h"
#include "servant/AppCache.h"

void PropertyServer::initialize()
{
    try
    {
        //关闭远程日志
        RemoteTimeLogger::getInstance()->enableRemote("", false);
        RemoteTimeLogger::getInstance()->enableRemote("PropertyPool", true);

        //增加对象
        addServant<PropertyImp>( ServerConfig::Application + "." + ServerConfig::ServerName +".PropertyObj" );

        _insertInterval  = TC_Common::strto<int>(g_pconf->get("/tars/hashmap<insertInterval>","5"));
        if(_insertInterval < 5)
        {
            _insertInterval = 5;
        }

	    _reserveDay = TC_Common::strto<int>(g_pconf->get("/tars/db_reserve<property_reserve_time>", "31"));
	    if(_reserveDay < 2)
	    {
		    _reserveDay = 2;
        }

        //获取业务线程个数
        string sAdapter = ServerConfig::Application + "." + ServerConfig::ServerName + ".PropertyObjAdapter";
        string sHandleNum = "/tars/application/server/";
        sHandleNum += sAdapter;
        sHandleNum += "<threads>";

        int iHandleNum = TC_Common::strto<int>(g_pconf->get(sHandleNum, "50"));
        vector<pair<int64_t, int > > vec;
        vec.resize(iHandleNum);
        
        initHashMap();

        string s("");
        _selectBuffer = getSelectBufferFromFlag(s);

        for(size_t i =0; i < vec.size(); ++i)
        {
            vec[i].first = 0;
            vec[i].second = 0;
        }
        _buffer[_selectBuffer] = vec;

        for(size_t i =0; i < vec.size(); ++i)
        {
            vec[i].first = 0;
            vec[i].second = 1;
        }
        _buffer[!_selectBuffer] = vec;

        TLOGDEBUG("PropertyServer::initialize iHandleNum:" << iHandleNum<< endl);
//        FDLOG("PropertyPool") << "PropertyServer::initialize iHandleNum:" << iHandleNum << endl;

        TLOGDEBUG("PropertyServer::initialize iSelectBuffer:" << _selectBuffer<< endl);
//        FDLOG("PropertyPool") << "PropertyServer::initialize iSelectBuffer:" << _selectBuffer << endl;

        _randOrder = AppCache::getInstance()->get("RandOrder");
        TLOGDEBUG("PropertyServer::initialize randorder:" << _randOrder << endl);

        _reapThread = new PropertyReapThread();
        
        _reapThread->start();

	    _timer.startTimer();
        // _timer.postRepeated(10000, false, std::bind(&PropertyServer::doReserveDb, this, "propertydb", g_pconf));
	    _timer.postCron("0 0 3 * * *", std::bind(&PropertyServer::doReserveDb, this, "propertydb", g_pconf));

        TARS_ADD_ADMIN_CMD_PREFIX("tars.tarsproperty.randorder", PropertyServer::cmdSetRandOrder);
    }
    catch ( exception& ex )
    {
        TLOGERROR("PropertyServer::initialize catch exception:" << ex.what() << endl);
        exit( 0 );
    }
    catch ( ... )
    {
        TLOGERROR("PropertyServer::initialize unknow  exception  catched" << endl);
        exit( 0 );
    }
}


void PropertyServer::doReserveDb(const string path, TC_Config *pconf)
{
	try
	{

		vector<string> dbs = pconf->getDomainVector("/tars/" + path);

		for(auto &db : dbs)
		{
			TC_DBConf tcDBConf;
			tcDBConf.loadFromMap(pconf->getDomainMap("/tars/" + path + "/" + db));

			TC_Mysql mysql(tcDBConf);

			string sql = "select table_name from information_schema.tables where table_schema='" + tcDBConf._database + "' and table_type='base table' order by table_name";

			string suffix = TC_Common::tm2str(TNOW - _reserveDay * 24 * 60 * 60, "%Y%m%d") + "00";

			string tableName = pconf->get("/tars/" + path + "/" + db + "<tbname>") + suffix;

            // TLOGDEBUG(__FUNCTION__ << " tableName: " << tableName<< endl);

			auto datas = mysql.queryRecord(sql);

			for(size_t i = 0; i < datas.size(); i++)
			{
				string name = datas[i]["table_name"];

				if(name < tableName)
				{
                    TLOGDEBUG(__FUNCTION__ << ", drop name:" << name << ", tableName: " << tableName << ", " << (name < tableName) << endl);

					mysql.execute("drop table " + name);
				}
				else
				{
					break;
				}
			}
		}

	}catch(exception &ex)
	{
		TLOGERROR(__FUNCTION__ << " exception: " << ex.what() << endl);
	}
}

string PropertyServer::getRandOrder(void)
{
    return _randOrder;
}

// string PropertyServer::getClonePath(void)
// {
//     return _clonePath;
// }

int PropertyServer::getInserInterv(void)
{
    return _insertInterval;
}

bool PropertyServer::getSelectBuffer(int iIndex, int64_t iInterval)
{
    int64_t iNow = TNOWMS;
    bool bFlag = true;
    vector<pair<int64_t, int> > &vBuffer = _buffer[iIndex];

    for(vector<pair<int64_t, int> >::size_type i=0; i != vBuffer.size(); i++)
    {
        if(vBuffer[i].second != 1)
        {
            //TLOGDEBUG("getSelectBuffer return false|i:" << i << "|ret:" << vBuffer[i].second << endl);
            return false;
        }
    }

    for(vector<pair<int64_t, int> >::size_type i=0; i != vBuffer.size(); i++)
    {
        if((iNow - vBuffer[i].first) < iInterval)
        {
            bFlag = false;
        }
    }

    if(bFlag)
    {
        for(vector<pair<int64_t, int> >::size_type i=0; i != vBuffer.size(); i++)
        {
            vBuffer[i].second = 0;
        }

        TLOGDEBUG("PropertyServer::getSelectBuffer getSelectBuffer end return true" << endl);
        return true;
    }

    TLOGDEBUG("PropertyServer::getSelectBuffer getSelectBuffer end return false" << endl);
    return false;
}

int PropertyServer::getSelectBufferFromFlag(const string& sFlag)
{
    if(sFlag.length()!=0)
    {
        return (TC_Common::strto<int>(sFlag.substr(2,2))/_insertInterval)%2;
    }
    else
    {
        time_t tTime=0;
        string sDate="",flag="";
        getTimeInfo(tTime,sDate,flag);
        return (TC_Common::strto<int>(flag.substr(2,2))/_insertInterval)%2;
    }
}

bool PropertyServer::cmdSetRandOrder(const string& command, const string& params, string& result)
{
    try
    {
        TLOGINFO("PropertyServer::cmdSetRandOrder " << command << " " << params << endl);

        _randOrder = params;

        result = "set RandOrder [" + _randOrder + "] ok";

        AppCache::getInstance()->set("RandOrder",_randOrder);
    }
    catch (exception &ex)
    {
        result = ex.what();
    }
    return true;
}

void PropertyServer::initHashMap()
{
    TLOGDEBUG("PropertyServer::initHashMap begin" << endl);

    int iHashMapNum = TC_Common::strto<int>(g_pconf->get("/tars/hashmap<hashmapnum>","3"));

    TLOGDEBUG("PropertyServer::initHashMap iHashMapNum:" << iHashMapNum << endl);

    _buffNum = iHashMapNum;

    _hashmap = new PropertyHashMap *[2];

    for(int k = 0; k < 2; ++k)
    {
        _hashmap[k] = new PropertyHashMap[iHashMapNum]();
    }

    int iMinBlock       = TC_Common::strto<int>(g_pconf->get("/tars/hashmap<minBlock>","128"));
    int iMaxBlock       = TC_Common::strto<int>(g_pconf->get("/tars/hashmap<maxBlock>","256"));
    float iFactor       = TC_Common::strto<float>(g_pconf->get("/tars/hashmap<factor>","2"));
    int iSize           = TC_Common::toSize(g_pconf->get("/tars/hashmap<size>"), 1024*1024*256);

    TLOGDEBUG("PropertyServer::initHashMap init multi hashmap begin..." << endl);

    for(int i = 0; i < 2; ++i)
    {
        for(int k = 0; k < iHashMapNum; ++k)
        {
            string sFileConf("/tars/hashmap<file");
            string sFileDefault("hashmap");

            sFileConf += TC_Common::tostr(i);
            sFileConf += TC_Common::tostr(k);
            sFileConf += ">";

            sFileDefault += TC_Common::tostr(i);
            sFileDefault += TC_Common::tostr(k);
            sFileDefault += ".txt";

            string sHashMapFile = ServerConfig::DataPath + "/" + g_pconf->get(sFileConf, sFileDefault);

            string sPath        = TC_File::extractFilePath(sHashMapFile);
            
            if(!TC_File::makeDirRecursive(sPath))
            {
                TLOGERROR("cannot create hashmap file " << sPath << endl);
                exit(0);
            }

            try
            {
                TLOGINFO("initDataBlockSize size: " << iMinBlock << ", " << iMaxBlock << ", " << iFactor << endl);

                _hashmap[i][k].initDataBlockSize(iMinBlock,iMaxBlock,iFactor);

#if TARGET_PLATFORM_IOS
	            _hashmap[i][k].create(new char[iSize], iSize);
#elif TARGET_PLATFORM_WINDOWS
	            _hashmap[i][k].initStore(sHashMapFile.c_str(), iSize);
#else
               key_t key = tars::hash<string>()(ServerConfig::LocalIp + "-" + sHashMapFile);

               RemoteNotify::getInstance()->report("shm key:" + TC_Common::tostr(key) + ", size:" + TC_Common::tostr(iSize), false);

               _hashmap[i][k].initStore(key, iSize);
#endif

                TLOGINFO("\n" <<  _hashmap[i][k].desc() << endl);
            }
            catch(TC_HashMap_Exception &e)
            {
	            RemoteNotify::getInstance()->report(string("init error: ") + e.what(), false);

	            TC_Common::msleep(100);

	            TC_File::removeFile(sHashMapFile,false);
                throw runtime_error(e.what());
            }
            
        }
    }

    TLOGDEBUG("PropertyServer::initHashMap init multi hashmap end..." << endl);
}

void PropertyServer::destroyApp()
{
    if(_reapThread)
    {
        delete _reapThread;
        _reapThread = NULL;
    }

    for(int i = 0; i < 2; ++i)
    {
        if(_hashmap[i])
        {
            delete [] _hashmap[i];
        }
    }

    if(_hashmap)
    {
        delete [] _hashmap;
    }

    TLOGDEBUG("PropertyServer::destroyApp ok" << endl);
}

void PropertyServer::getTimeInfo(time_t &tTime,string &sDate,string &sFlag)
{
     //3G统计要求
    //loadIntev 单位为分钟 
    string sTime,sHour,sMinute;
    time_t t    = TNOW;
    t           = (t/(_insertInterval*60))*_insertInterval*60;    //要求必须为Intev整数倍
    tTime       = t;
    t           = (t%3600 == 0?t-60:t);                                 //要求将9点写作0860  
    sTime       = TC_Common::tm2str(t,"%Y%m%d%H%M");
    sDate       = sTime.substr(0,8);
    sHour       = sTime.substr(8,2);
    sMinute     = sTime.substr(10,2);
    sFlag       = sHour +  (sMinute=="59"?"60":sMinute);                //要求将9点写作0860     
}

