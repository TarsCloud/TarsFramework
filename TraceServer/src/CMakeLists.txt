
project(tarstrace)

include_directories(../third-party/asio/include)
include_directories(../third-party/rapidjson/include)

set(TARS_TOOL_FLAG "--with-tars --without-trace")

gen_server("tars" "tarstrace")