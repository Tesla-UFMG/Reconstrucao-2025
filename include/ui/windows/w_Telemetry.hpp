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
#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>

// Third Party
#include "rapidcsv.h"
#include "serialib.h"
#include "tinyfiledialogs.h"

// Defines
#define MAX_RECENT_MESSAGES 1000

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
            std::string              outputPacketFolder;

            // Pacotes
            std::string              packetName;
            std::string              packetId;
            std::vector<std::string> packetColumnNames;
            bool                     processingStatus;
            bool                     m_editMode = false;
            std::string              m_editPacketId;
            bool                     m_showRecentMessages = true;

            // Pilotos e Testes
            std::vector<std::string> m_pilots;
            std::vector<std::string> m_testTypes;
            int                      m_selectedPilotIndex = 0;
            int                      m_selectedTestTypeIndex = 0;

            // Rastreamento do tempo do último salvamento
            std::chrono::steady_clock::time_point m_lastSaveTime;
            bool                     m_hasSaved = false;



            // Comentários da Sessão Ativa
            std::vector<std::string> m_activeCommentDates;
            std::vector<std::string> m_activeComments;
            char                     m_currentCommentBuf[256];
            int                      m_commentOffsetSec = 30;
            std::thread             readerThread;
            std::mutex              queueMutex;
            std::queue<std::string> messageQueue;
            std::atomic<bool>       keepReading{false};
            static Telemetry*        s_instance;

            void clearAndResizeInputBuffers();

            void getAvailablePorts();
            void closeDevice();
            void readMessages();
            void drainQueueIntoRecent();
            void openDevice(const char* port, int baud);
            bool processPacket(const std::string& packet);

            void renderConfigMenu();
            void renderPacketConfigMenu();
            void renderRecentMessages();
            void renderSavingMenu();

            void savePacketsToFile(const std::string& outputFolder);

        public:
            explicit Telemetry(bool* isOpen = nullptr);
            ~Telemetry();

            void render() override;

            void toggleConnection();
            static Telemetry* getInstance();
    };

} // namespace Window

#endif