#pragma once
#include <modbus/modbus.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdint>
#include <chrono>
#include <iostream>

class ModbusTcpClient {
public:
    ModbusTcpClient(const std::string& ip = "127.0.0.1", int port = 1502);
    ~ModbusTcpClient();

    void start();
    void stop();
    bool isConnected() const;
    bool isRunning() const;

    void setVelocity(float left_velo, float right_velo);
    void enableMoving(bool enable);
    void updateAndSend();

private:
    std::string ip_;
    int port_;
    modbus_t* ctx_;
    std::thread polling_thread_;
    bool keep_running_;
    mutable std::mutex comm_mutex_; // 允许在 const 方法里加锁
    std::atomic<bool> connected_{false};

    static ModbusTcpClient* instance_;

    uint16_t frame_[11] = {};
    uint32_t heartbeat_counter_ = 0;

    void pollingLoop();
    void connect();
    void disconnect();
    static void handleSignal(int signo);
};