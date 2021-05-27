#ifndef RULE_CTRL_H_
#define RULE_CTRL_H_

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <cerrno>
#include <cstring>
#include <atomic>

#include <sys/stat.h>

namespace mode_ctrl
{

constexpr auto CONFIG_NAME = "/etc/mode_ctrl_config";

class RuleController
{
public:
     RuleController( const std::string& configPath = CONFIG_NAME ) throw ( std::exception );

     ~RuleController();

     void Stop();

     void WaitThreads();

     void CheckApps();
private:
     bool ReadConfig( const std::string& path );

     void SetOwnMode();

private:
     std::string configPath_;
     std::vector<std::pair<std::string, uint32_t>> applications_;
     std::thread modeSpy_;
     std::atomic<bool> stop_;
     std::atomic<bool> copmlete_;
};

} // namespace mode_ctrl

#endif // RULE_CTRL_H_
