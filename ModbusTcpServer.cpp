#include "include/ModbusTcpServer.h"
#include <iostream>
#include <csignal>
#include <chrono>
#include <cstring>

ModbusTcpServer* ModbusTcpServer::instance_ = nullptr;

ModbusTcpServer::ModbusTcpServer(const std::string& ip, int port)
    : ip_(ip), port_(port), keep_running_(true), ctx_(nullptr), mb_mapping_(nullptr) {
    signal(SIGINT, ModbusTcpServer::handleSignal);
    instance_ = this;
    init();
}

ModbusTcpServer::~ModbusTcpServer() {
    stop();
}

void ModbusTcpServer::start() {
    startHeartbeatThread();
    startModbusServerThread();
}

void ModbusTcpServer::setUint32(int addr, uint32_t value) {
    mb_mapping_->tab_registers[addr]     = (value >> 16) & 0xFFFF;
    mb_mapping_->tab_registers[addr + 1] = value & 0xFFFF;
}

void ModbusTcpServer::setFloat(int addr, float value) {
    uint32_t raw;
    std::memcpy(&raw, &value, sizeof(float));
    setUint32(addr, raw);
}

void ModbusTcpServer::updateVelocity(float left_velo, float right_velo) {
    setFloat(4, std::abs(left_velo));
    mb_mapping_->tab_registers[6] = (left_velo >= 0) ? 1 : -1;

    setFloat(7, std::abs(right_velo));
    mb_mapping_->tab_registers[9] = (right_velo >= 0) ? 1 : -1;
}

bool ModbusTcpServer::isRunning() const {
    return keep_running_;
}

void ModbusTcpServer::enableMoving(bool enable) {
    mb_mapping_->tab_registers[10] = enable ? 0x03 : 0x00;
}

void ModbusTcpServer::init() {
    ctx_ = modbus_new_tcp(ip_.c_str(), port_);
    if (!ctx_) {
        throw std::runtime_error("Failed to create Modbus context");
    }

    mb_mapping_ = modbus_mapping_new(0, 0, REG_SIZE, 0);
    if (!mb_mapping_) {
        modbus_free(ctx_);
        throw std::runtime_error("Failed to allocate Modbus mapping");
    }

    mb_mapping_->tab_registers[0] = REG_HEADER_0;
    mb_mapping_->tab_registers[1] = REG_HEADER_1;
    mb_mapping_->tab_registers[2] = 0;
}

void ModbusTcpServer::startHeartbeatThread() {
    heartbeat_thread_ = std::thread([this]() {
        while (keep_running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            setUint32(2, heartbeat_counter_);
            heartbeat_counter_++;
            if(heartbeat_counter_ > 999999) {
                heartbeat_counter_ = 0; // Reset heartbeat counter
            }
        }
    });
}

void ModbusTcpServer::startModbusServerThread() {
    server_thread_ = std::thread([this]() {
        int server_socket = modbus_tcp_listen(ctx_, 10);
        while (keep_running_) {
            int client_socket = server_socket;
            modbus_tcp_accept(ctx_, &client_socket);
            std::cout << "Modbus TCP Client Connected." << std::endl;

            while (keep_running_) {
                uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
                int rc = modbus_receive(ctx_, query);
                if (rc > 0) {
                    modbus_reply(ctx_, query, rc, mb_mapping_);
                } else if (rc == -1) {
                    std::cout << "Client disconnected." << std::endl;
                    break;
                }
            }
        }
    });
}

void ModbusTcpServer::stop() {
    keep_running_ = false;
    if (heartbeat_thread_.joinable()) heartbeat_thread_.join();
    if (server_thread_.joinable()) server_thread_.join();
    if (mb_mapping_) modbus_mapping_free(mb_mapping_);
    if (ctx_) {
        modbus_close(ctx_);
        modbus_free(ctx_);
    }
}

void ModbusTcpServer::handleSignal(int signo) {
    if (instance_) instance_->keep_running_ = false;
}

bool ModbusTcpServer::isClientConnected() const {
    return client_connected_;
}