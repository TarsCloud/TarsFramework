#ifndef _AUTH_SERVER_H_
#define _AUTH_SERVER_H_

#include "servant/Application.h"

using namespace tars;

class AuthServer : public Application
{
protected:
    /**
     * ��ʼ��, ֻ����̵���һ��
     */
    virtual void initialize();

    /**
     * ����, ÿ�����̶������һ��
     */
    virtual void destroyApp();
};

#endif

