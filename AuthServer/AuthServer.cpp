#include "AuthServer.h"
#include "AuthImp.h"

void AuthServer::initialize()
{
    //���Ӷ���
    addServant<AuthImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".AuthObj");
}

void AuthServer::destroyApp()
{
    cout << "AuthServer::destroyApp ok" << endl;
}

