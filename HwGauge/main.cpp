#include "CLI/CLI.hpp"
#include "spdlog/spdlog.h"
#include <string>
#include <csignal>
#include <memory>
#include <chrono>
#include <atomic>

#include "Exposer/Exposer.hpp"
#include "Collector/Config.hpp"

#ifdef HWGAUGE_USE_INTEL_PCM
#include "Collector/CPUCollector/CPUCollector.hpp"
#include "Collector/CPUCollector/PCM.hpp"
#endif

#ifdef HWGAUGE_USE_NVML
#include "Collector/GPUCollector/GPUCollector.hpp"
#include "Collector/GPUCollector/NVML.hpp"
#endif

#ifdef HWGAUGE_USE_NPU
#include "Collector/NPUCollector/NPUCollector.hpp"
#include "Collector/NPUCollector/NPUImpl.hpp"
#endif

#ifdef __linux__
#include "Collector/SYSCollector/SYSCollector.hpp"
#include "Collector/SYSCollector/SYSImpl.hpp"
#endif

std::unique_ptr<hwgauge::Exposer> exposer = nullptr;

std::atomic<bool> g_stop_requested{false};
static void signal_handler(int signal)
{
    if (signal == SIGINT) {
        g_stop_requested.store(true, std::memory_order_relaxed);
    }
}

int main(int argc, char* argv[])
{
	// Parse command-line arguments
	CLI::App application{
		"HwGauge: A lightweight hardware power consumption exporter for Prometheus metrics or PostgreSQL."
	};
	argv = application.ensure_utf8(argv);

	// Command-line arguments: interval
	constexpr int default_interval = 1;
	int interval_seconds = default_interval;
	application.add_option("-i,--interval", interval_seconds, "Collection interval in seconds")
		->default_val(default_interval)
		->check(CLI::PositiveNumber);

	// Command-line arguments: address
	constexpr char default_address[] = "127.0.0.1:8000";
	std::string address = default_address;
	application.add_option("-a,--address", address, "Address to start exposer")
		->default_val(default_address);

	hwgauge::CollectorConfig cfg;
	// Command-line arguments: outTer
	cfg.outTer=true;
	application.add_flag("--outTer", cfg.outTer, "Enable to out the Collection Results to Terminal");
	// Command-line arguments: sysinfo
	cfg.sysinfo=true;
	application.add_flag("--sysInfo", cfg.sysinfo, "Enable to out the system information");

#ifdef HWGAUGE_USE_PROMETHEUS
	auto registry=std::make_shared<prometheus::Registry>();
	prometheus::Exposer pm_exposer(std::move(address));
	pm_exposer.RegisterCollectable(registry);
	cfg.registry = registry;

	cfg.pmEnable = false;
	application.add_flag("--pm-enable", cfg.pmEnable, "Enable Prometheus metrics export");
#endif

#ifdef HWGAUGE_USE_POSTGRESQL
	// Command-line arguments: database
	cfg.dbEnable = false;
	application.add_flag("--db-enable", cfg.dbEnable, "Enable database storage for NPU metrics");

	application.add_option("--db-host", cfg.dbConfig.host, "Database host")
		->default_val("localhost");
	application.add_option("--db-port", cfg.dbConfig.port, "Database port")
		->default_val("5432");
	application.add_option("--db-name", cfg.dbConfig.dbname, "Database name")
		->default_val("postgres");
	application.add_option("--db-user", cfg.dbConfig.user, "Database user")
		->default_val("postgres");
	application.add_option("--db-password", cfg.dbConfig.password, "Database password")
		->default_val("123456");

	// Command-line arguments: table_name
	cfg.dbTableName = "test";
	application.add_option("--db-table", cfg.dbTableName, "Database table name for device metrics")
		->default_val("test");
#endif
	CLI11_PARSE(application, argc, argv);

	// Initialize spdlog logger
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [tid %t] %v");
	spdlog::set_level(spdlog::level::info); 
	spdlog::info("Spdlog initialized successfully");

	// Create exposer
	exposer = std::make_unique<hwgauge::Exposer>(std::chrono::seconds(interval_seconds));
#ifdef HWGAUGE_USE_INTEL_PCM
	exposer->add_collector<hwgauge::CPUCollector<hwgauge::PCM>>(hwgauge::PCM(),cfg);
#endif

#ifdef HWGAUGE_USE_NVML
	exposer->add_collector<hwgauge::GPUCollector<hwgauge::NVML>>(hwgauge::NVML(),cfg);
#endif

#ifdef HWGAUGE_USE_NPU
	exposer->add_collector<hwgauge::NPUCollector<hwgauge::NPUImpl>>(hwgauge::NPUImpl(),cfg);
#endif

#ifdef __linux__
	if(cfg.sysinfo)exposer->add_collector<hwgauge::SYSCollector<hwgauge::SYSImpl>>(hwgauge::SYSImpl(),cfg);
#endif

	spdlog::info("Staring exposer on \"{}\"", address);
	spdlog::info("Press \"Ctrl+C\" to stop exposer");
	
	std::signal(SIGINT, signal_handler);
	std::thread signal_watcher([&]{
		while (!g_stop_requested.load(std::memory_order_relaxed))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		spdlog::info("Stopping exposer");
		exposer->stop();
	});

	exposer->run();
	//g_stop_requested.store(true, std::memory_order_relaxed);

	signal_watcher.join();
	exposer.reset();
	return 0;
}