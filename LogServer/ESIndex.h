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
        "type": "text",
        "fields": {
          "keyword": {
            "ignore_above": 256,
            "type": "keyword"
          }
        }
      },
      "span": {
        "type": "text",
        "fields": {
          "keyword": {
            "ignore_above": 256,
            "type": "keyword"
          }
        }
      },
      "parent": {
        "type": "text",
        "fields": {
          "keyword": {
            "ignore_above": 256,
            "type": "keyword"
          }
        }
      },
      "master": {
        "type": "text",
        "fields": {
          "keyword": {
            "ignore_above": 256,
            "type": "keyword"
          }
        }
      },
      "slave": {
        "type": "text",
        "fields": {
          "keyword": {
            "ignore_above": 256,
            "type": "keyword"
          }
        }
      },
      "function": {
        "type": "text",
        "fields": {
          "keyword": {
            "ignore_above": 256,
            "type": "keyword"
          }
        }
      },
      "time": {
        "type": "long"
      },
      "type": {
        "type": "text"
      },
      "ret": {
        "type": "text"
      },
      "data": {
        "type": "text"
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
        "type": "text",
        "fields": {
          "keyword": {
            "ignore_above": 256,
            "type": "keyword"
          }
        }
      },
      "tSpan": {
        "type": "text",
        "fields": {
          "keyword": {
            "ignore_above": 256,
            "type": "keyword"
          }
        }
      },
      "tMaster": {
        "type": "text",
        "fields": {
          "keyword": {
            "ignore_above": 256,
            "type": "keyword"
          }
        }
      },
      "tsTime": {
        "type": "long"
      },
      "teTime": {
        "type": "long"
      },
      "sHash": {
        "type": "text",
        "fields": {
          "keyword": {
            "ignore_above": 256,
            "type": "keyword"
          }
        }
      },
      "fHash": {
        "type": "text",
        "fields": {
          "keyword": {
            "ignore_above": 256,
            "type": "keyword"
          }
        }
      },
      "spans": {
        "type": "nested",
        "properties": {
          "span": {
            "type": "text",
            "fields": {
              "keyword": {
                "ignore_above": 256,
                "type": "keyword"
              }
            }
          },
          "parent": {
            "type": "text",
            "fields": {
              "keyword": {
                "ignore_above": 256,
                "type": "keyword"
              }
            }
          },
          "master": {
            "type": "text",
            "fields": {
              "keyword": {
                "ignore_above": 256,
                "type": "keyword"
              }
            }
          },
          "slave": {
            "type": "text",
            "fields": {
              "keyword": {
                "ignore_above": 256,
                "type": "keyword"
              }
            }
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
            "type": "text"
          },
          "srData": {
            "type": "text"
          },
          "ssData": {
            "type": "text"
          },
          "crData": {
            "type": "text"
          },
          "ret": {
            "type": "text"
          },
          "children": {
            "type": "text",
            "fields": {
              "keyword": {
                "ignore_above": 256,
                "type": "keyword"
              }
            }
          },
          "function": {
            "type": "text",
            "fields": {
              "keyword": {
                "ignore_above": 256,
                "type": "keyword"
              }
            }
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
        "type": "text",
        "fields": {
          "keyword": {
            "ignore_above": 256,
            "type": "keyword"
          }
        }
      },
      "vertexes": {
        "type": "nested",
        "properties": {
          "vertex": {
            "type": "text",
            "fields": {
              "keyword": {
                "ignore_above": 256,
                "type": "keyword"
              }
            }
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
            "type": "text",
            "fields": {
              "keyword": {
                "ignore_above": 256,
                "type": "keyword"
              }
            }
          },
          "toVertex": {
            "type": "text",
            "fields": {
              "keyword": {
                "ignore_above": 256,
                "type": "keyword"
              }
            }
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