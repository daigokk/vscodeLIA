#pragma once

#include "Config.h"
#include <thread>


class Pipe {
private:
    std::jthread thread_;
    void run(std::stop_token st, Config* pCfg);
    void commandShaping(std::string line, std::vector<std::string>& tokens, std::string& argument);
    bool commandDispach(std::vector<std::string>& tokens, std::string& argument, Config* pCfg) ;
public:
    bool endCommandStatus = false;
    void start(Config* pCfg);
    void stop();
};