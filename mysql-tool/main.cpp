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
		string sql = "select * from information_schema.tables where table_schema ='"+mysql.realEscapeString(db)+"' limit 1";

		TC_Mysql::MysqlData data = mysql.queryRecord(sql);
		if(data.size() > 0)
		{
			// for(size_t i = 0; i < data.size(); i++)
			// {
			// 	cout << data[i]["TABLE_NAME"] << endl;
			// }
			exit(0);
		}
		
		exit(1);
	}

	string getVersion(TC_Mysql &mysql)
	{
		TC_Mysql::MysqlData data = mysql.queryRecord("SELECT VERSION() as version");

		return data[0]["version"];
	}

	void executeSql(TC_Mysql &mysql, const string &sql)
	{
		mysql.execute(sql);
	}

	void executeFile(TC_Mysql &mysql, TC_Option &option)//const string &file)
	{
		string data = TC_File::load2str(option.getValue("file"));

		if(data.empty())
		{
			cout << "exec file:" << option.getValue("file") << ", no sql" << endl;
			exit(1);
		}

		data = TC_Common::replace(data, "/usr/local/app/tars", option.getValue("tars-path"));

		vector<string> sqls = TC_Common::sepstr<string>(data, ";");

        for(auto s : sqls)
        { 
            string sql = TC_Common::trim(s);

            if(sql.empty() || sql.length() < 2)
				continue;

			if(sql.substr(0, 2) == "--" || sql.substr(0, 2) == "/*" || sql.substr(0, 1) == "#")     
			{
				string::size_type pos = sql.find("\n");
				if(pos == string::npos)
				{
					continue;
				}

				sql = TC_Common::trim(sql.substr(pos));
			}       

            if(sql.empty())
				continue;

		    mysql.execute(sql);
        }
	}

	void executeTemplate(TC_Mysql &mysql, TC_Option &option)
	{
		string content = TC_File::load2str(option.getValue("profile"));

		string sql =  "replace into `t_profile_template` (`template_name`, `parents_name` , `profile`, `posttime`, `lastuser`) VALUES ('" + mysql.realEscapeString(option.getValue("template")) + "','" + mysql.realEscapeString(option.getValue("parent")) + "','" + mysql.realEscapeString(content) + "', now(),'admin')";

		mysql.execute(sql);
	}

	void replace(TC_Option &option)
	{
		string content = TC_File::load2str(option.getValue("replace"));

		if(content.find(option.getValue("src")) != string::npos)
		{
			content = TC_Common::replace(content, option.getValue("src"), option.getValue("dst"));

			TC_File::save2file(option.getValue("replace"), content);
		}
	}
};

int main(int argc, char *argv[])
{
	TC_Option option;

    try
    {
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
		    mysqlCmd.executeFile(mysql, option);
	    }
		else if(option.hasParam("template"))
		{
		    mysqlCmd.executeTemplate(mysql, option);
		}
	    else if(option.hasParam("replace"))
	    {
		    mysqlCmd.replace(option);
	    }
    }
    catch(exception &ex)
    {
		cout << "exec mysql parameter:" << TC_Common::tostr(option.getMulti().begin(), option.getMulti().end()) << endl;
		cout << "error:" << ex.what() << endl;
        exit(-1);
    }

    return 0;
}


