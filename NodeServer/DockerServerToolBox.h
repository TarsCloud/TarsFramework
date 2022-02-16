//
// Created by adugeek on 4/12/19.
//
#pragma once

#include <string>
#include <vector>
#include <utility>
#include <memory>

class DockerComposeYamlBuilder
{

public:

    DockerComposeYamlBuilder(const std::string &sApplication, const std::string &sServerName, const std::string &sImage) : _sApplication(sApplication),
                                                                                                        _sServerName(sServerName),
                                                                                                        _sImage(sImage) {}

    std::string
    build(const std::string &sEntryPoint, const std::vector<std::string> &vEntryPointOptions, const std::string &sBaseDir, const std::string &sConfDir, const std::string &sDataDir,
          const std::string &sLogDir) noexcept(false);

    inline void addVolume(std::string from, std::string to) {
        _vVolume.emplace_back(std::make_pair(std::move(from), std::move(to)));
    }

    inline void addEnv(std::string env) {
        _vEnv.emplace_back(std::move(env));
    }

private:
    std::string _sApplication{};
    std::string _sServerName{};
    std::string _sImage{};
    std::vector<std::pair<std::string, std::string>> _vVolume{};
    std::vector<std::string> _vEnv{};
};

//int waitProcessDone(int64_t iPid);

inline std::string getDockerComposeExePath() {
    return std::string("docker-compose");
}

inline std::string getDockerExePath() {
    return std::string("docker");
}

//bool checkPull(const std::string &sDockerPullOutputFile);