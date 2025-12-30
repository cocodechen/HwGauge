#include "CLI/CLI.hpp"
#include "spdlog/spdlog.h"
#include <string>
#include <csignal>
#include <memory>
#include <chrono>

#include "Exposer/Exposer.hpp"
#include "Collector/GPUCollector/GPUCollector.hpp"
#ifdef HWGAUGE_USE_NVML
#	include "Collector/GPUCollector/NVML.hpp"
#endif
#include "Collector/CPUCollector/CPUCollector.hpp"
#ifdef HWGAUGE_USE_INTEL_PCM
#	include "Collector/CPUCollector/PCM.hpp"
#endif

std::unique_ptr<hwgauge::Exposer> exposer = nullptr;

static void signal_handler(int signal) {
	if (signal == SIGINT && exposer != nullptr) {
		spdlog::info("Stopping exposer");
		exposer->stop();
		exposer.reset();
	}
}

int main(int argc, char* argv[]) {
	// Parse command-line arguments
	CLI::App application{
		"HwGauge: A lightweight hardware power consumption exporter for Prometheus metrics."
	};
	argv = application.ensure_utf8(argv);
	
	// Command-line arguments: address
	constexpr char default_address[] = "127.0.0.1:8000";
	std::string address = default_address;
	application.add_option("-a,--address", address, "Address to start Prometheus exposer")
		->default_val(default_address);

	// Command-line arguments: interval
	constexpr int default_interval = 1;
	int interval_seconds = default_interval;
	application.add_option("-i,--interval", interval_seconds, "Collection interval in seconds")
		->default_val(default_interval)
		->check(CLI::PositiveNumber);

	CLI11_PARSE(application, argc, argv);

	// Initialize spdlog logger
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [tid %t] %v");
	spdlog::info("Spdlog initialized successfully");

	// Create Prometheus exposer
	exposer = std::make_unique<hwgauge::Exposer>(address, std::chrono::seconds(interval_seconds));
	std::signal(SIGINT, signal_handler);

#ifdef HWGAUGE_USE_NVML
	exposer->add_collector<hwgauge::GPUCollector<hwgauge::NVML>>(hwgauge::NVML());
#endif
#ifdef HWGAUGE_USE_INTEL_PCM
	exposer->add_collector<hwgauge::CPUCollector<hwgauge::PCM>>(hwgauge::PCM());
#endif

	spdlog::info("Staring exposer on \"{}\"", address);
	spdlog::info("Press \"Ctrl+C\" to stop exposer");
	exposer->run();
	return 0;
}