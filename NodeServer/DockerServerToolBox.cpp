//
// Created by adugeek on 4/12/19.
//

#include "DockerServerToolBox.h"
#include <memory>
#include <sstream>
#include <unistd.h>
#include <util/tc_common.h>
#include <thread>
#include <sys/wait.h>
#include <cstring>

uid_t node_uid{0};
gid_t node_gid{0};

std::string DockerComposeYamlBuilder::build(const std::string &sEntryPoint, const std::vector<std::string> &vEntryPointOptions, const std::string &sBaseDir, const std::string &sConfDir,
                                const std::string &sDataDir,
                                const std::string &sLogDir) noexcept(false) {
    std::ostringstream stream;
    if (_sApplication.empty()) {
        throw std::runtime_error("DockerComposeYamlBuilder::build _sApplication is empty");
    }

    if (_sServerName.empty()) {
        throw std::runtime_error("DockerComposeYamlBuilder::build _sServerName is empty");
    }

//    if (_sVersion.empty()) {
//        _sVersion = "latest";
//    }

//    if (sBaseDir.empty()) {
//        throw std::runtime_error("DockerComposeYamlBuilder::build sBaseDir is empty");
//    }

    if (sDataDir.empty()) {
        throw std::runtime_error("DockerComposeYamlBuilder::build sDataDir is empty");
    }

    if (sConfDir.empty()) {
        throw std::runtime_error("DockerComposeYamlBuilder::build sConfDir is empty");
    }

    if (sLogDir.empty()) {
        throw std::runtime_error("DockerComposeYamlBuilder::build sLogDir is empty");
    }

//    if (_sRegistry.empty()) {
//        throw std::runtime_error("DockerComposeYamlBuilder::build sRegistry is empty");
//    }

    constexpr char ONE_SPACE[] = " ";
    constexpr char TWO_SPACE[] = "  ";

    constexpr char COMPOSE_VERSION[] = R"(version:)";
    constexpr char COMPOSE_FIXED_VERSION_VALUE[] = R"("3")";
    stream << COMPOSE_VERSION << ONE_SPACE << COMPOSE_FIXED_VERSION_VALUE << std::endl;

    constexpr char COMPOSE_SERVICES_FIELD[] = R"(services:)";
    stream << COMPOSE_SERVICES_FIELD << std::endl;
    stream << ONE_SPACE << _sApplication << "." << _sServerName << ":" << std::endl;

    constexpr char COMPOSE_IMAGE_FIELD[] = R"(image:)";
    stream << TWO_SPACE << COMPOSE_IMAGE_FIELD << ONE_SPACE << _sImage << endl;
//	_sRegistry << "/" << tars::TC_Common::lower(_sApplication) << "." << tars::TC_Common::lower(_sServerName) << ":"
//           << _sVersion << std::endl;

    constexpr char COMPOSE_PID_FIELD[] = R"(pid:)";
    constexpr char COMPOSE_FIXED_PID_VALUE[] = R"(host)";
    stream << TWO_SPACE << COMPOSE_PID_FIELD << ONE_SPACE << COMPOSE_FIXED_PID_VALUE << std::endl;

    constexpr char COMPOSE_NETWORK_FIELD[] = R"(network_mode:)";
    constexpr char COMPOSE_FIXED_NETWORK_VALUE[] = R"(host)";
    stream << TWO_SPACE << COMPOSE_NETWORK_FIELD << ONE_SPACE << COMPOSE_FIXED_NETWORK_VALUE << std::endl;

    constexpr char COMPOSE_USER_FIELD[] = R"(user:)";
    node_uid = (node_uid == 0 ? getuid() : node_uid);
    node_gid = (node_gid == 0 ? getgid() : node_gid);
    stream << TWO_SPACE << COMPOSE_USER_FIELD << ONE_SPACE << ::node_uid << ":" << ::node_gid << std::endl;

    constexpr char COMPOSE_VOLUMES_FIELD[] = R"(volumes:)";
    constexpr char YAML_ARRAY_FLAG[] = "-";

    stream << TWO_SPACE << COMPOSE_VOLUMES_FIELD << std::endl;
//    stream << TWO_SPACE << YAML_ARRAY_FLAG << ONE_SPACE << sBaseDir << ":" << sBaseDir << std::endl;
    stream << TWO_SPACE << YAML_ARRAY_FLAG << ONE_SPACE << sDataDir << ":" << sDataDir << std::endl;
    stream << TWO_SPACE << YAML_ARRAY_FLAG << ONE_SPACE << sConfDir << ":" << sConfDir << std::endl;
    stream << TWO_SPACE << YAML_ARRAY_FLAG << ONE_SPACE << sLogDir << ":" << sLogDir << std::endl;

    constexpr char FIXED_TIME_ZONE_FILE[] = "/etc/localtime";
    //映射 FIXED_TIME_ZONE_FILE 是为了保证容器程序的时区与主机时区一致
    stream << TWO_SPACE << YAML_ARRAY_FLAG << ONE_SPACE << FIXED_TIME_ZONE_FILE << ":" << FIXED_TIME_ZONE_FILE << std::endl;

    for (const auto &p:_vVolume) {
        stream << TWO_SPACE << YAML_ARRAY_FLAG << ONE_SPACE << p.first << ":" << p.second << std::endl;
    }

    constexpr char COMPOSE_ENVIRONMENT_FIELD[] = R"(environment:)";
    stream << TWO_SPACE << COMPOSE_ENVIRONMENT_FIELD << std::endl;
    stream << TWO_SPACE << YAML_ARRAY_FLAG << ONE_SPACE << "ExeDir=" << sBaseDir << std::endl;
    stream << TWO_SPACE << YAML_ARRAY_FLAG << ONE_SPACE << "ExeFile=" << sEntryPoint << std::endl;

    constexpr size_t FIXED_MAX_OPTIONS_COUNT = 10;
    if (vEntryPointOptions.size() > FIXED_MAX_OPTIONS_COUNT) {
        throw std::runtime_error("size of vEntryPointOptions should less then 10");
    }

    for (auto i = 0; i < vEntryPointOptions.size(); ++i) {
        stream << TWO_SPACE << YAML_ARRAY_FLAG << ONE_SPACE << "Arg" << i << "=" << vEntryPointOptions[i] << std::endl;
    }

    for (const auto &env:_vEnv) {
        stream << TWO_SPACE << YAML_ARRAY_FLAG << ONE_SPACE << env << std::endl;
    }

    return stream.str();
}
//
//int waitProcessDone(int64_t iPid) {
//
//    assert(iPid > 0);
//
//    if (iPid <= 0) {
//        throw std::runtime_error("iPid must bigger than zero");
//    }
//
//    int process_status;
//
//    while (true) {
//        int v = ::waitpid(iPid, &process_status, WNOHANG);
//        if (v != 0) {
//            break;
//        }
//        std::this_thread::sleep_for(std::chrono::milliseconds(100));
//    }
//
//    return process_status;
//}
//
//bool checkPull(const std::string &sDockerPullOutputFile) {
//    std::shared_ptr<FILE> spFile(fopen(sDockerPullOutputFile.c_str(), "r"), [](FILE *p) { fclose(p); });
//    if (spFile == nullptr) { return false; }
//
//    FILE *pFile = spFile.get();
//
//    std::unique_ptr<char[]> spLine;
//
//    char *line = spLine.get();
//    size_t len = 0;
//
//    constexpr char sSuccessFlagValue[] = "Status:";
//    constexpr size_t iSuccessFlagValueSize = sizeof(sSuccessFlagValue) - 1;
//
//    bool result = false;
//    while (getline(&line, &len, pFile) != -1) {
//        result = (len > iSuccessFlagValueSize && memcmp(sSuccessFlagValue, line, iSuccessFlagValueSize) == 0);
//    }
//    return result;
//}
//
//void setRegistry(const std::string &sRegistry) {
//    _sRegistry = sRegistry;
//}
//
//const std::string &getRegistry() {
//    return _sRegistry;
//}
