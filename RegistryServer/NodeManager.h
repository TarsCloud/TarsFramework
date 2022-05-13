//
// Created by jarod on 2022/5/5.
//

#ifndef FRAMEWORK_NODEMANAGER_H
#define FRAMEWORK_NODEMANAGER_H

#include "Node.h"
#include "util/tc_monitor.h"
#include "util/tc_singleton.h"

using namespace tars;

class NodeManager : public TC_Singleton<NodeManager>
{
public:
	NodePrx createNodePrx(const string& nodeName, const string &nodeObj);
	bool hasNodePrx(const string& nodeName);
	NodePrx getNodePrx(const string& nodeName);
	void eraseNodePrx(const string& nodeName);

	void createNodeCurrent(const string& nodeName, CurrentPtr &current);
	CurrentPtr getNodeCurrent(const string& nodeName);
	void eraseNodeCurrent(const string& nodeName);
	void eraseNodeCurrent(CurrentPtr &current);

protected:

	//node节点代理列表
	map<string , NodePrx> _mapNodePrxCache;
	TC_ThreadLock _NodePrxLock;

	map<string, int> _mapNodeId;
	map<int, string> _mapIdNode;
	map<int, CurrentPtr> _mapIdCurrent;
};


#endif //FRAMEWORK_NODEMANAGER_H
