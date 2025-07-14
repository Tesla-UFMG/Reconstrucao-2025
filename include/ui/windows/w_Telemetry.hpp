#ifndef Telemetry_WINDOW_HPP
#define Telemetry_WINDOW_HPP

// Project
#include "App.hpp"
#include "DB.hpp"
#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

// C++
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <vector>

// Third Party
#include "rapidcsv.h"
#include "serialib.h"
#include "tinyfiledialogs.h"

namespace Window {
    class Telemetry : public IWindow {
        private:
            // Configuração UART
            serialib                 device;
            int                      baudrate;
            std::string              serialPort;
            std::vector<std::string> serialPorts;
            int                      selectedPortIndex;
            std::vector<std::string> recentMessages;
            bool                     saveToFile;

            // Pacotes
            std::string              packetName;
            std::string              packetId;
            std::vector<std::string> packetColumnNames;
            bool                     processingStatus;

            // Thread
            std::thread             readerThread;
            std::mutex              queueMutex;
            std::queue<std::string> messageQueue;
            std::atomic<bool>       keepReading{false};

            void getAvailablePorts();
            void closeDevice();
            void readMessages();
            void drainQueueIntoRecent();
            void openDevice(const char* port, int baud);
            bool processPacket(const std::string& packet);

            void renderConfigMenu();
            void renderPacketConfigMenu();
            void renderRecentMessages();

        public:
            explicit Telemetry(bool* isOpen = nullptr);
            ~Telemetry();

            void render() override;
    };

} // namespace Window

#endif