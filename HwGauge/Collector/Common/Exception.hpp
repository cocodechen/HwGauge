#pragma once

#include <stdexcept>
#include <string>

namespace hwgauge
{
    // 继续下一轮采集
    class RecoverableError : public std::runtime_error
    {
        public:
            explicit RecoverableError(const std::string& msg)
                : std::runtime_error(msg) {}
    };

    // 直接终止采集
    class FatalError : public std::runtime_error
    {
        public:
            explicit FatalError(const std::string& msg)
                : std::runtime_error(msg) {}
    };

}
