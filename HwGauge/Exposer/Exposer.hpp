#pragma once

#include "Collector/Collector.hpp"
#include "Collector/Exception.hpp"

#include <vector>
#include <memory>
#include <utility>
#include <atomic>
#include <chrono>
#include <spdlog/spdlog.h>

namespace hwgauge
{
	class Exposer
	{
	public:
		Exposer(std::chrono::seconds interval) :
			interval(interval) {}

		template<typename T, typename... Args>
		void inline add_collector(Args&&... args)
		{
			try
			{
				collectors.push_back(std::make_unique<T>(std::forward<Args>(args)...));
			}
			catch (const hwgauge::RecoverableError& e)
			{
				spdlog::error("Recoverable error: {}", e.what());
			}
			catch (const hwgauge::FatalError& e)
			{
				spdlog::critical("Fatal error from: {}", e.what());
				exit(EXIT_FAILURE);
			}
		}

		void run();
		void stop();
	private:
		void collect();
	private:
		std::atomic<bool> running = false;
		std::chrono::seconds interval;
		std::vector<std::unique_ptr<Collector>> collectors;
	};
}