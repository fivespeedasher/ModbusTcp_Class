#pragma once
#include <modbus/modbus.h>
#include <string>
#include <thread>
#include <atomic>

class ModbusTcpServer {
public:
    static constexpr int REG_SIZE = 12;
    static constexpr uint16_t REG_HEADER_0 = 0xAA;
    static constexpr uint16_t REG_HEADER_1 = 0x55;

    ModbusTcpServer(const std::string& ip = "127.0.0.1", int port = 1501);
    ~ModbusTcpServer();

    void start();
    bool isRunning() const;
    bool isClientConnected() const;
    
    void updateVelocity(float left_velo, float right_velo);
    void enableMoving(bool enable);

private:
    std::string ip_;
    int port_;
    bool keep_running_;
    uint32_t heartbeat_counter_ = 0;
    modbus_t* ctx_;
    modbus_mapping_t* mb_mapping_;
    std::thread heartbeat_thread_;
    std::thread server_thread_;
    std::atomic<bool> client_connected_{false};

    static ModbusTcpServer* instance_;

    void init();
    void startHeartbeatThread();
    void startModbusServerThread();
    void stop();
    static void handleSignal(int signo);

    void setUint32(int addr, uint32_t value);
    void setFloat(int addr, float value);
};