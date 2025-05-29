#include "ModbusTcpMaster.h"

int main() {
    try {
        ModbusTcpClient client("127.0.0.1", 777);  // 指定服务端IP和端口
        client.start();
        std::cout << "[Main] Client started." << std::endl;

        client.enableMoving(true);

        while(client.isRunning()) {
            client.setVelocity(0.1f, -500000.0f);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        // std::cout << "[Main] Velocity set to (0.1, -5.5)." << std::endl;


    } catch (const std::exception& e) {
        std::cerr << "[Main] Exception: " << e.what() << std::endl;
    }

    return 0;
}
