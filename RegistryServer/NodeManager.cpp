//
// Created by jarod on 2022/5/5.
//

#include "NodeManager.h"
#include "NodePush.h"
#include "servant/Application.h"

NodePrx NodeManager::createNodePrx(const string& nodeName, const string &nodeObj)
{
	NodePrx nodePrx;
	Application::getCommunicator()->stringToProxy(nodeObj, nodePrx);

	TC_ThreadLock::Lock lock(_NodePrxLock);
	_mapNodePrxCache[nodeName] = nodePrx;

	return nodePrx;
}

bool NodeManager::hasNodePrx(const string& nodeName)
{
	TC_ThreadLock::Lock lock(_NodePrxLock);

	return _mapNodePrxCache.find(nodeName) != _mapNodePrxCache.end();
}

NodePrx NodeManager::getNodePrx(const string& nodeName)
{
	TC_ThreadLock::Lock lock(_NodePrxLock);

	if (_mapNodePrxCache.find(nodeName) != _mapNodePrxCache.end())
	{
		return _mapNodePrxCache[nodeName];
	}

	return NULL;
}

void NodeManager::eraseNodePrx(const string& nodeName)
{
	TC_ThreadLock::Lock lock(_NodePrxLock);

	_mapNodePrxCache.erase(nodeName);
}

void NodeManager::createNodeCurrent(const string& nodeName, CurrentPtr &current)
{
	eraseNodeCurrent(nodeName);

	TC_ThreadLock::Lock lock(_NodePrxLock);

	_mapNodeId[nodeName] = current->getUId();
	_mapIdNode[current->getUId()] = nodeName;
	_mapIdCurrent[current->getUId()] = current;
}

CurrentPtr NodeManager::getNodeCurrent(const string& nodeName)
{
	TC_ThreadLock::Lock lock(_NodePrxLock);

	auto it = _mapNodeId.find(nodeName);
	if(it != _mapNodeId.end())
	{
		return _mapIdCurrent.at(it->second);
	}

	return NULL;
}

void NodeManager::eraseNodeCurrent(const string& nodeName)
{
	TC_ThreadLock::Lock lock(_NodePrxLock);

	auto it = _mapNodeId.find(nodeName);
	if(it != _mapNodeId.end())
	{
		_mapIdNode.erase(it->second);
		_mapIdCurrent.erase(it->second);
	}

	_mapNodeId.erase(nodeName);
}

void NodeManager::eraseNodeCurrent(CurrentPtr &current)
{
	TC_ThreadLock::Lock lock(_NodePrxLock);

	auto it = _mapIdNode.find(current->getUId());
	if(it != _mapIdNode.end())
	{
		_mapNodeId.erase(it->second);
		_mapIdNode.erase(it->first);
		_mapIdCurrent.erase(it->first);
	}
}
