
#include "TarsClient.h"
#include "util/tc_port.h"

TarsClient::TarsClient(TC_Option &option) : _option(option)
{
	_comm = new Communicator();

	string locator;

	if(option.hasParam("TARS_REGISTRY_HOST"))
	{
		locator = option.getValue("TARS_REGISTRY_HOST");
	}
	else 
	{
		locator = TC_Port::getEnv("TARS_REGISTRY_HOST");
	}

	if(locator.empty())
	{
		locator = "tars.tarsregistry.QueryObj@tcp -h 127.0.0.1 -p 17890";
	}
	else
	{
		locator = "tars.tarsregistry.QueryObj@tcp -h " + locator + " -p 17890";
	}
	_comm->setProperty("locator", locator);
}

void TarsClient::call(const string &command)
{
	cout << "call:" << command << endl;
	_commands[command](this, _option);
}

TarsClient::~TarsClient()
{
	delete _comm;
	_comm = NULL;
}