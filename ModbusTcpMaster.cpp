#include "ModbusTcpMaster.h"
#include <iostream>
#include <cstring>
#include <chrono>

ModbusTcpClient* ModbusTcpClient::instance_ = nullptr;

ModbusTcpClient::ModbusTcpClient(const std::string& ip, int port)
    : ip_(ip), port_(port), ctx_(nullptr), keep_running_(false) {
    signal(SIGINT, ModbusTcpClient::handleSignal);
    frame_[0] = 0xAA;
    frame_[1] = 0x55;
    instance_ = this;
}

ModbusTcpClient::~ModbusTcpClient() {
    stop();
}

void ModbusTcpClient::connect() {
    ctx_ = modbus_new_tcp(ip_.c_str(), port_);
    modbus_set_slave(ctx_, 1);
    if (modbus_connect(ctx_) == -1) {
        std::cerr << "[Client] Connection failed: " << modbus_strerror(errno) << std::endl;
        modbus_free(ctx_);
        ctx_ = nullptr;
    }
}

void ModbusTcpClient::disconnect() {
    if (ctx_) {
        modbus_close(ctx_);
        modbus_free(ctx_);
        ctx_ = nullptr;
    }
}

void ModbusTcpClient::start() {
    keep_running_ = true;
    polling_thread_ = std::thread(&ModbusTcpClient::pollingLoop, this);
}

void ModbusTcpClient::stop() {
    keep_running_ = false;
    if (polling_thread_.joinable()) polling_thread_.join();
    disconnect();
}
bool ModbusTcpClient::isConnected() const {
    std::lock_guard<std::mutex> lock(comm_mutex_);
    return ctx_ != nullptr;
}
bool ModbusTcpClient::isRunning() const {
    return keep_running_;
}
void ModbusTcpClient::setVelocity(float left_velo, float right_velo) {
    std::lock_guard<std::mutex> lock(comm_mutex_);

    uint32_t lspd_raw, rspd_raw;
    // lspd_raw = static_cast<uint32_t>(std::abs(left_velo) * 1000);
    lspd_raw = static_cast<uint32_t>(65536);
    rspd_raw = static_cast<uint32_t>(std::abs(right_velo) * 1000);

    frame_[4] = (lspd_raw >> 16) & 0xFFFF;
    frame_[5] = lspd_raw & 0xFFFF;
    frame_[6] = (left_velo >= 0) ? 1 : -1;

    frame_[7] = (rspd_raw >> 16) & 0xFFFF;
    frame_[8] = rspd_raw & 0xFFFF;
    frame_[9] = (right_velo >= 0) ? 1 : -1;
}

void ModbusTcpClient::enableMoving(bool enable) {
    std::lock_guard<std::mutex> lock(comm_mutex_);
    frame_[10] = enable ? 0x03 : 0x00;
}

// private methods
void ModbusTcpClient::pollingLoop() {
    connect();
    while (keep_running_) {
        if (!ctx_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            connect();
            continue;
        }

        updateAndSend();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ModbusTcpClient::updateAndSend() {
    std::lock_guard<std::mutex> lock(comm_mutex_);

    heartbeat_counter_++;
    frame_[2] = (heartbeat_counter_ >> 16) & 0xFFFF;
    frame_[3] = heartbeat_counter_ & 0xFFFF;

    if (ctx_) {
        int rc = modbus_write_registers(ctx_, 0, 11, frame_);
        if (rc == -1) {
            std::cerr << "[Client] Send failed: " << modbus_strerror(errno) << std::endl;
            disconnect();
        }
    }
}

void ModbusTcpClient::handleSignal(int signo) {
    if (instance_) instance_->keep_running_ = false;
}

