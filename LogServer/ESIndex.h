#pragma once

#include <string>
#include <cassert>

constexpr char LogIndexPrefix[] = "tars_call_log_";
constexpr char TraceIndexPrefix[] = "tars_call_trace_";
constexpr char GraphIndexPrefix[] = "tars_call_graph_";

constexpr char LogIndexTemplate[] = R"(
{
  "template": "tars_call_log_*",
  "mappings": {
    "properties": {
      "trace": {
        "type": "keyword"
      },
      "span": {
        "type": "keyword"
      },
      "parent": {
        "type": "keyword"
      },
      "type": {
        "type": "keyword"
      },
      "master": {
        "type": "keyword"
      },
      "slave": {
        "type": "keyword"
      },
      "function": {
        "type": "keyword"
      },
      "time": {
        "type": "long"
      },
      "ret": {
        "type": "keyword"
      },
      "data": {
        "type": "keyword"
      }
    }
  }
})";
constexpr char LogIndexUri[] = "/_template/tars_call_log";

constexpr char TraceIndexTemplate[] = R"(
{
  "template": "tars_call_trace_*",
  "mappings": {
    "properties": {
      "trace": {
        "type": "keyword"
      },
      "tSpan": {
        "type": "keyword"
      },
      "tMaster": {
        "type": "keyword"
      },
      "tsTime": {
        "type": "long"
      },
      "teTime": {
        "type": "long"
      },
      "sHash": {
        "type": "keyword"
      },
      "fHash": {
        "type": "keyword"
      },
      "spans": {
        "type": "nested",
        "properties": {
          "span": {
            "type": "keyword"
          },
          "parent": {
            "type": "keyword"
          },
          "master": {
            "type": "keyword"
          },
          "slave": {
            "type": "keyword"
          },
          "csTime": {
            "type": "long"
          },
          "srTime": {
            "type": "long"
          },
          "ssTime": {
            "type": "long"
          },
          "crTime": {
            "type": "long"
          },
          "csData": {
            "type": "keyword"
          },
          "srData": {
            "type": "keyword"
          },
          "ssData": {
            "type": "keyword"
          },
          "crData": {
            "type": "keyword"
          },
          "ret": {
            "type": "keyword"
          },
          "children": {
            "type": "keyword"
          }
        }
      }
    }
  }
})";
constexpr char TraceIndexTemplateUri[] = "/_template/tars_call_trace";

constexpr char GraphIndexTemplate[] = R"(
{
  "template": "tars_call_graph_*",
  "mappings": {
    "properties": {
      "type": {
        "type": "keyword"
      },
      "vertexes": {
        "type": "nested",
        "properties": {
          "vertex": {
            "type": "keyword"
          },
          "callCount": {
            "type": "long"
          },
          "callTime": {
            "type": "long"
          }
        }
      },
      "edges": {
        "type": "nested",
        "properties": {
          "fromVertex": {
            "type": "keyword"
          },
          "toVertex": {
            "type": "keyword"
          },
          "callCount": {
            "type": "long"
          },
          "callTime": {
            "type": "long"
          },
          "order": {
            "type": "short"
          }
        }
      }
    }
  }
})";
constexpr char GraphIndexTemplateUri[] = "/_template/tars_call_graph";

inline string buildLogIndexByLogfile(const string& file)
{
	assert(file.size() >= 12);
	return LogIndexPrefix + file.substr(file.size() - 12, 8);
}

inline string buildRawLogIndexByDate(const string& date)
{
	return LogIndexPrefix + date;
}

inline string buildTraceIndexByLogfile(const string& file)
{
	assert(file.size() >= 12);
	return TraceIndexPrefix + file.substr(file.size() - 12, 8);
}

inline string buildTraceIndexByDate(const string& date)
{
	return TraceIndexPrefix + date;
}

inline string buildGraphIndexByLogfile(const string& file)
{
	assert(file.size() >= 12);
	return GraphIndexPrefix + file.substr(file.size() - 12, 8);
}

inline string buildGraphIndexByDate(const string& date)
{
	return GraphIndexPrefix + date;
}