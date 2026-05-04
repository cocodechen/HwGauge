#pragma once
#ifdef HWGAUGE_USE_LOCAL_HTTP

#include <thread>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <mutex>
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"
#include "httplib.h"



namespace hwgauge {
    class LocalHttpServer {
    public:
        LocalHttpServer() : running_(false) {
            server_ = std::make_unique<httplib::Server>();
        }

        ~LocalHttpServer() { stop(); }

        void start(const std::string& host, int port) {
            running_ = true;
            server_thread_ = std::thread([this, host, port]() {
                spdlog::info("Local HTTP Server starting at http://{}:{}", host, port);
                server_->listen(host.c_str(), port);
            });
        }

        void stop() {
            if (running_ && server_) {
                server_->stop();
                if (server_thread_.joinable()) {
                    server_thread_.join();
                }
                running_ = false;
            }
        }

        httplib::Server& get_server() { return *server_; }

    private:
        std::unique_ptr<httplib::Server> server_;
        std::thread server_thread_;
        bool running_;
    };

    template <typename LabelT, typename MetricT>
    class HttpApi {
    public:
        // 构造时传入全局 Server 和特定的路由路径 (例如 "/api/cpu")
        HttpApi(std::shared_ptr<LocalHttpServer> server, const std::string& path)
            : server_(server), path_(path)
        {}

        void init() {
            if (!server_) return;
            
            // 在全局 server 中注册本硬件的路由
            server_->get_server().Get(path_, [this](const httplib::Request&, httplib::Response& res) {
                nlohmann::json response;
                {
                    std::lock_guard<std::mutex> lock(data_mutex_);
                    response["timestamp"] = last_time_;
                    response["data"] = nlohmann::json::array();

                    for (size_t i = 0; i < last_labels_.size(); ++i) {
                        nlohmann::json item;
                        item["label"] = last_labels_[i];   // 依赖 to_json
                        item["metric"] = last_metrics_[i]; // 依赖 to_json
                        response["data"].push_back(item);
                    }
                }
                res.set_content(response.dump(), "application/json");
                res.set_header("Access-Control-Allow-Origin", "*");
            });
            spdlog::info("Registered HTTP endpoint: {}", path_);
        }

        void write(const std::string& cur_time, const std::vector<LabelT>& labels, const std::vector<MetricT>& metrics) {
            std::lock_guard<std::mutex> lock(data_mutex_);
            last_time_ = cur_time;
            last_labels_ = labels;
            last_metrics_ = metrics;
        }

    private:
        std::shared_ptr<LocalHttpServer> server_;
        std::string path_;
        
        std::mutex data_mutex_;
        std::string last_time_;
        std::vector<LabelT> last_labels_;
        std::vector<MetricT> last_metrics_;
    };
}

#endif