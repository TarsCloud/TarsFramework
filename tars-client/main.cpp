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
#include "util/tc_config.h"
#include <iostream>
#include <string>
#include "TarsClient.h"
#include "command/ping.h"

using namespace tars;
using namespace std;

int main(int argc, char *argv[])
{
	TC_Option option;

    try
    {
		option.decode(argc, argv);

		TarsClient tarsClient(option);

		tarsClient.add("ping", ping);
		tarsClient.add("start", TarsClient::command_function());
		tarsClient.add("stop", TarsClient::command_function());
		tarsClient.add("restart", TarsClient::command_function());
		tarsClient.add("patch", TarsClient::command_function());

		tarsClient.call("ping");
    }
    catch(exception &ex)
    {
		cout << "error: " << ex.what() << endl;
        exit(-1);
    }

    return 0;
}


