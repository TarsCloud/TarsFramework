#pragma once

#include "Topology.h"

class TopologyImp : public Topology
{
public:
	TopologyImp() = default;

	void initialize() override
	{
	};

	/**
	 * 退出
	 */
	void destroy() override
	{
	};

	Int32
	graphFunction(const std::string& date, const std::string& functionName, std::vector<Graph>& graph, CurrentPtr current) override;

	Int32
	graphServer(const std::string& date, const std::string& serverName, std::vector<Graph>& graph, CurrentPtr current) override;

	Int32 graphTrace(const std::string& date, const std::string& traceId, Graph& graph, CurrentPtr current) override;

	Int32
	listFunction(const std::string& date, const std::string& serverName, std::vector<std::string>& fs, CurrentPtr current) override;

	Int32 listTrace(const std::string& date, const std::string& serverName, std::vector<std::string>& ts, CurrentPtr current) override;


	Int32
	listTraceSummary(const std::string& date, Int64 beginTime, Int64 endTime, const std::string& serverName, std::vector<Summary>& ss,
			CurrentPtr current) override;

};
