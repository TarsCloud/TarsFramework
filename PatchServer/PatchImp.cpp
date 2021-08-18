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

#include "util/tc_file.h"
#include "util/tc_config.h"
#include "util/tc_md5.h"
#include "servant/RemoteLogger.h"
#include "PatchImp.h"
#include "PatchCache.h"
#include "PatchServer.h"

extern PatchCache g_PatchCache;

struct LIST
{
    LIST(const string& dir, vector<FileInfo> &vf) : _dir(dir), _vf(vf)
    {
    }

    void operator()(const string &file)
    {
        //普通文件才同步, 连接文件有效
        if(tars::TC_File::isFileExistEx(file, S_IFREG))
        {
            FileInfo fi;
            fi.path     = file.substr(_dir.length());
            fi.size     = tars::TC_File::getFileSize(file);
            fi.canExec  = tars::TC_File::canExecutable(file);
            fi.md5      = tars::TC_MD5::md5file(file);

            _vf.push_back(fi);

            TLOGDEBUG("LIST path:" << fi.path << "|file:" << file << "|size:" << fi.size << "|exec:" << (fi.canExec?"true":"false") << endl);
        }
    }

    string              _dir;
    vector<FileInfo>    &_vf;
};

PatchImp::PatchImp()
: _size(1024*1024)
{
}

void PatchImp::initialize()
{
    try
    {
        _directory       = TC_File::simplifyDirectory((*g_conf)["/tars<directory>"]);
        _uploadDirectory = TC_File::simplifyDirectory((*g_conf)["/tars<uploadDirectory>"]);
        _size            = TC_Common::toSize(g_conf->get("/tars<size>", "1M"), 1024*1024);
    }
    catch(exception &ex)
    {
        TLOGDEBUG("PatchImp::initialize directory must not be empty." << endl);
        exit(0);
    }

    if(!tars::TC_File::isAbsolute(_directory))
    {
        TLOGDEBUG("PatchImp::initialize directory must be absolute directory path." << endl);
        exit(0);
    }

    TLOGDEBUG("PatchImp::initialize patch dirtectory:" << _directory  << "|uploadDirectory:" << _uploadDirectory << "|size:" << _size << endl);
}


/************************************************************************************************** 
 ** 对外接口，普通下载接口
 **
 **/
int PatchImp::listFileInfo(const string &path, vector<FileInfo> & vf, TarsCurrentPtr current)
{
    TLOGDEBUG("PatchImp::listFileInfo ip:" << current->getHostName()  << "|path:" << path << endl);

    string dir = tars::TC_File::simplifyDirectory(_directory + FILE_SEP + path);

    int ret = __listFileInfo(dir, vf);

    stringstream ss;
    tars::TarsDisplayer ds(ss);
    ds.displaySimple(vf, false);

    TLOGDEBUG("PatchImp::listFileInfo ip:" << current->getHostName()  << "|path:" << path << "|dir:" << dir << "|str:" << ss.str() << endl);

    return ret;
}

int PatchImp::download(const string & file, int pos, vector<char> & vb, TarsCurrentPtr current)
{
    TLOGDEBUG("PatchImp::download ip:" << current->getHostName()  << "|file:" << file << "|pos:" << pos << endl);

    string path = tars::TC_File::simplifyDirectory(_directory + FILE_SEP + file);
    
    int iRet = -1;

    if (iRet < 0)
    {
        iRet = __downloadFromMem (path, pos, vb);
    }

    if (iRet < 0)
    {
        iRet = __downloadFromFile(path, pos, vb);
    }

    return iRet;
}

int PatchImp::preparePatchFile(const string &app, const string &serverName, const string &patchFile, TarsCurrentPtr current)
{
    string upfile = _uploadDirectory + FILE_SEP + app + FILE_SEP + serverName + FILE_SEP + patchFile;
    string dstDirectory = _directory + "/TARSBatchPatching/" + app + FILE_SEP + serverName;
    string dstfile = dstDirectory + FILE_SEP + app +"." + serverName + ".tgz";

    TLOGDEBUG("PatchImp::preparePatchFile upfile:" << upfile << ", dstfile:" << dstfile << endl);

    bool succ = TC_File::makeDirRecursive(dstDirectory);
    if (!succ)
    {
        TLOGERROR("PatchImp::preparePatchFile makeDirRecursive path:" << dstDirectory << "|error!" << endl);
        return -2;
    }

    if (!TC_File::isFileExist(upfile)) 
    {
        TLOGERROR("PatchImp::preparePatchFile isFileExist file:" << upfile << "|not exist!" << endl);
        return -1;
    }

    TC_File::copyFile(upfile, dstfile, true);

    return 0;
}

int PatchImp::deletePatchFile(const string & app, const string & serverName, const string & patchFile, TarsCurrentPtr current)
{
	string upfile = _uploadDirectory + FILE_SEP + app + FILE_SEP + serverName + FILE_SEP + patchFile;

	TLOGDEBUG("PatchImp::deletePatchFile upfile:" << upfile << endl);

	return TC_File::removeFile(upfile, false);
}

/**************************************************************************************************
 *  发布服务内部使用函数
 */
