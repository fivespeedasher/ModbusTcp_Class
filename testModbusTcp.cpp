#include "ModbusTcpServer.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    try {
        ModbusTcpServer server;
        server.start();

        server.enableMoving(true);
        while (server.isRunning()) {
            server.updateVelocity(100.00, -50.00);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        server.enableMoving(false);
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}