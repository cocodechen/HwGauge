#pragma once

#include "Collector/Common/Exception.hpp"
#include "spdlog/spdlog.h"

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace hwgauge
{
    namespace fs = std::filesystem;

    template <typename LabelT, typename MetricT>
    class CsvLogger
    {
    public:
        explicit CsvLogger(const std::string& filepath) 
        {
            // 1. 自动处理后缀 .csv
            fs::path p(filepath);
            if (p.extension() != ".csv")p += ".csv";
            m_filepath = p.string();

            // 2. 尝试自动创建父目录 (可选，增强健壮性)
            // 如果传入 "logs/gpu"，这里会自动创建 logs 文件夹
            if (p.has_parent_path() && !fs::exists(p.parent_path()))
            {
                std::error_code ec;
                fs::create_directories(p.parent_path(), ec);
                if (ec) {
                    spdlog::error("[CsvLogger] Failed to create directory: {}", p.parent_path().string());
                    throw FatalError("CsvLogger dir creation failed");
                }
            }
            // 3. 打开文件
            m_ofs.open(m_filepath, std::ios::out | std::ios::app);
            
            if (!m_ofs.is_open()) {
                spdlog::error("[CsvLogger] Failed to open file: {}", m_filepath);
                throw FatalError("CsvLogger open failed: " + m_filepath);
            }
            spdlog::info("[CsvLogger] Initialized logger for: {}", m_filepath);
        }

        virtual ~CsvLogger()
        {
            if (m_ofs.is_open())
            {
                m_ofs.close();
                spdlog::debug("[CsvLogger] Closed file: {}", m_filepath);
            }
        }

        void init()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            // 检查文件大小，只有当文件为空（新建）时才写入表头
            if (m_ofs.tellp() == 0)
            {
                std::string header = this->getHeader();
                m_ofs << "Timestamp," << header << "\n";
                m_ofs.flush();
                spdlog::info("[CsvLogger] New file detected, header written.");
            }
            else spdlog::debug("[CsvLogger] Appending to existing file.");
        }

        void write(const std::string& timestamp, 
                const std::vector<LabelT>& labels, 
                const std::vector<MetricT>& metrics) 
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            try
            {
                for (size_t i = 0; i < labels.size(); ++i)
                {
                    m_ofs << timestamp << ",";
                    m_ofs << this->formatRow(labels[i], metrics[i]) << "\n";
                }
                m_ofs.flush();
            } 
            catch (const std::exception& e)
            {
                spdlog::error("[CsvLogger] Exception during write: {}", e.what());
                throw RecoverableError("Write operation failed");
            }
            spdlog::info("[CsvLogger]  Successfully inserted {} records into {}",labels.size(),m_filepath);
        }

    protected:
        virtual std::string getHeader() const = 0;
        virtual std::string formatRow(const LabelT& label, const MetricT& metric) const = 0;

    private:
        std::string m_filepath;
        std::ofstream m_ofs;
        std::mutex m_mutex;
    };
}