#include "TopologyImp.h"
#include "ESReader.h"
#include "TraceData.h"

static void insertEdge(const Edge& edge, vector<Edge>& edges)
{
	for (auto&& item: edges)
	{
		if (item.fromVertex == edge.fromVertex && item.toVertex == edge.toVertex && item.order == edge.order)
		{
			item.callCount += edge.callCount;
			item.callTime += edge.callTime;
			return;
		}
	}
	edges.emplace_back(edge);
}

static void insertVertex(const Vertex& vertex, vector<Vertex>& vertexes)
{
	for (auto&& item: vertexes)
	{
		if (item.vertex == vertex.vertex)
		{
			item.callCount += vertex.callCount;
			item.callTime += vertex.callTime;
			return;
		}
	}
	vertexes.emplace_back(vertex);
}

static void graphTraceFunction_(const ITrace* trace, short order, const string& parentFunction, const string& span, Graph& graph)
{
	const ISpan* ptr{};
	for (auto&& item: trace->spans)
	{
		if (item.span == span)
		{
			ptr = &item;
			break;
		}
	}
	if (ptr == nullptr)
	{
		throw std::runtime_error("span not exist");
	}

	auto function = ptr->slave + "." + ptr->function;
	Vertex vertex{};
	vertex.vertex = function;
	if (ptr->srTime != 0 && ptr->ssTime != 0 && ptr->srTime <= ptr->ssTime)
	{
		vertex.callCount += 1;
		vertex.callTime += ptr->ssTime - ptr->srTime;
	}
	insertVertex(vertex, graph.vs);

	Edge edge{};
	edge.fromVertex = parentFunction;
	edge.toVertex = function;
	if (ptr->csTime != 0 && ptr->crTime != 0 && ptr->csTime <= ptr->crTime)
	{
		edge.callCount += 1;
		edge.callTime += ptr->crTime - ptr->csTime;
	}
	edge.spanId = ptr->span;
	edge.csTime = ptr->csTime;
	edge.srTime = ptr->srTime;
	edge.ssTime = ptr->ssTime;
	edge.crTime = ptr->crTime;
	edge.csData = ptr->csData;
	edge.srData = ptr->srData;
	edge.ssData = ptr->ssData;
	edge.crData = ptr->crData;
	edge.ret = ptr->ret;
	edge.order = order;
	insertEdge(edge, graph.es);
	for (auto&& child: ptr->children)
	{
		graphTraceFunction_(trace, order + 1, function, child, graph);
	}
}

static void graphTraceFunction(const ITrace* trace, Graph& graph)
{
	if (trace->tSpan.empty())
	{
		return;
	}
	Vertex vertex{};
	vertex.vertex = trace->tMaster;
	if (trace->tsTime != 0 && trace->teTime != 0 && trace->tsTime <= trace->teTime)
	{
		vertex.callCount += 1;
		vertex.callTime += trace->teTime - trace->tsTime;
	}
	insertVertex(vertex, graph.vs);
	graphTraceFunction_(trace, 1, trace->tMaster, trace->tSpan, graph);
	std::sort(graph.vs.begin(), graph.vs.end());
	std::sort(graph.es.begin(), graph.es.end());
}

static void transformGraph(const IGraph& iGraph, Graph& graph)
{
	graph.es.reserve(iGraph.edges.size());
	for (auto&& item: iGraph.edges)
	{
		auto bes = Edge{};
		bes.fromVertex = item.fromVertex;
		bes.toVertex = item.toVertex;
		bes.callCount = item.callCount;
		bes.callTime = item.callTime;
		graph.es.emplace_back(bes);
	}

	graph.vs.reserve(iGraph.vertexes.size());
	for (auto&& item: iGraph.vertexes)
	{
		auto bvs = Vertex{};
		bvs.vertex = item.vertex;
		bvs.callCount = item.callCount;
		bvs.callTime = item.callTime;
		graph.vs.emplace_back(bvs);
	}
}

Int32 TopologyImp::listFunction(const string& date, const string& serverName, vector<std::string>& fs, CurrentPtr current)
{
	std::set<std::string> sfs{};
	if (ESReader::listFunction(date, serverName, sfs) != 0)
	{
		return -1;
	}
	fs.insert(fs.begin(), sfs.begin(), sfs.end());
	return 0;
}

Int32 TopologyImp::listTrace(const string& date, const string& serverName, vector<std::string>& ts, CurrentPtr current)
{
	return ESReader::listTrace(date, 0, INT64_MAX, serverName, ts);
}

Int32
TopologyImp::listTraceSummary(const string& date, Int64 beginTime, Int64 endTime, const string& serverName, vector<Summary>& ss, CurrentPtr current)
{
	return ESReader::listTraceSummary(date, beginTime, endTime, serverName, ss);
}

Int32 TopologyImp::graphFunction(const string& date, const string& functionName, vector<Graph>& graph, CurrentPtr current)
{
	std::vector<IGraph> igs{};
	if (ESReader::getFunctionGraph(date, functionName, igs) != 0)
	{
		return -1;
	}
	graph.resize(igs.size());
	for (auto i = 0; i < igs.size(); ++i)
	{
		transformGraph(igs[i], graph[i]);
	}
	return 0;
}

Int32 TopologyImp::graphServer(const string& date, const string& serverName, vector<Graph>& graph, CurrentPtr current)
{
	std::vector<IGraph> igs{};
	if (ESReader::getServerGraph(date, serverName, igs) != 0)
	{
		return -1;
	}
	graph.resize(igs.size());
	for (auto i = 0; i < igs.size(); ++i)
	{
		transformGraph(igs[i], graph[i]);
	}
	return 0;
}

Int32 TopologyImp::graphTrace(const string& date, const string& traceId, Graph& graph, CurrentPtr current)
{
	ITrace iTrace{};
	if (ESReader::getTrace(date, traceId, iTrace) != 0)
	{
		return -1;
	}
	graphTraceFunction(&iTrace, graph);
	return 0;
}
