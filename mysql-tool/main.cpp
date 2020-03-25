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

#include "util/tc_mysql.h"
#include "util/tc_option.h"
#include "util/tc_file.h"
#include "util/tc_common.h"
#include <iostream>
#include <string>

using namespace tars;
using namespace std;

/**
 * mysql-tool --h --u --p --P --check
 * mysql-tool --h --u --p --P --sql
 * mysql-tool --h --u --p --P --file
 * @param argc
 * @param argv
 * @return
 */

struct MysqlCommand
{
	string host;
	string user;
	string port;
	string pass;
	string db;
	string charset;

	void check(TC_Mysql &mysql)
	{
		TC_Mysql::MysqlData data = mysql.queryRecord("select 1=1");
		if(data.size() <= 0)
		{
			exit(-1);
		}
	}

	void has(TC_Mysql &mysql, const string &db)
	{
//		cout << "has database: " << db << endl;
		mysql.execute("use " + db);
	}

	string getVersion(TC_Mysql &mysql)
	{
		TC_Mysql::MysqlData data = mysql.queryRecord("SELECT VERSION() as version");

		return data[0]["version"];
	}

	void executeSql(TC_Mysql &mysql, const string &sql)
	{
//		cout << "exec sql:" << sql << endl;

		mysql.execute(sql);
	}

	void executeFile(TC_Mysql &mysql, const string &file)
	{
//		cout << "exec file:" << file << endl;

		string data = TC_File::load2str(file);
		mysql.execute(data);
	}
};

int main(int argc, char *argv[])
{
    try
    {
		TC_Option option;
		option.decode(argc, argv);

	    MysqlCommand mysqlCmd;

	    mysqlCmd.host = option.getValue("host");
	    mysqlCmd.user = option.getValue("user");
	    mysqlCmd.port = option.getValue("port");
	    mysqlCmd.pass = option.getValue("pass");
	    mysqlCmd.db = option.getValue("db");
	    mysqlCmd.charset = option.getValue("charset");

	    TC_Mysql mysql;

	    mysql.init(mysqlCmd.host, mysqlCmd.user, mysqlCmd.pass, mysqlCmd.db, mysqlCmd.charset, TC_Common::strto<int>(mysqlCmd.port), CLIENT_MULTI_STATEMENTS);

	    if(option.hasParam("check"))
	    {
		    mysqlCmd.check(mysql);
	    }
	    else if(option.hasParam("has"))
	    {
		    mysqlCmd.has(mysql, option.getValue("has"));
	    }
	    else if(option.hasParam("version"))
	    {
		    cout << mysqlCmd.getVersion(mysql) << endl;
		    return 0;
	    }
	    else if(option.hasParam("sql"))
	    {
	    	mysqlCmd.executeSql(mysql, option.getValue("sql"));
	    }
	    else if(option.hasParam("file"))
	    {
		    mysqlCmd.executeFile(mysql, option.getValue("file"));
	    }
    }
    catch(exception &ex)
    {
        exit(-1);
    }

    return 0;
}


