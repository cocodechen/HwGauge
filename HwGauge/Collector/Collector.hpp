#pragma once

#include <string>
#include <memory>

namespace hwgauge
{
	class Collector
    {
    public:
        explicit Collector(){}

        virtual ~Collector() = default;
        
        virtual std::string name() = 0;
        virtual void collect() = 0;

        Collector(const Collector&) = delete;
        Collector& operator=(const Collector&) = delete;
    protected:
	};

}

