#include "Exposer.hpp"
#include "spdlog/spdlog.h"
#include "Collector/Exception.hpp"

namespace hwgauge
{
	void Exposer::run()
	{
		running.store(true, std::memory_order_release);
		exposer.RegisterCollectable(registry);

		using clock = std::chrono::steady_clock;

		auto next_tick = clock::now();

		while (running.load(std::memory_order_acquire)) {
			collect();

			next_tick += interval;
			auto now = clock::now();

			if (now < next_tick)
			{
				std::this_thread::sleep_until(next_tick);
			}
		}
	}

	void Exposer::stop() {
		running.store(false, std::memory_order_release);
	}

	void Exposer::collect() {
		for (auto& collector : collectors)
		{
			std::string name = collector->name();
			try
			{
				collector->collect();
				spdlog::trace("Retrieve metrics from {} successfully", name);
			}
			catch (const hwgauge::RecoverableError& e)
			{
				// 记录错误，继续下一个 collector / 下一轮
				spdlog::error("Recoverable error from {}: {}",name, e.what());
				continue;
			}
			catch (const hwgauge::FatalError& e)
			{
				// 记录错误，停止整个采集循环
				spdlog::critical("Fatal error from {}: {}",name, e.what());
				stop();
				return;
			}
		}
	}
}