int PatchImp::__listFileInfo(const string &path, vector<FileInfo> &vf)
{
    TLOGDEBUG("PatchImp::__listFileInfo patch:" << path << endl);

    if (path.find("..") != string::npos)
    {
        return -1;
    }

    if(!tars::TC_File::isFileExistEx(path, S_IFDIR))
    {
        //是文件
        FileInfo fi;
        fi.path     = tars::TC_File::extractFileName(path);
        fi.size     = tars::TC_File::getFileSize(path);
        fi.canExec  = tars::TC_File::canExecutable(path);
        fi.md5      = tars::TC_MD5::md5file(path);

        vf.push_back(fi);

        return 1;
    }
    else
    {
        //目录
        vector<string> files;

        tars::TC_File::listDirectory(path, files, true);

        for_each(files.begin(), files.end(), LIST(path, vf));

        return 0;
    }

    return 0;
}


int PatchImp::upload(const std::string & app,const std::string & serverName,const tars::FileContent & content, tars::CurrentPtr current)
{
    LOG->debug() << __FUNCTION__ << "|" << current->getIp() << "|" << content.md5 << "|" << content.name
                 << "|" << content.firstChunk << "|" << content.lastChunk << "|" << content.content.size() << "|" << endl;

    string uploadDir = _uploadDirectory + FILE_SEP + app + FILE_SEP + serverName;

    if (!TC_File::makeDirRecursive(uploadDir)) {
        TLOGERROR("upload, error mkdir: " << uploadDir << endl);
        return -1;
    }

    ofstream writer;

    string uploadFile = uploadDir + FILE_SEP + content.name;

    if (content.firstChunk) {
        writer.open(uploadFile, ios_base::binary);
    } else {
        writer.open(uploadFile, ios_base::app | ios_base::binary);
    }
    if (!writer.is_open()) {
        TLOGERROR("upload, open file error: " << strerror(errno) << endl);
        return -2;
    }

    writer.write((char*)content.content.data(), content.content.size());

    writer.close();

    if (content.lastChunk) {
        string md5 = TC_MD5::md5file(uploadFile);

        if (md5 != content.md5) {
            TLOGERROR("upload, file md5 error!" << uploadFile << ", " << md5 << "!=" << content.md5 << endl);
            return -3;
        }
    }

    return 0;
}

int PatchImp::__downloadFromMem (const string & file, size_t pos, vector<char> & vb)
{
    TLOGDEBUG("PatchImp::__downloadFromMem file:" << file << "|pos:" << pos << "|size:" << _size << endl);

    pair<char *, size_t> mem;
    if (g_PatchCache.load(file, mem) != 0)
    {
        TLOGERROR("PatchImp::__downloadFromMem file:" << file << "|pos:" << pos << "|LoadFile error" << endl);
        return -1;
    }

    if (pos >= mem.second)
    {
        TLOGDEBUG("PatchImp::__downloadFromMem file:" << file << "|pos:" << pos << "|to tail ok" << endl);

        g_PatchCache.release(file);

        return 1;
    }

    const size_t sizeBuf = mem.second - pos >= _size ? _size : mem.second - pos;

    TLOGDEBUG("PatchImp::__downloadFromMem file:" << file << "|pos:" << pos << "|sizeBuf:" << sizeBuf << endl);
    
    vb.resize(sizeBuf);

    memcpy((char *)&vb[0], mem.first + pos, sizeBuf);
    
    g_PatchCache.release(file);

    return 0;
}


int PatchImp::__downloadFromFile(const string & file, size_t pos, vector<char> & vb)
{
    TLOGDEBUG("PatchImp::__downloadFromFile file:" << file << "|pos:" << pos << "|size:" << _size << endl);

    FILE * fp = fopen(file.c_str(), "rb");
    if (fp == NULL)
    {
        TLOGERROR("PatchImp::__downloadFromFile file:" << file << "|pos:" << pos << "|open file error:" << strerror(errno) << endl);
        return -1;
    }

    //从指定位置开始读数据
    if (fseek(fp, pos, SEEK_SET) == -1)
    {
        fclose(fp);

        TLOGERROR("PatchImp::__downloadFromFile file:" << file << "|pos:" << pos << "|fseek error:" << strerror(errno) << endl);
        return -2;
    }

    //开始读取文件
    vb.resize(_size);
    size_t r = fread((void*)(&vb[0]), 1, _size, fp);
    if (r > 0)
    {
        //成功读取r字节数据
        vb.resize(r);

        fclose(fp);

        TLOGDEBUG("PatchImp::__downloadFromFile file:" << file << "|pos:" << pos << "|r:" << r << endl);

        return 0;
    }
    else
    {
        //到文件末尾了
        if (feof(fp))
        {
            fclose(fp);

            TLOGDEBUG("PatchImp::__downloadFromFile file:" << file << "|pos:" << pos << "|to tail ok" << endl);

            return 1;
        }

        //读取文件出错了
        TLOGERROR("PatchImp::__downloadFromFile file:" << file << "|pos:" << pos << "|r:" << r << "|error:" << strerror(errno) << endl);

        fclose(fp);

        return -3;
    }

}


