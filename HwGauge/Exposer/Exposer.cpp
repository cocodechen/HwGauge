#include "Exposer.hpp"
#include "spdlog/spdlog.h"
#include "Collector/Common/Context.hpp"

#include <chrono>     // std::chrono::system_clock
#include <ctime>      // std::time_t, std::localtime, std::tm
#include <sstream>    // std::ostringstream
#include <iomanip>    // std::put_time
#include <string>     // std::string

namespace hwgauge
{
	/* 获取当前时间戳 */
	inline std::string getNowTime()
	{
		auto now = std::chrono::system_clock::now();
		std::time_t now_time = std::chrono::system_clock::to_time_t(now);
		std::tm now_tm = *std::localtime(&now_time);
		std::ostringstream timestamp_ss;
		timestamp_ss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
		return timestamp_ss.str();
	}
	
	void Exposer::run()
	{
		running.store(true, std::memory_order_release);
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

	void Exposer::collect()
	{
		auto cur_time=getNowTime();
		spdlog::debug("CurrentTime {}",cur_time);
		for (auto& collector : collectors)
		{
			std::string name = collector->name();
			try
			{
				collector->collect(cur_time);
				spdlog::debug("Retrieve metrics from {} successfully", name);
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
		sharedPower.reSet(); // 每轮结束重置共享上下文，准备下一轮采集
	}
}