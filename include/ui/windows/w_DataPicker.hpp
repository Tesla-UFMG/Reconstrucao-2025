#ifndef DATAPICKER_WINDOW_HPP
#define DATAPICKER_WINDOW_HPP

// Project
#include "App.hpp"
#include "DB.hpp"
#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

// C++
#include <cstring>
#include <filesystem>
#include <string>

// Third Party
#include "rapidcsv.h"
#include "tinyfiledialogs.h"

namespace Window {
    class DataPicker : public IWindow {
        private:
            std::vector<std::filesystem::path> m_csvToRemove;
            std::vector<std::string> m_telemetryToRemove;

            void refreshData();

            // Payloads
            void sendArchivePayload(const std::string& fileType, const std::string& fileName);
            void sendColumnPayload(const std::string& fileType, const std::string& fileName,
                                   const std::string& columnName);

            // Renders
            void renderMenuBar();
            void renderArchiveContextPopup(const GenericFile& file, int i);
            void renderArchiveNode(const GenericFile& file);
            void renderColumnItem(const std::string& fileType, const std::string& fileName, const std::string& colName);

        public:
            explicit DataPicker(bool* isOpen = nullptr);
            void render() override;
    };

} // namespace Window

#endif